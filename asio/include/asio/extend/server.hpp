#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__
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
    // Tcp Session Server
    class TcpSession
        : public NetObject
        , public std::enable_shared_from_this<TcpSession>
    {
    public:
        explicit TcpSession(tcp::socket socket, Room& room, NetServer* server) noexcept
            : socket_(std::move(socket)),
            room_(room),
            server_(server)
        {

        }

        virtual ~TcpSession() 
        {
            this->Final();
        }

        tcp::socket& Socket()
        {
            return this->socket_;
        }

        void Close() override
        {
            if (this->socket_.is_open())
            {
				this->socket_.close();
            }
        }

        void Start()
        {
            // init snowflake generate session id
            const uint64 sessionId = this->server_->GenerateUUId();
            this->setSessionId(sessionId);
            // server room jion
            this->room_.Join(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
			this->read_msg_.setNetObject(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
            // connect event
			this->SetConnect(true);
            this->server_->Connect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
            // start receive stream data
            do_read_header();
        }

        void Deliver(const Message& msg)
        {
            this->write(msg);
        }

		void Send(const Message& msg)    override
		{
			this->write(msg);
		}
        
        void Post(const Message& msg)    override
        {
            this->write(msg);
        }

		// warning error 10009 scope NetObject and socket
        std::string Ip() override
        {
            return this->socket_.remote_endpoint().address().to_string();
        }

        std::string Port() override
        {
            unsigned short port = this->socket_.remote_endpoint().port();
            return std::to_string(port);
        }
    private:
        void clear()
        {
            std::lock_guard lock(this->mutex_);
            this->write_msgs_.clear();
        }
        void write(const Message& msg)
        {
            std::lock_guard lock(this->mutex_);
            bool write_in_progress = !write_msgs_.empty();
            this->write_msgs_.push_back(msg);
            if (!write_in_progress)
            {
                this->do_write();
            }
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
                        if (this->server_->IsPackSessionId())
                        {
                            MsgHeader* header = (MsgHeader*)(this->read_msg_.data());
                            header->sessionId = this->getSessionId();
                        }
                        do_read_body();
                    }
                    else
                    {
						this->server_->Disconnect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
                        this->room_.Leave(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
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
                        this->server_->HandleMessage(read_msg_);
                        do_read_header();
                    }
                    else
                    {
						this->server_->Disconnect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
                        this->room_.Leave(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
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
                        this->write_msgs_.pop_front();
                        if (!write_msgs_.empty())
                        {
                            do_write();
                        }
                    }
                    else
                    {
						this->server_->Disconnect(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
                        this->room_.Leave(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
                        this->SetConnect(false);
                    }
                });
        }

        void Final()
        {
            std::lock_guard lock(this->mutex_);
            this->write_msgs_.clear();
        }

    private:
        tcp::socket socket_;
        Room& room_;
        Message read_msg_;
        MessageQueue write_msgs_;
        NetServer* server_;
		std::mutex mutex_;
    };

    //----------------------------------------------------------------------
    // Singleton Server Basic class
    class TcpServerWorker : public Worker, public NetServer
    {
    public:
        explicit TcpServerWorker(const tcp::endpoint& endpoint)
            : acceptor_(io_context, endpoint)
            , signals_(io_context)
            , stoped_(false)
        {
            // ensure one signals handler application
			//signals_.add(SIGINT);
			//signals_.add(SIGTERM);
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
					this->Stop();
					this->stoped_ = true;
				});
            this->do_accept();
        }

        virtual ~TcpServerWorker() {}

        // stop asio io_content
		void Stop() 
        {
            this->signals_.cancel();
			// close acceptor
			this->io_context.post([this]() {
				if (this->acceptor_.is_open())
				{
					this->acceptor_.close();
				}
				});
			// io service
			if (!this->io_context.stopped())
			{
				this->io_context.stop();
			}
		}

        Room& getRoom() 
        {
            return this->room_;
        }

    public:
		void Connect(NetObjectPtr pNetObj)    override {}
		void Disconnect(NetObjectPtr pNetObj) override {}
		void HandleMessage(Message& msg)      override {}
    protected:
        void SetName(const std::string_view& name)
        {
            this->name_ = name;
        }
    private:
        // single thread run
        void Exec() override
        {
            // multi thread
		    // Run the I/O service on the requested number of threads
			/*
            std::vector<std::thread> v;
			v.reserve(threads - 1);
			for (auto i = threads - 1; i > 0; --i)
				v.emplace_back(
					[&ioc]
					{
						ioc.run();
					});
            */
            io_context.run();
        }

        void AfterInit()  override final
        {
            int serverId = this->ServerId() == 0 ? 1 : this->ServerId();
            int subId = this->ServerSubId() == 0 ? 1 : this->ServerSubId();
            this->InitUUID(serverId, subId);
        }

        void BeforeExit() override final
        {

        }

        // accept
        void do_accept()
        {
            acceptor_.async_accept(
                [this](std::error_code ec, tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::make_shared<TcpSession>(std::move(socket), room_, dynamic_cast<NetServer*>(this))->Start();
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
        std::string name_;
    };

    //----------------------------------------------------------------------



}

#endif // __TCP_SERVER_HPP__
