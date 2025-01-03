#ifndef __WEBSOCKET_SSL_HPP__
#define __WEBSOCKET_SSL_HPP__
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
// Example: WebSocket SSL client, asynchronous
//
//------------------------------------------------------------------------------

#include "../ssl/root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------
#include <asio/extend/object>
#include <asio/extend/typedef>
#include <asio/extend/worker>
using namespace asio;
// Sends a WebSocket message and prints the response
class WebClientSessionSSL 
    : public asio::NetObject, public std::enable_shared_from_this<WebClientSessionSSL>
{
    tcp::resolver resolver_;
    websocket::stream<
        beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    asio::Message read_msg_;
    asio::MessageQueue write_msgs_;
    std::mutex mutex_;
    asio::NetEvent* net_event_;
    friend class WebClientSSLWorker;
public:
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
    void Close() override
    {
        // Close the WebSocket connection
        if (ws_.is_open()) {
            ws_.async_close(websocket::close_code::normal,
                beast::bind_front_handler(
                    &WebClientSessionSSL::on_close,
                    this->shared_from_this()));
        }
    }
public:
    // Resolver and socket require an io_context
    explicit
        WebClientSessionSSL(net::io_context& ioc, ssl::context& ctx, asio::NetEvent* event)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc), ctx)
        , net_event_(event)
    {
        ws_.binary(true);
    }

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
                &WebClientSessionSSL::on_resolve,
                this->shared_from_this()));
    }

    void on_resolve(
            beast::error_code ec,
            tcp::resolver::results_type results)
    {
        if (ec)
            return fail(ec, "resolve");

        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &WebClientSessionSSL::on_connect,
                this->shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if (ec)
            return fail(ec, "connect");

        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(
            ws_.next_layer().native_handle(),
            host_.c_str()))
        {
            ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            return fail(ec, "connect");
        }

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());

        // Perform the SSL handshake
        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &WebClientSessionSSL::on_ssl_handshake,
                this->shared_from_this()));
    }

    void on_ssl_handshake(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "ssl_handshake");

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
                " websocket-client-async-ssl");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &WebClientSessionSSL::on_handshake,
                this->shared_from_this()));
    }

    void on_handshake(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "handshake");

        // Send the message
        //ws_.async_write(
        //	net::buffer(text_),
        //	beast::bind_front_handler(
        //		&session::on_write,
        //		shared_from_this()));

        // Net connect
        this->net_event_->Connect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));

        std::string text = "hello";
        static asio::Message msg;
        msg.body_length(static_cast<int>(text.length()));
        std::memcpy(msg.body(), text.data(), text.size());
        asio::MsgHeader header;
        header.msgId = 50002;
        header.body_len = msg.body_length();
        msg.encode_header(header);
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

        if (!write_msgs_.empty()) {
            this->write_msgs_.pop_front();
        }

        if (!write_msgs_.empty()) {
            do_write();
        }

        // Read a message into our buffer
        //ws_.async_read(
        //    buffer_,
        //    beast::bind_front_handler(
        //        &session::on_read,
        //        shared_from_this()));
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
            beast::bind_front_handler(
                &WebClientSessionSSL::on_write,
                this->shared_from_this()));
    }

    void read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WebClientSessionSSL::on_read,
                this->shared_from_this()));
    }

    void on_read(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        // Close the WebSocket connection
        //ws_.async_close(websocket::close_code::normal,
        //    beast::bind_front_handler(
        //        &session::on_close,
        //        shared_from_this()));

        // step 4 check
		asio::Message* msg = &this->read_msg_;
		std::memcpy(msg->data(), buffer_.data().data(), buffer_.size());
        if (!msg->decode_header())
        {
            fail(ec, "error msg header");
            return;
        }
		//asio::MsgHeader* header((asio::MsgHeader*)msg->data());

        // Net handle message
        this->net_event_->HandleMessage(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()), *msg);

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // receive data
        this->read();
    }

    void on_close(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        // std::cout << beast::make_printable(buffer_.data()) << std::endl;
    }
    // 
    // Report a failure
    void fail(beast::error_code ec, char const* what)
    {
        this->net_event_->Error(ec.value());

        std::cerr << what << ": " << ec.message() << "\n";

        // Net disconnect
        this->net_event_->Disconnect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
    }
};

//------------------------------------------------------------------------------

/// <summary>
/// WebClientSSLWorker thread
/// </summary>
class WebSocketSSLClient : public asio::Worker, public asio::NetEvent
{
public:
    WebSocketSSLClient() {}
    virtual ~WebSocketSSLClient() {}
    void Stop()
    {
        this->SetAutoReconnect(false);
        if (this->ws_) {
            this->ws_->Close();
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
        if (this->ws_)
        {
            this->ws_->Post(msg);
        }
    }
    void Send(const asio::Message& msg) override
    {
        if (this->ws_)
        {
            this->ws_->Send(msg);
        }
    }
protected:
    virtual void Connect(NetObjectPtr pNetObj) override {}
    virtual void Disconnect(NetObjectPtr pNetObj) override {}
    virtual void HandleMessage(NetObjectPtr pNetObj, const Message& msg) override
    {
        printf("%.*s\n", msg.body_length(), msg.body());
    }
    virtual void Error(int error) override {}
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
            // The io_context is required for all I/O
            net::io_context ioc;

            // The SSL context is required, and holds certificates
            ssl::context ctx{ssl::context::tlsv12_client};

            // This holds the root certificate used for verification
            load_root_certificates(ctx);

            // Launch the asynchronous operation
            auto ws = std::make_shared<WebClientSessionSSL>(ioc, ctx, this);
            this->ws_ = ws;
            ws->run(host_.c_str(), port_.c_str());
            /*
            typedef std::shared_ptr<session> session_ptr;
            std::vector<session_ptr> wslist;
            for (size_t i = 0; i < 10; i++)
            {
                std::make_shared<WebClientSessionSSL>(ioc, ctx, this)->run(host_.c_str(), port_.c_str());
            }
            */
            // Run the I/O service. The call will return when
            // the socket is closed.
            ioc.run();

            // Release local scope session reference of io_context
            this->ws_ = nullptr;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        } while (this->auto_reconnect_);
    }
private:
    bool auto_reconnect_ = { false };
    std::string host_;
    std::string port_;
    std::shared_ptr<WebClientSessionSSL> ws_;
};

//-------------------------------------------------------------------------

#endif // __WEBSOCKET_SSL_HPP__
