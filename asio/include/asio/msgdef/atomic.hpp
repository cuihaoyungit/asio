//
// atomic.hpp
// add by [12/13/2022 cuihaoyun]
//

#ifndef __ATOMIC_HPP__
#define __ATOMIC_HPP__
#include <queue>
#include <mutex>

namespace asio {
	class Message;
	typedef struct _AtomicData
	{
		std::queue<Message*> msg_queue;
		std::mutex queue_mutex;
		std::condition_variable condition;
		bool stop = { false };
	}AtomicData;

}


#endif // __ATOMIC_HPP__
