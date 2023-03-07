//
// client.hpp
// add by [11/21/2022 cuihaoyun]
// 

#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__
#include <deque>
#include <mutex>
#include <asio.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/msgdef/state.hpp>
#include <asio/extend/object.hpp>
#include <asio/detail/socket_types.hpp>
#include <asio/extend/typedef.hpp>
#include <asio/extend/worker.hpp>

namespace asio {

    using asio::ip::tcp;
    class NetObject;
    // Single Client
    //--------------------------------------------------------------
    class Client : public Worker, public NetObject, public NetClientEvent
    {
    public:
        Client(const std::string &ip, const std::string &port)
            :socket_(io_context_),
             signals_(io_context_),
             is_close_(false),
             connect_state_(ConnectState::ST_STOPPED)
        {
            this->SetConnect(false);
            this->connect_state_ = ConnectState::ST_STARTING;
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(ip, port);
            this->endpoints_ = endpoints;

            //////////////////////////////////////////////////////////////////////////
            // 还需要验证和测试
			signals_.add(SIGINT);
			signals_.add(SIGTERM);
#if defined(SIGQUIT)
			signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

			signals_.async_wait(
				[this](std::error_code /*ec*/, int /*signo*/)
				{
					// The server is stopped by cancelling all outstanding asynchronous
					// operations. Once all operations have finished the io_context::run()
					// call will exit.
			        this->StopContext();
				});

            do_connect(endpoints);
        }

        void Close() override 
        {
            this->disconnect();
        }

        void Send(const Message& msg) override 
        {
            this->write(msg);
        }

        void Deliver(const Message& msg) override
        {
            this->write(msg);
        }

        uint64_t SocketId() override {
            return socket_.native_handle();
        }

		void StopContext() {
			if (!io_context_.stopped())
			{
				io_context_.stop();
			}
		}
    public:
		void Connect(NetObject* pObject) override {}
		void Disconnect(NetObject* pObject) override {}
		void HandleMessage(NetObject* pObject, const Message& msg) override {}
		void PostMsg(const Message& msg) override 
        {
            this->write(msg);
        }
        void Run() override
        {
            io_context_.run();
        }
	private:
        void write(const Message& msg)
        {
			std::lock_guard lock(this->mutex_);
            if (!this->IsConnect()) {
                return;
            }
            asio::post(io_context_,
                [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg);
                if (!write_in_progress && this->IsConnect())
                {
                    do_write();
                }
                });
        }

		void disconnect()
		{
			is_close_ = true;
			this->connect_state_ = ConnectState::ST_STOPPING;
			asio::post(io_context_, [this]() {
				socket_.close();
			    io_context_.stop();
			    this->write_msgs_.clear();
                this->SetConnect(false);
			    this->connect_state_ = ConnectState::ST_STOPPED;
				});
		}

        void close()
        {
            this->connect_state_ = ConnectState::ST_STOPPING;
            asio::post(io_context_, [this]() {
                socket_.close();
                this->write_msgs_.clear();
                this->SetConnect(false);
                this->connect_state_ = ConnectState::ST_STOPPED;
                this->Disconnect(this);
                this->reconnect();
                });
        }

        void reconnect()
        {
			if (is_close_) {
				return;
			}
            this->Reconnect();
            static std::mutex mtx;
            std::lock_guard lock(mtx);
            std::cout << this->GetConnectName() <<"\t"<<"reconnecting" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (!this->IsConnect())
            {
                do_connect(endpoints_);
            }
        }

        void do_connect(const tcp::resolver::results_type& endpoints)
        {
            this->connect_state_ = ConnectState::ST_STARTED;
            asio::async_connect(socket_, endpoints,
                [this](std::error_code ec, tcp::endpoint)
                {
                    if (!ec)
                    {
                        this->connect_state_ = ConnectState::ST_CONNECTED;
                        this->SetConnect(true);
                        std::cout << "connection succeeded." << std::endl;
                        this->Connect(this);
                        do_read_header();
                    }
                    else {
                        this->SetConnect(false);
                        std::cout << "connection failed." << std::endl;
                        this->close();
                    }
                });
            this->connect_state_ = ConnectState::ST_CONNECTING;
        }

        void do_read_header()
        {
            asio::async_read(socket_,
                asio::buffer(read_msg_.data(), Message::header_length),
                [this](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec && read_msg_.decode_header())
                    {
                        do_read_body();
                    }
                    else
                    {
                        this->close();
                    }
                });
        }

        void do_read_body()
        {
            asio::async_read(socket_,
                asio::buffer(read_msg_.body(), read_msg_.body_length()),
                [this](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
#if 0
                        std::cout.write(read_msg_.body(), read_msg_.body_length());
                        std::cout << "\n";
#endif
                        this->HandleMessage(this, read_msg_);
                        do_read_header();
                    }
                    else
                    {
                        this->close();
                    }
                });
        }

        void do_write()
        {
            asio::async_write(socket_,
                asio::buffer(write_msgs_.front().data(),
                    write_msgs_.front().length()),
                [this](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
                        if (!write_msgs_.empty()) {
                            write_msgs_.pop_front();
                        }

                        if (!write_msgs_.empty()) {
                            do_write();
                        }
                    }
                    else
                    {
                        this->close();
                    }
                });
        }

    private:
        asio::io_context io_context_;
        asio::signal_set signals_;
        tcp::socket socket_;
        Message read_msg_;
        MessageQueue write_msgs_;
        tcp::resolver::results_type endpoints_;
        bool is_close_;
        ConnectState connect_state_;
		std::mutex mutex_;
    };



}


#endif // __CLIENT_HPP__
