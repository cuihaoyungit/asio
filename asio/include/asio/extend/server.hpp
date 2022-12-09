//
// server.hpp
// add by [11/21/2022 cuihaoyun]
// 

#ifndef __SERVER_HPP__
#define __SERVER_HPP__
#include <set>
#include <asio.hpp>
#include <asio/msgdef/message.hpp>
#include <asio/msgdef/state.hpp>
#include <asio/extend/object.hpp>
#include <asio/detail/socket_types.hpp>
#include <asio/extend/typedef.hpp>
#include <asio/extend/object.hpp>
#include <asio/extend/worker.hpp>

namespace asio {

    using asio::ip::tcp;
    class NetServerEvent;

    //----------------------------------------------------------------------

    class Room
    {
    public:
        void join(NetObjectPtr participant)
        {
            participants_.insert(participant);
            for (const auto& msg : recent_msgs_)
                participant->deliver(msg);
        }

        void leave(NetObjectPtr participant)
        {
            participants_.erase(participant);
        }

        void deliver(const Message& msg)
        {
            recent_msgs_.push_back(msg);
            while (recent_msgs_.size() > max_recent_msgs)
                recent_msgs_.pop_front();

            for (auto participant : participants_)
                participant->deliver(msg);
        }

    private:
        std::set<NetObjectPtr> participants_;
        enum { max_recent_msgs = 100 };
        MessageQueue recent_msgs_;
    };

    //----------------------------------------------------------------------

    class session
        : public NetObject,
        public std::enable_shared_from_this<NetObject>
    {
    public:
        session(tcp::socket socket, Room& room, NetServerEvent* event)
            : socket_(std::move(socket)),
            room_(room),
            net_event_(event)
        {
        }

        void start()
        {
            room_.join(shared_from_this());
            net_event_->Connect(this);
            do_read_header();
        }

        void deliver(const Message& msg)
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
                asio::buffer(read_msg_.data(), Message::header_length),
                [this, self](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec && read_msg_.decode_header())
                    {
                        do_read_body();
                    }
                    else
                    {
                        room_.leave(shared_from_this());
                        net_event_->Disconnect(this);
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
						read_msg_.setObject(shared_from_this());
                        room_.deliver(read_msg_);
                        net_event_->HandleMessage(read_msg_);
                        do_read_header();
                    }
                    else
                    {
                        room_.leave(shared_from_this());
                        net_event_->Disconnect(this);
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
                        room_.leave(shared_from_this());
                        net_event_->Disconnect(this);
                    }
                });
        }

        tcp::socket socket_;
        Room& room_;
        Message read_msg_;
        MessageQueue write_msgs_;
        NetServerEvent* net_event_;
    };

    //----------------------------------------------------------------------

    class Server :public Worker, public NetServerEvent, public Dispatcher
    {
    public:
        Server(const tcp::endpoint& endpoint)
            : acceptor_(io_context, endpoint)
        {
            do_accept();
        }

        ~Server() {}

        void setRouter(const std::shared_ptr<Router>& router) {
            this->router_ = router_;
        }
    public:
		void Connect(NetObject* pObj)    override {}
		void Disconnect(NetObject* pObj) override {}
		void HandleMessage(Message& msg) override {}
    private:
        void Run() override
        {
            io_context.run();
        }
        void do_accept()
        {
            acceptor_.async_accept(
                [this](std::error_code ec, tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::make_shared<session>(std::move(socket), room_, this)->start();
                    }

            do_accept();
                });
        }
		asio::io_context io_context;
        tcp::acceptor acceptor_;
        Room room_;
    protected:
        std::shared_ptr<Router> router_;
    };

    //----------------------------------------------------------------------



}

#endif // __SERVER_HPP__
