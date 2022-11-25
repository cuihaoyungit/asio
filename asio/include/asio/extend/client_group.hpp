//
// client_group.hpp
// add by [11/21/2022 cuihaoyun]
//

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
	
	class NetClientEvent
	{
	public:
		NetClientEvent() = default;
		virtual ~NetClientEvent() {}
		virtual void Connect(NetObject* pObject) {}
		virtual void Disconnect(NetObject* pObject) {}
		virtual void HandleMessage(NetObject* pObject, const message& msg) {}
		virtual void PostMsg(const message& msg) {}
	};

	using asio::ip::tcp;
	class NetClient : public Worker, public NetObject
	{
	public:
		NetClient(NetClientEvent* event, const std::string& address, const int port) ASIO_NOEXCEPT
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
		~NetClient() {
			if (!io_context_.stopped())
			{
				io_context_.stop();
			}
			if (socket_)
			{
				delete socket_;
			}
		}

		uint64_t SocketId() override {
			return socket_->native_handle();
		}

		void disconnect()
		{
			is_close_ = true;
			this->connect_state_ = ConnectState::ST_STOPPING;
			asio::post(io_context_, [this]() {
				socket_->close();
				io_context_.stop();
				this->write_msgs_.clear();
				this->is_connect_ = false;
				this->connect_state_ = ConnectState::ST_STOPPED;
				});
		}

		void Send(const message& msg) override {
			this->write(msg);
		}

	protected:
		void handle_message(NetObject* pObject, const message& msg) {
			this->handle_event_->HandleMessage(this, msg);
		}

		void Run() override
		{
			this->io_context_.run();
		}

		void close()
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
						is_connect_ = true;
						std::cout << "connection succeeded. " << socket_->native_handle() << std::endl;
						this->handle_event_->Connect(this);
						do_read_header();
					}
					else {
						is_connect_ = false;
						std::cout << "connection failed." << std::endl;
						this->close();
					}
				});
			this->connect_state_ = ConnectState::ST_CONNECTING;
		}

		void do_read_header()
		{
			asio::async_read(*socket_,
				asio::buffer(read_msg_.data(), message::header_length),
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
			asio::async_read(*socket_,
				asio::buffer(read_msg_.body(), read_msg_.body_length()),
				[this](std::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
#if _DEBUG
						{
							static std::mutex mtx;
							std::lock_guard lock(mtx);
							std::cout.write(read_msg_.body(), read_msg_.body_length());
							std::cout << "\n";
						}
#else
						std::cout.write(read_msg_.body(), read_msg_.body_length());
						std::cout << std::this_thread::get_id() << "\n";
#endif
						this->handle_message(this, read_msg_);

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
						this->close();
					}
				});
		}

		void write(const message& msg)
		{
			if (!this->is_connect_) {
				return;
			}
			asio::post(io_context_,
				[this, msg]()
				{
					bool write_in_progress = !write_msgs_.empty();
					write_msgs_.push_back(msg);
					if (!write_in_progress && is_connect_)
					{
						do_write();
					}
				});
		}
	private:
		asio::io_context io_context_;
		tcp::socket* socket_;
		message read_msg_;
		message_queue write_msgs_;
		tcp::resolver::results_type endpoints_;
		std::atomic<bool> is_connect_;
		std::atomic<bool> is_close_;
		ConnectState connect_state_;
		NetClientEvent* handle_event_;
	};


	class NetClientWorkGroup : public NetClientEvent
	{
	public:
		NetClientWorkGroup() ASIO_NOEXCEPT {}
		~NetClientWorkGroup() {
			for (const auto& it : net_clients_) {
				delete it;
			}
			net_clients_.clear();
		}

		void Startup(const std::string& address, const int port, int numClients) {
			for (int i = 0; i < numClients; i++) {
				NetClient* pNetClient = new NetClient(this, address, port);
				net_clients_.push_back(pNetClient);
				pNetClient->Startup();
			}
		}

		void WaitStop() {
			for (const auto it : net_clients_) {
				it->disconnect();
				it->WaitStop();
			}
		}

		NetClient* GetNetClient(int index) {
			if (index < net_clients_.size()) {
				return net_clients_[index];
			}
			return nullptr;
		}

		void PostMsg(const message& msg) override
		{
			NetClient* pClient = net_clients_[std::rand() % net_clients_.size()];
			if (pClient)
			{
				pClient->Send(msg);
			}
		}
	private:
		NetClientWorkGroup(const NetClientWorkGroup&) = delete;
		NetClientWorkGroup operator = (const NetClientWorkGroup&) = delete;
	private:
		std::vector<NetClient*> net_clients_;
	};

	using NetClientGroup = NetClientWorkGroup;


}

#endif // __CLIENT_GROUP_HPP__
