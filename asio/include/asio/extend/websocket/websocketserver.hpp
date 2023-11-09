#ifndef __WEBSOCKET_SERVER_HPP__
#define __WEBSOCKET_SERVER_HPP__
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

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
// step 1
#include <asio/msgdef/message.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/worker.hpp>
#include <asio/extend/typedef.hpp>
#include <asio/extend/websocket/websocketroom>
using namespace asio;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------
// Echoes back all received WebSocket messages
class WebSession
    : public NetObject
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    Message read_msg_;
    MessageQueue write_msgs_;
    std::mutex mutex_;
    WebSocketRoom &room_;
public:
    // Take ownership of the socket
    explicit WebSession(tcp::socket&& socket, WebSocketRoom& room)
        : ws_(std::move(socket))
        , room_(room)
    {
        // step 2
        ws_.binary(true);
        
        //
        //this->room_.Join(this->shared_from_this());
    }

    virtual ~WebSession()
    {
        //this->room_.Leave(this->shared_from_this());
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
                &WebSession::on_run,
                /*shared_from_this()*/this));
    }

    // Start the asynchronous operation
    void on_run()
    {
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
                " websocket-server-async");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &WebSession::on_accept,
                /*shared_from_this()*/this));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        //
        //this->room_.Join(this->shared_from_this());
        // Read a message
        this->do_read();
    }

    void do_read()
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

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
            return fail(ec, "read");

        // step 3 read complete
        int header_size = sizeof(asio::MsgHeader);
        asio::Message msg;
        std::memcpy(msg.data(), buffer_.data().data(), buffer_.size());
        msg.decode_header();
        asio::MsgHeader* header((asio::MsgHeader*)msg.data());

        // Echo the message
        //ws_.text(ws_.got_text());
        //ws_.async_write(
        //    buffer_.data(),
        //    beast::bind_front_handler(
        //        &WebSession::on_write,
        //        /*shared_from_this()*/this));

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
        //this->room_.Leave(this->shared_from_this());
        std::cerr << what << ": " << ec.message() << "\n";
    }
    private:

};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class WebSocketServer : public Worker, public NetServer
{
    net::io_context ioc_{1};
    tcp::acceptor acceptor_;
    WebSocketRoom room_;
public:
    WebSocketServer(
        /*net::io_context& ioc, */
        tcp::endpoint endpoint)
        : acceptor_(ioc_)
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

    virtual ~WebSocketServer()
    {

    }
    // stop asio io_content
    void StopContext()
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
private:
    void Connect(NetObjectPtr pNetObj)    override {}
    void Disconnect(NetObjectPtr pNetObj) override {}
    void HandleMessage(Message& msg)      override {}
    void Error(int error) override {}
    void Exec() override
    {
        // thread workers
		std::vector<std::thread> v;
		v.reserve(1);
		int threads = 1;
		for (auto i = threads - 1; i > 0; --i) {
			v.emplace_back(
				[&]
				{
					this->ioc_.run();
				});
		}

        // main thread worker
        this->ioc_.run();

        // wait for threads exit
        for (auto &it : v)
        {
            if (it.joinable())
            {
                it.join();
            }
        }
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
                &WebSocketServer::on_accept,
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
            std::make_shared<WebSession>(std::move(socket), this->room_)->run();
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



#endif // __WEBSOCKET_SERVER_HPP__
