//
// message.hpp
// add by [11/20/2022 cuihaoyun]
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#pragma warning(disable : 26495)
#include <asio/extend/object.hpp>

namespace asio {

	enum MsgType
	{
		NET_MSG    = 1,  // 网络消息
		THREAD_MSG = 2   // 线程消息
	};

	typedef struct TPkgHeader
	{
		DWORD seq    = {0};
		int body_len = {0};
		MsgType type = NET_MSG;
	} MsgHeader;

	class Message
	{
	public:
	  static constexpr std::size_t header_length   = sizeof(MsgHeader);
	  static constexpr std::size_t max_body_length = 512;

	  Message()
		: body_length_(0), connect_object_(nullptr),id_(0)
	  {
	  }

	  const char* data() const
	  {
		return data_;
	  }

	  char* data()
	  {
		return data_;
	  }

	  std::size_t length() const
	  {
		return header_length + body_length_;
	  }

	  const char* body() const
	  {
		return data_ + header_length;
	  }

	  char* body()
	  {
		return data_ + header_length;
	  }

	  std::size_t body_length() const
	  {
		return body_length_;
	  }

	  void body_length(std::size_t new_length)
	  {
		  body_length_ = new_length;
		  if (body_length_ > max_body_length)
			  body_length_ = max_body_length;
	  }

	  bool decode_header()
	  {
		  MsgHeader* msg = (MsgHeader*)data_;
		  body_length_ = msg->body_len;
		  if (body_length_ > max_body_length)
		  {
			  body_length_ = 0;
			  return false;
		  }
		  return true;
	  }

	  void encode_header(MsgHeader& msg) {
		  std::memcpy(data_, &msg, header_length);
	  }

	  void setObject(std::shared_ptr<NetObject> obj) {
		  this->connect_object_ = obj;
	  }

	  auto getObject()->std::shared_ptr<NetObject> {
		  return connect_object_;
	  }

	  void setId(const int id) {
		  this->id_ = id;
	  }

	  int getId() { return id_; }
	private:
	  char data_[header_length + max_body_length];
	  MsgHeader msg_header_;
	  std::size_t body_length_;
	  std::shared_ptr<NetObject> connect_object_;
	  int id_;
	};
}

#endif // __MESSAGE_HPP__
