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
#include <memory>
#pragma warning(disable : 26495)
#include <asio/msgdef/node.hpp>
#include <asio/extend/object.hpp>

namespace asio {
	class NetObject;
	class Message;
	enum  MsgType
	{
		NET_MSG    = 1,  // 网络消息
		THREAD_MSG = 2   // 线程消息
	};

	typedef struct TPkgHeader
	{
		int seq      = {0};
		int body_len = {0};
		int gate_id  = {0};
		MsgType type = NET_MSG;
	} MsgHeader;

	struct NetPacket {
		int msdId     = {0};
		int connectId = {0};
		Message* buff = {nullptr};
	};

	//
	// Support memory object pool
	//
	class Message : public Node<Message>
	{
	public:
	  static constexpr std::size_t header_length   = sizeof(MsgHeader);
	  static constexpr std::size_t max_body_length = 512;

	  Message(Message& other)
	  {
		  this->id_ = other.id_;
		  this->connect_object_ = other.connect_object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
	  }
	  Message(const Message& other) {
		  this->id_ = other.id_;
		  this->connect_object_ = other.connect_object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
	  }

	  Message& operator=(Message& other) {
		  if (this == &other)
			  return *this;
		  this->id_ = other.id_;
		  this->connect_object_ = other.connect_object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
		  return *this;
	  }
	  Message& operator=(const Message& other) {
		  if (this == &other)
			  return *this;
		  this->id_ = other.id_;
		  this->connect_object_ = other.connect_object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
		  return *this;
	  }

	  Message():body_length_(0), id_(0)
	  {
		  this->clear();
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

	  void setObject(std::weak_ptr<NetObject> obj) {
		  this->connect_object_ = obj;
	  }

	  auto getObject()-> std::weak_ptr<NetObject> {
		  return connect_object_;
	  }

	  void setId(const int id) {
		  this->id_ = id;
	  }

	  int getId() { return id_; }

	  void clear() {
		  std::memset(data_, 0, header_length + max_body_length);
	  }
	private:
	  char data_[header_length + max_body_length];
	  int id_;
	  std::size_t body_length_;
	  std::weak_ptr<NetObject> connect_object_;
	};
}

#endif // __MESSAGE_HPP__
