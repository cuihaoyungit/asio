//
// typedef.hpp
// add by [11/25/2022 cuihaoyun]
// 

#ifndef __TYPEDEF_HPP__
#define __TYPEDEF_HPP__
#include <deque>
#include <asio/msgdef/message.hpp>

namespace asio {

	class Message;
	typedef std::deque<Message> MessageQueue;







}



#endif // __TYPEDEF_HPP__
