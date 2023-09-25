#ifndef __TCP_CLIENT_HPP__
#define __TCP_CLIENT_HPP__
#include <deque>
#include <mutex>
#include <asio.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/msgdef/state.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/typedef.hpp>
#include <asio/extend/worker.hpp>

namespace asio {

    using asio::ip::tcp;
    class NetObject;
    // Single TcpClient
    //--------------------------------------------------------------
    class TcpClient
        : public NetObject
        , public std::enable_shared_from_this<TcpClient>
    {
    public:
        TcpClient(asio::io_context& io_context, NetEvent* event, const std::string &ip, const std::string &port)
            : io_context_(io_context)
            , socket_(io_context)
            , auto_reconnect_(false)
            , connect_state_(ConnectState::ST_STOPPED)
            , net_event_(event)
        {
            this->SetConnect(false);
            this->connect_state_ = ConnectState::ST_STARTING;
            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(ip, port);
            this->endpoints_ = endpoints;
            this->host_ = ip;
            this->port_ = port;
            this->do_connect(endpoints);
        }

        virtual ~TcpClient()
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
        std::string Ip()   override
        {
            return this->socket_.remote_endpoint().address().to_string();
        }

        std::string Port() override
        {
            return this->port_;
        }

        void Shutdown()
        {
            this->Close();
            this->Stop();
        }
        
        void SetAutoReconnect(bool bAutoReconnect) 
        {
            this->auto_reconnect_ = bAutoReconnect;
        }
    public:
		void Close() override
		{
            this->SetAutoReconnect(false);
            this->connect_state_ = ConnectState::ST_STOPPING;
            asio::post(io_context_, [this]() 
                {
                    this->socket_.close();
                    this->connect_state_ = ConnectState::ST_STOPPED;
                });
		}
    protected:
        void Stop()
        {
            if (!this->io_context_.stopped())
            {
                this->io_context_.stop();
            }
        }
	private:
        void clear() // need lock ? 2023-08-10
        {
            std::lock_guard lock(this->mutex_);
            this->write_msgs_.clear();
        }
        void write(const Message& msg) // async write keep sequence with lock
        {
            std::lock_guard lock(this->mutex_);
            if (!this->IsConnect()) {
                return;
            }
            asio::post(io_context_,
                [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                this->write_msgs_.push_back(msg);
                if (!write_in_progress && this->IsConnect())
                {
                    this->do_write();
                }
                });
        }

		void disconnect()
		{
            this->SetAutoReconnect(false);
			this->connect_state_ = ConnectState::ST_STOPPING;
			asio::post(io_context_, [this]() {
				this->socket_.close();
			    this->io_context_.stop();
			    this->clear();
                this->SetConnect(false);
			    this->connect_state_ = ConnectState::ST_STOPPED;
				});
		}

        void reset()
        {
            this->connect_state_ = ConnectState::ST_STOPPING;
            if (!this->auto_reconnect_)
            {
                return;
            }
            // reconnect
            asio::post(io_context_, [this]() {
                this->socket_.close();
                this->clear();
                this->SetConnect(false);
                this->connect_state_ = ConnectState::ST_STOPPED;
                this->reconnect();
                });
        }

    public:
        void reconnect()
        {
            if (!auto_reconnect_)
            {
                return;
            }
            std::cout << this->GetConnectName() <<":"<<"reconnecting" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (!this->IsConnect())
            {
                this->net_event_->Reconnect(dynamic_cast<NetObject*>(this));
                this->do_connect(endpoints_);
            }
        }
    private:
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
                        std::cout << this->GetConnectName() << ":" << "connection succeeded." << std::endl;
                        this->net_event_->Connect(dynamic_cast<NetObject*>(this));
                        this->read_msg_.setNetObject(std::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
                        this->do_read_header();
                    }
                    else {
                        this->SetConnect(false);
                        this->net_event_->Disconnect(dynamic_cast<NetObject*>(this));
                        std::cout << this->GetConnectName() << ":" << "connection failed." << std::endl;
                        this->reset();
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
                        this->do_read_body();
                    }
                    else
                    {
                        this->net_event_->Disconnect(dynamic_cast<NetObject*>(this));
                        this->reset();
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
                        this->net_event_->HandleMessage(dynamic_cast<NetObject*>(this), read_msg_);
                        this->do_read_header();
                    }
                    else
                    {
                        this->net_event_->Disconnect(dynamic_cast<NetObject*>(this));
                        this->reset();
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
                            this->write_msgs_.pop_front();
                        }

                        if (!write_msgs_.empty()) {
                            this->do_write();
                        }
                    }
                    else
                    {
                        this->net_event_->Disconnect(dynamic_cast<NetObject*>(this));
                        this->reset();
                    }
                });
        }

    private:
        asio::io_context& io_context_;
        tcp::socket socket_;
        Message read_msg_;
        MessageQueue write_msgs_;
        tcp::resolver::results_type endpoints_;
        bool auto_reconnect_;
        ConnectState connect_state_;
		std::mutex mutex_;
        NetEvent* net_event_;
        std::string host_;
        std::string port_;
    };
	typedef std::shared_ptr<TcpClient> TcpClientPtr;
    //////////////////////////////////////////////////////////////////////////


    class TcpClientWorker : public Worker, public NetEvent
    {
    public:
        TcpClientWorker() {}
        virtual ~TcpClientWorker() {}
        void Stop()
        {
            this->SetAutoReconnect(false);
            if (this->tc_) {
                this->tc_->Close();
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
            if (this->tc_)
            {
                this->tc_->Post(msg);
            }
        }
        void Send(const asio::Message& msg) override
        {
            if (this->tc_)
            {
                this->tc_->Post(msg);
            }
        }
    protected:
		void SetName(const std::string_view& name)
		{
			this->name_ = name;
		}
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
                asio::io_context io_context;
                /*
                asio::signal_set signal(io_context);
                // ensure one signals handler in application
                signal.add(SIGINT); // ctrl + c
                signal.add(SIGTERM);// kill process
#if defined(SIGQUIT)
                signal.add(SIGQUIT);// posix linux
#endif // defined(SIGQUIT)
                // signals
                signal.async_wait(
                	[this](std::error_code ec, int signo)
                	{
                		// The server is stopped by cancelling all outstanding asynchronous
                		// operations. Once all operations have finished the io_context::run()
                		// call will exit.
                        this->Stop();
                	});
                */
                if (this->name_.empty()) 
                {
                    this->name_ = typeid(TcpClientWorker).name(); 
                }
                // tcp client connect
                auto tc = std::make_shared<TcpClient>(io_context, this, this->host_, this->port_);
                this->tc_ = tc;
                tc->SetConnectName(this->name_);
                tc->SetAutoReconnect(false);
                io_context.run();
                this->tc_ = nullptr;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            } while (this->auto_reconnect_);
        }
    private:
        bool auto_reconnect_ = { false };
        TcpClientPtr tc_;
        std::string name_;
        std::string host_;
        std::string port_;
    };



}


#endif // __TCP_CLIENT_HPP__
