//
// server.hpp
// add by [11/21/2022 cuihaoyun]
#ifndef __SERVER_HPP__
#define __SERVER_HPP__
#include <set>
#include <mutex>
#include <asio.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/msgdef/state.hpp>
#include <asio/extend/object.hpp>
#include <asio/detail/socket_types.hpp>
#include <asio/extend/typedef.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/worker.hpp>
#include <asio/extend/room.hpp>

namespace asio {

    using asio::ip::tcp;
    class NetServerEvent;
    class Room;

    //----------------------------------------------------------------------
    // Session Server
    class Session
        : public NetObject,
        public std::enable_shared_from_this<NetObject>
    {
    public:
        Session(tcp::socket socket, Room& room, NetServer* netserver) noexcept
            : socket_(std::move(socket)),
            room_(room),
            net_server_(netserver) 
        {

        }

        virtual ~Session() {
            this->Final();
        }

        void Close() override
        {
            this->socket_.close();
        }

        void Start()
        {
            room_.Join(shared_from_this());
            net_server_->Connect(shared_from_this());
#if 1
			read_msg_.setNetId(this->SocketId());
			read_msg_.setNetObject(shared_from_this());
#endif
            this->SetConnect(true);
            do_read_header();
        }

        void Deliver(const Message& msg)
        {
			std::lock_guard lock(this->mutex_);
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress)
            {
                do_write();
            }
        }

		void Send(const Message& msg) override
		{
			this->Deliver(msg);
		}
        
        void Post(const Message& msg) override
        {
            this->Deliver(msg);
        }

        uint64_t SocketId() override final {
            return this->socket_.native_handle();
        }

    private:
        void do_read_header()
        {
            auto self(shared_from_this());
            asio::async_read(socket_,
                asio::buffer(read_msg_.data(), Message::header_length),
                [this, self](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec && read_msg_.decode_header())
                    {
                        do_read_body();
                    }
                    else
                    {
                        room_.Leave(shared_from_this());
                        net_server_->Disconnect(shared_from_this());
                        this->SetConnect(false);
                    }
                });
        }

        void do_read_body()
        {
            auto self(shared_from_this());
            asio::async_read(socket_,
                asio::buffer(read_msg_.body(), read_msg_.body_length()),
                [this, self](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
#if 0
                        read_msg_.setNetId(this->SocketId());
						read_msg_.setNetObject(shared_from_this());
#endif
                        //room_.Deliver(read_msg_);
                        net_server_->HandleMessage(read_msg_);
                        do_read_header();
                    }
                    else
                    {
                        room_.Leave(shared_from_this());
                        net_server_->Disconnect(shared_from_this());
                        this->SetConnect(false);
                    }
                });
        }

        void do_write()
        {
            auto self(this->shared_from_this());
            asio::async_write(socket_,
                asio::buffer(write_msgs_.front().data(),
                    write_msgs_.front().length()),
                [this, self](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
                        write_msgs_.pop_front();
                        if (!write_msgs_.empty())
                        {
                            do_write();
                        }
                    }
                    else
                    {
                        room_.Leave(shared_from_this());
                        net_server_->Disconnect(shared_from_this());
                        this->SetConnect(false);
                    }
                });
        }

        void Final() override
        {
            std::lock_guard lock(this->mutex_);
            this->write_msgs_.clear();
        }

    private:
        tcp::socket socket_;
        Room& room_;
        Message read_msg_;
        MessageQueue write_msgs_;
        NetServer* net_server_;
		std::mutex mutex_;
    };

    //----------------------------------------------------------------------
    // Singleton Server Basic class
    class Server : public Worker, public NetServer
    {
    public:
        Server(const tcp::endpoint& endpoint)
            : acceptor_(io_context, endpoint)
            , signals_(io_context)
            , stoped_(false)
        {
			signals_.add(SIGINT);
			signals_.add(SIGTERM);
#if defined(SIGQUIT)
			signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
			acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			signals_.async_wait(
				[this](std::error_code /*ec*/, int /*signal*/)
				{
					// The server is stopped by cancelling all outstanding asynchronous
					// operations. Once all operations have finished the io_context::run()
					// call will exit.
					if(acceptor_.is_open())
					{
						acceptor_.close();
					}
					this->StopContext();
					this->stoped_ = true;
				});
            this->do_accept();
        }

        virtual ~Server() {
            this->Final();
        }

        // stop asio io_content
		void StopContext() 
        {
            this->Shutdown();
		}

        void Shutdown()
        {
			if (!io_context.stopped())
			{
				io_context.stop();
			}
        }
    public:
		void Connect(NetObjectPtr pNetObj)    override {}
		void Disconnect(NetObjectPtr pNetObj) override {}
		void HandleMessage(Message& msg)      override {}
    private:
        // single thread run
        void Run() override
        {
            io_context.run();
        }

        void AfterInit()  override
        {
            int serverId = this->ServerId();
            int subId = this->ServerSubId();
            if (serverId == 0) {
                serverId = 1;
            }

            if (subId == 0) {
                subId = 1;
            }
            this->room_.Init(serverId, subId);
        }

        void BeforeExit() override
        {

        }

        void do_accept()
        {
            acceptor_.async_accept(
                [this](std::error_code ec, tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::make_shared<Session>(std::move(socket), room_, dynamic_cast<NetServer*>(this))->Start();
                    }

                    this->do_accept();
                });
        }
    private:
        // io_service
		asio::io_context io_context;
        tcp::acceptor acceptor_;
        Room room_;
        asio::signal_set signals_;
        bool stoped_;
    };

    //----------------------------------------------------------------------



}

#endif // __SERVER_HPP__
