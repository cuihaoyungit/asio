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
		Session(tcp::socket socket, Room& room, NetServer* server) noexcept
			: socket_(std::move(socket)),
			room_(room),
			server_(server)
		{

		}

		virtual ~Session() {
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
			room_.Join(shared_from_this());
			read_msg_.setNetObject(shared_from_this());
			// connect event
			this->SetConnect(true);
			server_->Connect(shared_from_this());
			// start receive stream data
			do_read_header();
		}

		void Deliver(const Message& msg) override
		{
			std::lock_guard lock(this->mutex_);
			bool write_in_progress = !write_msgs_.empty();
			this->write_msgs_.push_back(msg);
			if (!write_in_progress)
			{
				do_write();
			}
		}

		void Send(const Message& msg)    override
		{
			this->Deliver(msg);
		}

		void Post(const Message& msg)    override
		{
			this->Deliver(msg);
		}

		// warning error 10009 scope NetObject socket
		std::string Ip() override
		{
			return this->socket_.remote_endpoint().address().to_string();
		}

		uint64 SocketId() override final
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
						if (server_->IsPackSessionId())
						{
							MsgHeader* header = (MsgHeader*)(this->read_msg_.data());
							header->sessionId = this->getSessionId();
						}
						do_read_body();
					}
					else
					{
						this->server_->Disconnect(shared_from_this());
						this->room_.Leave(shared_from_this());
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
						server_->PostMsg(read_msg_);
						do_read_header();
					}
					else
					{
						this->server_->Disconnect(shared_from_this());
						this->room_.Leave(shared_from_this());
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
						this->server_->Disconnect(shared_from_this());
						this->room_.Leave(shared_from_this());
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
		NetServer* server_;
		std::mutex mutex_;
	};

	//----------------------------------------------------------------------
	// SubServer
	class SubServer : public Worker
	{
		friend class NetServerWorkGroup;
	public:
		SubServer(NetServer* server, const tcp::endpoint& endpoint)
			: Worker(),
			server_(server),
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
				[this](std::error_code /*ec*/, int /*signal*/)
				{
					// The server is stopped by cancelling all outstanding asynchronous
					// operations. Once all operations have finished the io_context::run()
					// call will exit.
					this->acceptor_.close();
					this->StopContext();
				});
			this->do_accept();
		}
		~SubServer() {}

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

		Room& getRoom() {
			return this->room_;
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
						std::make_shared<Session>(std::move(socket), room_, server_)->Start();
					}

					do_accept();
				});
		}
	private:

		void AfterInit() override
		{

		}

		void Init() override
		{

		}

		void BeforeExit() override
		{

		}

		void Exit() override
		{

		}

		// single thread run
		void Run() override {
			io_context_.run();
		}

	private:
		int port_;
		NetServer* server_;
		// io_service
		asio::io_context io_context_;
		tcp::acceptor acceptor_;
		asio::signal_set signals_;
		Room room_;
		bool stoped_;
	};

	//----------------------------------------------------------------------
	// NetServerWorkGroup
	class NetGroupServer : public NetServer
	{
	public:
		NetGroupServer() : m_stop(false) {}
		virtual ~NetGroupServer() {
			for (const auto& it : m_vSubServers) {
				delete it;
			}
			m_vSubServers.clear();

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

		void Startup(const std::vector<int>& vPorts, int threadWorks = 1) {
			// server port
			this->Init();
			for (auto port : vPorts)
			{
				SubServer* pSubServer = new SubServer(dynamic_cast<NetServer*>(this), tcp::endpoint(tcp::v4(), port));
				pSubServer->SetPort(port);
				m_vSubServers.push_back(pSubServer);
				pSubServer->Startup();
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

		void StopContext()
		{
			for (const auto it : m_vSubServers) {
				it->StopContext();
			}
		}

		void WaitStop() {
			for (const auto it : m_vSubServers) {
				it->WaitStop();
			}
			this->Exit();
		}

		auto GetServer(int index) -> SubServer* {
			if (index < m_vSubServers.size()) {
				return m_vSubServers[index];
			}
			return nullptr;
		}

		auto RandomNetServer()
		{
			return m_vSubServers[rand() % m_vSubServers.size()];
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
			int serverId = this->ServerId() == 0 ? 2 : this->ServerId();
			int subId = this->ServerSubId() == 0 ? 1 : this->ServerSubId();
			this->InitUUID(serverId, subId);
		}
		
		void Exit() override
		{
		
		}

		NetGroupServer(const NetGroupServer&) = delete;
		const NetGroupServer operator = (const NetGroupServer&) = delete;
	private:
		typedef std::vector<SubServer*>  ServerList;
		ServerList m_vSubServers;

		typedef std::vector<std::thread> ThreadList;
		ThreadList m_workers;

		typedef std::queue<Message>      MsgQueue;
		MsgQueue   m_msgQueue;

		// synchronization
		std::mutex m_queue_mutex;
		std::condition_variable m_condition;
		bool m_stop;
	};


}


#endif // __SERVER_GROUP_HPP__
