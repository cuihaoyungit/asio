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

namespace asio {
	class ConnectObject;

	typedef struct TPkgHeader
	{
		DWORD seq;
		int body_len;
	} MsgHeader;

	class message
	{
	public:
	  static constexpr std::size_t header_length   = sizeof(MsgHeader);
	  static constexpr std::size_t max_body_length = 512;

	  message()
		: body_length_(0), connect_object_(nullptr)
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

	  void setObject(ConnectObject *obj) {
		  this->connect_object_ = obj;
	  }

	  ConnectObject* getObject() {
		  return connect_object_;
	  }
	private:
	  char data_[header_length + max_body_length];
	  MsgHeader msg_header_;
	  std::size_t body_length_;
	  ConnectObject* connect_object_;
	};
}

#endif // __MESSAGE_HPP__
