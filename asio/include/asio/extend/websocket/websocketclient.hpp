#ifndef __WEBSOCKET_CLIENT_HPP__
#define __WEBSOCKET_CLIENT_HPP__
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket client, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
// step 1
#include <asio/msgdef/message>
#include <asio/extend/object>
#include <asio/extend/typedef>
#include <asio/extend/worker>
//using namespace asio;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class WebClientWorker;
class WebSession :
	public asio::NetObject
{
	net::io_context ioc_;
	tcp::resolver resolver_;
	websocket::stream<beast::tcp_stream> ws_;
	beast::flat_buffer buffer_; // read buffer
	std::string host_;
	std::string port_;
	asio::MessageQueue write_msgs_;
	std::mutex mutex_;
	asio::NetEvent* net_event_;
	friend class WebClientWorker;
public:
	// Resolver and socket require an io_context
	explicit
		WebSession(asio::NetEvent* event)
		: resolver_(net::make_strand(ioc_))
		, ws_(net::make_strand(ioc_))
		, net_event_(event)
	{
		// step 2
		ws_.binary(true);
	}

	virtual ~WebSession()
	{
		this->Final();
	}
private:
	// Start the asynchronous operation
	void run(
		char const* host,
		char const* port)
	{
		// Save these for later
		host_ = host;
		port_ = port;
		// Look up the domain name
		resolver_.async_resolve(
			host,
			port,
			beast::bind_front_handler(
				&WebSession::on_resolve,
				/*shared_from_this()*/this));
	}
public: // NetObject
	void Send(const asio::Message& msg) override
	{
		this->write(msg);
	}
	void Post(const asio::Message& msg) override
	{
		this->write(msg);
	}
	std::string Ip() override
	{
		return this->host_;
	}
	std::string Port() override
	{
		return this->port_;
	}
	uint64 SocketId() override
	{
		return 0;
	}
	void Close() override
	{
		// Close the WebSocket connection
		if (ws_.is_open()) {
			ws_.async_close(websocket::close_code::normal,
				beast::bind_front_handler(
					&WebSession::on_close,
					/*shared_from_this()*/this));
		}
	}
	void StopContext()
	{
		this->Close();
	}
public:
	void Run()
	{
		this->ioc_.run();
	}
private:
	void Final() override
	{

	}
	void clear()
	{
		std::lock_guard lock(this->mutex_);
		this->write_msgs_.clear();
	}
	void write(const asio::Message& msg)
	{
		std::lock_guard lock(this->mutex_);
		bool write_in_progress = !write_msgs_.empty();
		this->write_msgs_.push_back(msg);
		if (!write_in_progress) {
			this->do_write();
		}
	}
	void do_write()
	{
		// Send the message
		ws_.async_write(
			net::buffer(write_msgs_.front().data(),
				write_msgs_.front().length()),
			[this](beast::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);
				if (ec) {
					return fail(ec, "write");
				}
				else {
					if (!write_msgs_.empty()) {
						this->write_msgs_.pop_front();
					}

					if (!write_msgs_.empty()) {
						do_write();
					}
				}
			});
	}
private:
	void on_resolve(
		beast::error_code ec,
		tcp::resolver::results_type results)
	{
		if (ec)
			return fail(ec, "resolve");

		// Set the timeout for the operation
		beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

		// Make the connection on the IP address we get from a lookup
		beast::get_lowest_layer(ws_).async_connect(
			results,
			beast::bind_front_handler(
				&WebSession::on_connect,
				/*shared_from_this()*/this));
	}

	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
	{
		if (ec)
			return fail(ec, "connect");

		// Turn off the timeout on the tcp_stream, because
		// the websocket stream has its own timeout system.
		beast::get_lowest_layer(ws_).expires_never();

		// Set suggested timeout settings for the websocket
		ws_.set_option(
			websocket::stream_base::timeout::suggested(
				beast::role_type::client));

		// Set a decorator to change the User-Agent of the handshake
		ws_.set_option(websocket::stream_base::decorator(
			[](websocket::request_type& req)
			{
				req.set(http::field::user_agent,
				std::string(BOOST_BEAST_VERSION_STRING) +
				" websocket-client-async");
			}));

		// Update the host_ string. This will provide the value of the
		// Host HTTP header during the WebSocket handshake.
		// See https://tools.ietf.org/html/rfc7230#section-5.4
		host_ += ':' + std::to_string(ep.port());

		// Perform the websocket handshake
		ws_.async_handshake(host_, "/",
			beast::bind_front_handler(
				&WebSession::on_handshake,
				/*shared_from_this()*/this));
	}

	void on_handshake(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "handshake");

		// websocket connect
		this->net_event_->Connect(dynamic_cast<NetObject*>(this));

		// step 3
		std::string text = "hello";
		static asio::Message msg;
		msg.body_length(text.length());
		std::memcpy(msg.body(), text.data(), text.size());
		asio::MsgHeader header;
		header.seq = 50002;
		header.body_len = msg.body_length();
		msg.encode_header(header);

		// Send the message
		//ws_.async_write(
		//	net::buffer(msg.data(), msg.length()),
		//	beast::bind_front_handler(
		//		&session::on_write,
		//		/*shared_from_this()*/this));
		//
		this->Post(msg);

		// Read a message
		this->read();
	}

	void on_write(
		beast::error_code ec,
		std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "write");

		// Read a message into our buffer
		ws_.async_read(
			buffer_,
			beast::bind_front_handler(
				&WebSession::on_read,
				/*shared_from_this()*/this));
	}

	void read()
	{
		// Read a message into our buffer
		ws_.async_read(
			buffer_,
			beast::bind_front_handler(
				&WebSession::on_read,
				/*shared_from_this()*/this));
	}

	void on_read(
		beast::error_code ec,
		std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "read");

		// step 4 check
		int header_size = sizeof(asio::MsgHeader);
		asio::Message msg;
		std::memcpy(msg.data(), buffer_.data().data(), buffer_.size());
		msg.decode_header();
		asio::MsgHeader* header((asio::MsgHeader*)msg.data());

		// handle message
		this->net_event_->HandleMessage(dynamic_cast<NetObject*>(this), msg);

		// read

		// Close the WebSocket connection
		if (ws_.is_open()) {
			ws_.async_close(websocket::close_code::normal,
				beast::bind_front_handler(
					&WebSession::on_close,
					/*shared_from_this()*/this));
		}
	}

	void on_close(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "close");

		// disconnect
		this->net_event_->Disconnect(dynamic_cast<NetObject*>(this));

		// If we get here then the connection is closed gracefully

		// The make_printable() function helps print a ConstBufferSequence
		std::cout << beast::make_printable(buffer_.data()) << std::endl;
	}
};

/// <summary>
/// WebClientWorker thread
/// </summary>
class WebClientWorker : public asio::Worker, public asio::NetEvent
{
public:
	WebClientWorker() {}
	virtual ~WebClientWorker() {}
	void Stop()
	{
		if (this->session_)
		{
			this->session_->StopContext();
		}
	}
	void SetAutoReconnect(bool bAutoReconnect)
	{
		this->auto_reconnect_ = bAutoReconnect;
	}
	void SetEndpoint(const std::string_view& host, const std::string_view& port)
	{
		this->host_ = host;
		this->port_ = port;
	}
public:
	void Post(const asio::Message& msg) override
	{
		if (this->session_)
		{
			this->session_->Post(msg);
		}
	}
	void Send(const asio::Message& msg) override
	{
		if (this->session_)
		{
			this->session_->Post(msg);
		}
	}
protected:
private:
	void Init() override
	{

	}
	void Exit() override
	{

	}
	void Exec() override
	{
		if (this->host_.empty() || this->port_.empty()) {
			return;
		}
		do
		{
			this->session_ = std::make_shared<WebSession>(this);
			this->session_->run(host_.c_str(), port_.c_str());
			this->session_->Run();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		} while (this->auto_reconnect_);
	}
private:
	bool auto_reconnect_ = { false };
	std::shared_ptr<WebSession> session_;
	std::string host_;
	std::string port_;
};

#endif // __WEBSOCKET_CLIENT_HPP__
