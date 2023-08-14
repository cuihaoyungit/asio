#ifndef __WEBSOCKET_ROOM_HPP__
#define __WEBSOCKET_ROOM_HPP__
#include <boost/smart_ptr.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

// Forward declaration
namespace asio {
    class NetObject;
}
// using namespace asio;

// Represents the shared server state
class WebSocketRoom
{
    std::string const doc_root_;

    // This mutex synchronizes all access to sessions_
    std::mutex mutex_;

    // Keep a list of all the connected clients
    std::unordered_set<asio::NetObject*> sessions_;

public:
    explicit WebSocketRoom() {}

    std::string const&
        doc_root() const noexcept
    {
        return doc_root_;
    }

    void join(asio::NetObject* session)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.insert(session);
    }
    void leave(asio::NetObject* session)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(session);
    }
    void send(asio::Message& message)
    {
        // Put the message in a shared pointer so we can re-use it for each client
        // auto const ss = std::make_shared<std::string const>(std::move(message));

        // Make a local list of all the weak pointers representing
        // the sessions, so we can do the actual sending without
        // holding the mutex:
        std::vector<std::weak_ptr<asio::NetObject>> v;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            v.reserve(sessions_.size());
            for (auto p : sessions_)
                v.emplace_back(p->weak_from_this());
        }

        // For each session in our local list, try to acquire a strong
        // pointer. If successful, then send the message on that session.
        for (auto const& wp : v)
            if (auto sp = wp.lock())
                sp->Send(message);
    }
};




#endif // __WEBSOCKET_ROOM_HPP__
