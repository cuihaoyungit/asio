#ifndef __CLIENT_GROUP_HPP__
#define __CLIENT_GROUP_HPP__
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <asio.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/extend/worker.hpp>
#include <asio/msgdef/state.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/typedef.hpp>

namespace asio {

	using asio::ip::tcp;
	class NetClientEvent;
	class NetTcpClient : public Worker, public NetObject
	{
	public:
		explicit NetTcpClient(NetClientEvent* event, const std::string& address, const int port) ASIO_NOEXCEPT
			: Worker(),
			handle_event_(event),
			is_connect_(false), is_close_(false), connect_state_(ConnectState::ST_STOPPED), socket_(nullptr)
		{
			tcp::resolver resolver(io_context_);
			auto endpoints = resolver.resolve(address, std::to_string(port));
			this->endpoints_ = endpoints;
			this->connect_state_ = ConnectState::ST_STARTING;
			this->socket_ = new tcp::socket(this->io_context_);
			do_connect(endpoints);
		}
		~NetTcpClient()
		{
			if (!io_context_.stopped())
			{
				io_context_.stop();
			}
			if (socket_)
			{
				delete socket_;
			}
		}

		tcp::socket& Socket()
		{
			return *this->socket_;
		}

		uint64 SocketId() override
		{
			return socket_->native_handle();
		}

		void disconnect()
		{
			is_close_ = true;
			this->connect_state_ = ConnectState::ST_STOPPING;
			asio::post(io_context_, [this]() {
				this->socket_->close();
				this->io_context_.stop();
				this->write_msgs_.clear();
				this->is_connect_ = false;
				this->connect_state_ = ConnectState::ST_STOPPED;
				});
		}
		void Close() override
		{
			this->disconnect();
		}
		// warning error 10009 scope NetObject and socket
		std::string Ip() override
		{
			return this->socket_->remote_endpoint().address().to_string();
		}

		void Send(const Message& msg) override
		{
			this->write(msg);
		}
		void Post(const Message& mgs) override
		{
			this->write(msg);
		}
	protected:
		void handle_message(NetObject* pObject, const Message& msg) {
			this->handle_event_->HandleMessage(this, msg);
		}

		void Run() override
		{
			this->io_context_.run();
		}

		void reset()
		{
			this->connect_state_ = ConnectState::ST_STOPPING;
			asio::post(io_context_, [this]() {
				socket_->close();
				this->handle_event_->Disconnect(this);
				this->write_msgs_.clear();
				this->is_connect_ = false;
				this->connect_state_ = ConnectState::ST_STOPPED;
				this->reconnect();
				});
		}

		void reconnect()
		{
			if (is_close_) {
				return;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "reconnecting" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(10));
			if (!this->is_connect_)
			{
				do_connect(endpoints_);
			}
		}

		void do_connect(const tcp::resolver::results_type& endpoints)
		{
			this->connect_state_ = ConnectState::ST_STARTED;
			asio::async_connect(*socket_, endpoints,
				[this](std::error_code ec, tcp::endpoint)
				{
					static std::mutex mtx;
					std::lock_guard lock(mtx);
					if (!ec)
					{
						this->connect_state_ = ConnectState::ST_CONNECTED;
						this->is_connect_ = true;
						std::cout << "connection succeeded. " << socket_->native_handle() << std::endl;
						this->handle_event_->Connect(this);
						do_read_header();
					}
					else {
						is_connect_ = false;
						std::cout << "connection failed." << std::endl;
						this->reset();
					}
				});
			this->connect_state_ = ConnectState::ST_CONNECTING;
		}

		void do_read_header()
		{
			asio::async_read(*socket_,
				asio::buffer(read_msg_.data(), Message::header_length),
				[this](std::error_code ec, std::size_t /*length*/)
				{
					if (!ec && read_msg_.decode_header())
					{
						do_read_body();
					}
					else
					{
						this->reset();
					}
				});
		}

		void do_read_body()
		{
			asio::async_read(*socket_,
				asio::buffer(read_msg_.body(), read_msg_.body_length()),
				[this](std::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						this->handle_message(this, read_msg_);
						do_read_header();
					}
					else
					{
						this->reset();
					}
				});
		}

		void do_write()
		{
			asio::async_write(*socket_,
				asio::buffer(write_msgs_.front().data(),
					write_msgs_.front().length()),
				[this](std::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						if (!write_msgs_.empty())
						{
							write_msgs_.pop_front();
						}

						if (!write_msgs_.empty())
						{
							do_write();
						}
					}
					else
					{
						this->reset();
					}
				});
		}

		void clear()
		{
			std::lock_guard lock(this->mutex_);
			this->write_msgs_.clear();
		}

		void write(const Message& msg)
		{
			std::lock_guard lock(this->mutex_);
			if (!this->is_connect_) {
				return;
			}
			asio::post(io_context_,
				[this, msg]()
				{
					bool write_in_progress = !write_msgs_.empty();
					this->write_msgs_.push_back(msg);
					if (!write_in_progress && is_connect_)
					{
						this->do_write();
					}
				});
		}
	private:
		asio::io_context io_context_;
		tcp::socket* socket_;
		Message read_msg_;
		MessageQueue write_msgs_;
		tcp::resolver::results_type endpoints_;
		std::atomic<bool> is_connect_;
		std::atomic<bool> is_close_;
		ConnectState connect_state_;
		NetClientEvent* handle_event_;
		std::mutex mutex_;
	};


	class NetClientWorkGroup : public NetClientEvent
	{
	public:
		NetClientWorkGroup() ASIO_NOEXCEPT {}
		~NetClientWorkGroup()
		{
			for (const auto& it : net_clients_) {
				delete it;
			}
			net_clients_.clear();
		}

		void Startup(const std::string& address, const int port, int numClients)
		{
			for (int i = 0; i < numClients; i++) {
				NetTcpClient* pNetClient = new NetTcpClient(this, address, port);
				net_clients_.push_back(pNetClient);
				pNetClient->Startup();
			}
		}

		void WaitStop()
		{
			for (const auto it : net_clients_) {
				it->disconnect();
				it->WaitStop();
			}
		}

		NetTcpClient* GetNetTcpClient(int index)
		{
			if (index < net_clients_.size()) {
				return net_clients_[index];
			}
			return nullptr;
		}

		void PostMsg(const Message& msg) override
		{
			NetTcpClient* pClient = net_clients_[std::rand() % net_clients_.size()];
			if (pClient)
			{
				pClient->Send(msg);
			}
		}

		void Init() override
		{

		}

		void Exit() override
		{

		}

	private:
		NetClientWorkGroup(const NetClientWorkGroup&) = delete;
		NetClientWorkGroup operator = (const NetClientWorkGroup&) = delete;
	private:
		std::vector<NetTcpClient*> net_clients_;
	};

	using NetClientGroup = NetClientWorkGroup;


}

#endif // __CLIENT_GROUP_HPP__
