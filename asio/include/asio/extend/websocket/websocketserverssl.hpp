#ifndef __WEBSOCKET_SERVER_SSL_HPP__
#define __WEBSOCKET_SERVER_SSL_HPP__
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
// Example: WebSocket server, asynchronous
//
//------------------------------------------------------------------------------

#include "../ssl/server_certificate.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// step 1
#include <asio/msgdef/message.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/worker.hpp>
#include <asio/extend/typedef.hpp>
using namespace asio;

//------------------------------------------------------------------------------
// Echoes back all received WebSocket messages
class WebSessionSSL
    : public NetObject, public std::enable_shared_from_this<WebSessionSSL>
{
private:
	websocket::stream<
		beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
	Message read_msg_;
    MessageQueue write_msgs_;
    std::mutex mutex_;
    NetServer* server_;
public:
    // Take ownership of the socket
    explicit WebSessionSSL(tcp::socket&& socket, ssl::context& ctx, NetServer* server)
        : ws_(std::move(socket), ctx)
        , server_(server)
    {
        // step 2
        ws_.binary(true);
    }

    virtual ~WebSessionSSL()
    {

    }

    void Close() override
    {

    }

    void Send(const Message& msg) override
    {
        this->write(msg);
    }

    void Post(const Message& msg) override
    {
        this->write(msg);
    }

    // warning error 10009 scope NetObject and socket
    std::string Ip() override
    {
        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint endpoint = this->ws_.next_layer().next_layer().socket().remote_endpoint(ec);
        if (ec) {
			return endpoint.address().to_string();
        }
        return "";
    }

    std::string Port() override
    {
        unsigned short port = 0;
        return std::to_string(port);
    }
    // Get on the correct executor
    void run()
    {
		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(ws_.get_executor(),
			beast::bind_front_handler(
				&WebSessionSSL::on_run,
				this->shared_from_this()));
    }

    // Start the asynchronous operation
    void on_run()
    {
		// Set the timeout.
		beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

		// Perform the SSL handshake
		ws_.next_layer().async_handshake(
			ssl::stream_base::server,
			beast::bind_front_handler(
				&WebSessionSSL::on_handshake,
				this->shared_from_this()));
    }

	void
		on_handshake(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "handshake");

		// Turn off the timeout on the tcp_stream, because
		// the websocket stream has its own timeout system.
		beast::get_lowest_layer(ws_).expires_never();

		// Set suggested timeout settings for the websocket
		ws_.set_option(
			websocket::stream_base::timeout::suggested(
				beast::role_type::server));

		// Set a decorator to change the Server of the handshake
		ws_.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res)
			{
				res.set(http::field::server,
				std::string(BOOST_BEAST_VERSION_STRING) +
				" websocket-server-async-ssl");
			}));

		// Accept the websocket handshake
		ws_.async_accept(
			beast::bind_front_handler(
				&WebSessionSSL::on_accept,
				this->shared_from_this()));
	}

    void on_accept(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        // Connect
        this->server_->Connect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));

        // Read a message
        this->do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WebSessionSSL::on_read,
                this->shared_from_this()));
    }

    void on_read(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
            return fail(ec, "read");

        // step 3 read complete
        // check max buff length valid
        if (this->buffer_.size() > this->read_msg_.max_length())
        {
			// Close the WebSocket connection
			ws_.async_close(websocket::close_code::normal,
				beast::bind_front_handler(
					&WebSessionSSL::on_close,
					this->shared_from_this()));
            return;
        }

        // Copy data into message
		std::memcpy(this->read_msg_.data(), buffer_.data().data(), buffer_.size());
		this->read_msg_.decode_header();
		asio::MsgHeader* header((asio::MsgHeader*)this->read_msg_.data());

        // Handle message
        this->server_->HandleMessage(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()), this->read_msg_);

		//printf("%.*s\n", this->read_msg_.body_length(), this->read_msg_.body());
		
        // Clear the buffer
		buffer_.consume(buffer_.size());

        // Echo the message
		//ws_.text(ws_.got_text());
		//ws_.async_write(
		//	buffer_.data(),
		//	beast::bind_front_handler(
		//		&WebSession::on_write,
		//		this->shared_from_this()));

        this->do_read();
    }

    void on_write(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
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
    // Report a failure
    void fail(beast::error_code ec, char const* what)
    {
        // Disconnect
        this->server_->Disconnect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
        std::cerr << what << ": " << ec.message() << "\n";
    }

	void on_close(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "close");

        // Disconnect
        this->server_->Disconnect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
		// If we get here then the connection is closed gracefully

		// The make_printable() function helps print a ConstBufferSequence
		//std::cout << beast::make_printable(buffer_.data()) << std::endl;
	}
private:

};

//------------------------------------------------------------------------------
// Accepts incoming connections and launches the sessions
class WebSocketServerSSL : public Worker, public NetServer
{
private:
	net::io_context& ioc_;
	ssl::context& ctx_;
	tcp::acceptor acceptor_;
    int threads_;
public:
    WebSocketServerSSL(
		net::io_context& ioc,
		ssl::context& ctx,
        const tcp::endpoint& endpoint, int threads)
		: ioc_(ioc)
		, ctx_(ctx)
		, acceptor_(net::make_strand(ioc))
        , threads_(threads)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }

        //
        this->run();
    }

    virtual ~WebSocketServerSSL()
    {

    }
    // stop asio io_content
    void Stop()
    {
        if (!this->ioc_.stopped())
        {
            this->ioc_.stop();
        }
    }
private:
    // Start accepting incoming connections
    void run()
    {
        do_accept();
    }
protected:
    void Connect(NetObjectPtr pNetObj)     override {}
    void Disconnect(NetObjectPtr pNetObj)  override {}
    void HandleMessage(NetObjectPtr pNetObj, const Message& msg) override {}
    void Error(int error) override {}
private:
    void Exec() override
    {
		// snowflake algrithem
		int mainId = this->MainId() == 0 ? 1 : this->MainId();
		int subId = this->SubId()   == 0 ? 1 : this->SubId();
		this->InitUUID(mainId, subId);

        //this->run();

		// This holds the self-signed certificate used by the server
		//load_server_certificate(ctx_);

		// Run the I/O service on the requested number of threads
		std::vector<std::thread> v;
		v.reserve(threads_ - 1);
        for (auto i = threads_ - 1; i > 0; --i) {
            v.emplace_back(
                [&]
                {
                    this->ioc_.run();
                });
        }
		this->ioc_.run();
    }
    void Init() override
    {

    }
    void Exit() override
    {

    }
private:
    void do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &WebSocketServerSSL::on_accept,
                /*shared_from_this()*/this));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<WebSessionSSL>(std::move(socket), ctx_, this)->run();
        }

        // Accept another connection
        do_accept();
    }

    // Report a failure
    void fail(beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }
};



#endif // __WEBSOCKET_SERVER_SSL_HPP__
