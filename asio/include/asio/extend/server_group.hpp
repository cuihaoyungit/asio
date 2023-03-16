//
// server_group.hpp
// add by [11/21/2022 cuihaoyun]
//

#ifndef __SERVER_GROUP_HPP__
#define __SERVER_GROUP_HPP__

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <functional>
#include <mutex>
#include <asio.hpp>
#include <queue>
#include <set>
#include <mutex>
#include <asio/detail/socket_types.hpp>
#include <asio/extend/worker.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/typedef.hpp>
#include <asio/extend/room.hpp>
#include <asio/signal_set.hpp>

namespace asio {

	using asio::ip::tcp;
	class NetServerEvent;
	class Room;

	//----------------------------------------------------------------------
	// Session GroupServer
	class Session
		: public NetObject,
		public std::enable_shared_from_this<NetObject>
	{
	public:
		Session(tcp::socket socket, Room& room, NetServerEvent* event) noexcept
			: socket_(std::move(socket)),
			room_(room),
			net_event_(event)
		{

		}

		virtual ~Session() {
			this->Final();
		}

		void Start()
		{
			room_.Join(shared_from_this());
			net_event_->Connect(shared_from_this());
			this->SetConnect(true);
			do_read_header();
		}

		void Deliver(const Message& msg) override
		{
			std::lock_guard lock(this->mutex_);
			bool write_in_progress = !write_msgs_.empty();
			write_msgs_.push_back(std::move(msg));
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

		uint64_t SocketId() override final
		{
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
						net_event_->Disconnect(shared_from_this());
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
						read_msg_.setNetId(this->SocketId());
						read_msg_.setObject(shared_from_this());
						//room_.deliver(read_msg_);
						net_event_->PostMsg(read_msg_);
						do_read_header();
					}
					else
					{
						room_.Leave(shared_from_this());
						net_event_->Disconnect(shared_from_this());
						this->SetConnect(false);
					}
				});
		}

		void do_write()
		{
			auto self(shared_from_this());
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
						net_event_->Disconnect(shared_from_this());
						this->SetConnect(false);
					}
				});
		}

		void Final() override
		{

		}

		tcp::socket socket_;
		Room& room_;
		Message read_msg_;
		MessageQueue write_msgs_;
		NetServerEvent* net_event_;
		std::mutex mutex_;
	};

	//----------------------------------------------------------------------
	// NetServer
	class NetServer : public Worker
	{
		friend class NetServerWorkGroup;
	public:
		NetServer(NetServerEvent* handleMessage, const tcp::endpoint& endpoint)
			: Worker(),
			handle_message_(handleMessage),
			port_(0),
			stoped_(false),
			acceptor_(io_context_, endpoint),
			signals_(io_context_)
		{
			signals_.add(SIGINT);
			signals_.add(SIGTERM);
#if defined(SIGQUIT)
			signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
			acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			signals_.async_wait(
				[this](std::error_code /*ec*/, int /*signo*/)
				{
					// The server is stopped by cancelling all outstanding asynchronous
					// operations. Once all operations have finished the io_context::run()
					// call will exit.
					this->acceptor_.close();
					this->StopContext();
				});
			this->do_accept();
		}
		~NetServer() {}

		void Init() override
		{

		}

		void Run() override {
			io_context_.run();
		}

		void Exit() override
		{

		}

		void StopContext() {
			this->Shutdown();
		}

		void Shutdown() {
			if (!io_context_.stopped())
			{
				io_context_.stop();
				stoped_ = true;
			}
		}

		void SetPort(int port)
		{
			port_ = port;
		}
	private:
		void do_accept()
		{
			acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_.async_accept(
				[this](std::error_code ec, tcp::socket socket)
				{
					if (!ec)
					{
						std::make_shared<Session>(std::move(socket), room_, handle_message_)->Start();
					}

					do_accept();
				});
		}

		int port_;
		NetServerEvent* handle_message_;
		asio::io_context io_context_;
		tcp::acceptor acceptor_;
		asio::signal_set signals_;
	protected:
		Room room_;
		bool stoped_;
	};

	//----------------------------------------------------------------------
	// NetServerWorkGroup
	class NetServerWorkGroup : public NetServerEvent
	{
	public:
		NetServerWorkGroup() : m_stop(false) {}
		~NetServerWorkGroup() {
			for (const auto& it : m_vNetServers) {
				delete it;
			}
			m_vNetServers.clear();

			// notify stop thread
			{
				std::unique_lock<std::mutex> lock(m_queue_mutex);
				m_stop = true;
			}
			m_condition.notify_all();
			
			// waiting for thread stop
			for (auto& worker : m_workers)
				worker.join();
		}

		void Startup(const std::vector<unsigned short>& vPorts, int threadWorks = 1) {
			// server port
			this->Init();
			for (auto port : vPorts)
			{
				NetServer* pNetServer = new NetServer(this, tcp::endpoint(tcp::v4(), port));
				pNetServer->SetPort(port);
				m_vNetServers.push_back(pNetServer);
				pNetServer->Startup();
			}

			// thread pool works
			for (size_t i = 0; i < threadWorks; ++i) {
				m_workers.emplace_back(
					[this]
					{
						for (;;)
						{
							std::unique_lock<std::mutex> lock(this->m_queue_mutex);
							this->m_condition.wait(lock,
								[this] { return this->m_stop || !this->m_msgQueue.empty(); });
							if (this->m_stop && this->m_msgQueue.empty())
								return;
							auto msg = std::move(this->m_msgQueue.front());
							this->m_msgQueue.pop();
							this->HandleMessage(msg);
						}
					});
			}
		}

		void WaitStop() {
			for (const auto it : m_vNetServers) {
				it->StopContext();
				it->WaitStop();
			}
			this->Exit();
		}

		auto GetNetServer(int index) -> NetServer* {
			if (index < m_vNetServers.size()) {
				return m_vNetServers[index];
			}
			return nullptr;
		}

		auto RandomNetServer()
		{
			return m_vNetServers[rand() % m_vNetServers.size()];
		}

		void PostMsg(const Message& msg) override {
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			if (m_stop) {
				return;
			}
			m_msgQueue.emplace(msg);
			m_condition.notify_one();
		}

	private:
		void Init() override
		{
		
		}
		
		void Exit() override
		{
		
		}

		NetServerWorkGroup(const NetServerWorkGroup&) = delete;
		NetServerWorkGroup operator = (const NetServerWorkGroup&) = delete;
	private:
		typedef std::vector<NetServer*>  ServerList;
		ServerList m_vNetServers;

		typedef std::vector<std::thread> ThreadList;
		ThreadList m_workers;

		typedef std::queue<Message>      MsgQueue;
		MsgQueue   m_msgQueue;

		// synchronization
		std::mutex m_queue_mutex;
		std::condition_variable m_condition;
		bool m_stop;
	};

	using NetServerGroup = NetServerWorkGroup;

}


#endif // __SERVER_GROUP_HPP__
