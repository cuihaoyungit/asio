//
// server.hpp
// add by [11/21/2022 cuihaoyun]
//

#ifndef __SERVER_HPP__
#define __SERVER_HPP__

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
#include <asio/extend/worker.hpp>
#include <asio/msgdef/message.hpp>

namespace asio {

	class NetServerEvent
	{
	public:
		NetServerEvent() = default;
		virtual ~NetServerEvent() {}
		virtual void Connect() {}
		virtual void Disconnect() {}
		virtual void HandleMessage(const message& msg) {}
		virtual void PostMsg(const message& msg) {}
	};

	using asio::ip::tcp;
	typedef std::deque<message> message_queue;

	//----------------------------------------------------------------------

	class chat_participant
	{
	public:
		virtual ~chat_participant() {}
		virtual void deliver(const message& msg) = 0;
	};

	typedef std::shared_ptr<chat_participant> chat_participant_ptr;

	//----------------------------------------------------------------------

	class chat_room
	{
	public:
		void join(chat_participant_ptr participant)
		{
			std::lock_guard lock(mutex_);
			participants_.insert(participant);
			for (const auto& msg : recent_msgs_)
				participant->deliver(msg);
		}

		void leave(chat_participant_ptr participant)
		{
			std::lock_guard lock(mutex_);
			participants_.erase(participant);
		}

		void deliver(const message& msg)
		{
			recent_msgs_.push_back(msg);
			while (recent_msgs_.size() > max_recent_msgs)
				recent_msgs_.pop_front();

			for (auto& participant : participants_)
				participant->deliver(msg);
		}

	private:
		std::mutex mutex_;
		std::set<chat_participant_ptr> participants_;
		enum { max_recent_msgs = 100 };
		message_queue recent_msgs_;
	};

	//----------------------------------------------------------------------

	class session
		: public chat_participant,
		public std::enable_shared_from_this<session>
	{
	public:
		session(tcp::socket socket, chat_room& room, NetServerEvent* event)
			: socket_(std::move(socket)),
			room_(room),
			net_event_(event)
		{

		}

		void start()
		{
			room_.join(shared_from_this());
			do_read_header();
		}

		void deliver(const message& msg)
		{
			bool write_in_progress = !write_msgs_.empty();
			write_msgs_.push_back(msg);
			if (!write_in_progress)
			{
				do_write();
			}
		}

	private:
		void do_read_header()
		{
			auto self(shared_from_this());
			asio::async_read(socket_,
				asio::buffer(read_msg_.data(), message::header_length),
				[this, self](std::error_code ec, std::size_t /*length*/)
				{
					if (!ec && read_msg_.decode_header())
					{
						do_read_body();
					}
					else
					{
						room_.leave(shared_from_this());
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
						room_.deliver(read_msg_);
						net_event_->PostMsg(read_msg_);
						do_read_header();
					}
					else
					{
						room_.leave(shared_from_this());
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
						room_.leave(shared_from_this());
					}
				});
		}

		tcp::socket socket_;
		chat_room& room_;
		message read_msg_;
		message_queue write_msgs_;
		NetServerEvent* net_event_;
	};

	//----------------------------------------------------------------------

	class NetServer : public Worker
	{
		friend class NetServerWorkGroup;
	public:
		NetServer(NetServerEvent* handleMessage, const tcp::endpoint& endpoint)
			: Worker(),
			handle_message_(handleMessage),
			port_(0),
			acceptor_(io_context_, endpoint)
		{
			this->do_accept();
		}
		~NetServer() { }

		void Run() override {
			io_context_.run();
		}

		void Stop() {
			if (!io_context_.stopped())
			{
				io_context_.stop();
			}
		}

		void SetPort(int port)
		{
			port_ = port;
		}
	private:
		void do_accept()
		{
			acceptor_.async_accept(
				[this](std::error_code ec, tcp::socket socket)
				{
					if (!ec)
					{
						std::make_shared<session>(std::move(socket), room_, handle_message_)->start();
					}

					do_accept();
				});
		}

		int port_;
		NetServerEvent* handle_message_;
		asio::io_context io_context_;
		tcp::acceptor acceptor_;
		chat_room room_;
	};

	//----------------------------------------------------------------------

	class NetServerWorkGroup : public NetServerEvent
	{
	public:
		NetServerWorkGroup() : m_stop(false) {}
		~NetServerWorkGroup() {
			for (const auto& it : m_vNetServers) {
				delete it;
			}
			m_vNetServers.clear();

			{
				std::unique_lock<std::mutex> lock(m_queue_mutex);
				m_stop = true;
			}
			m_condition.notify_all();
			for (std::thread& worker : m_workers)
				worker.join();
		}

		void Startup(const std::vector<int>& vPorts, int threadWorks = 1) {
			// server port
			for (auto it : vPorts)
			{
				NetServer* pNetServer = new NetServer(this, tcp::endpoint(tcp::v4(), it));
				pNetServer->SetPort(it);
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
						}
					});
			}
		}

		void WaitStop() {
			for (const auto it : m_vNetServers) {
				it->Stop();
				it->WaitStop();
			}
		}

		NetServer* GetNetClient(int index) {
			if (index < m_vNetServers.size()) {
				return m_vNetServers[index];
			}
			return nullptr;
		}

		void PostMsg(const message& msg) override {
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			if (m_stop) {
				return;
			}
			m_msgQueue.emplace(msg);
			m_condition.notify_one();
		}

	private:
		void HandleMessage(const message& msg) override
		{
			static std::mutex mtx;
			std::lock_guard lock(mtx);
			std::ostringstream oss;
			oss << std::this_thread::get_id();
			std::string sspid = oss.str();
			unsigned long long pid = std::stoull(sspid);
			std::cout << sspid << "\n";
		}

		void Connect() override
		{

		}

		void Disconnect() override
		{

		}
		NetServerWorkGroup(const NetServerWorkGroup&) = delete;
		NetServerWorkGroup operator = (const NetServerWorkGroup&) = delete;
	private:
		std::vector<NetServer*>  m_vNetServers;

		std::vector<std::thread> m_workers;
		std::queue<message> m_msgQueue;
		// synchronization
		std::mutex m_queue_mutex;
		std::condition_variable m_condition;
		bool m_stop;
	};

	using NetServerGroup = NetServerWorkGroup;

}









#endif // __SERVER_HPP__
