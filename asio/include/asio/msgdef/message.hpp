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
#include <asio/extend/base.hpp>

namespace asio {
	class NetObject;
	class Message;
	typedef std::shared_ptr<NetObject> NetObjectPtr;
	typedef std::weak_ptr<NetObject>   NetObjectWeakPtr;

#pragma pack(push, 1)
	enum ProtoFormat
	{
		eProtoBuf = 0, // Protobuf
		eBinary   = 1, // Binary
		eQtStream = 2, // QtStream
		eByteBuff = 3  // ByteBuff
	};
#pragma pack(pop)

	// TPkgHeader
#pragma pack(push, 4)
	typedef struct _TPkgHeader
	{
		int msgId          = 0;
		int body_len       = 0;
		uint64 sId		   = 0; // session id
		int crc            = 0;
		uint8 format       = ProtoFormat::eProtoBuf;
		uint8 gateId       = 0;
		uint8 crypto       = 0;
		uint8 appId        = 0;
	} MsgHeader;
#pragma pack(pop)

	// NetPacket
	struct NetPacket {
		int length    = {0};
		Message* buff = {nullptr};
	};

	// Message Support memory object pool
	class Message : public Node<Message>
	{
	public:
	  static constexpr int header_length   = sizeof(MsgHeader);
	  static constexpr int max_body_length = 4096;

	  Message(Message& other)
	  {
		  this->object_ = other.object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
	  }
	  Message(const Message& other) 
	  {
		  this->object_ = other.object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
	  }

	  Message& operator=(Message& other)
	  {
		  if (this == &other)
			  return *this;
		  this->object_ = other.object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
		  return *this;
	  }
	  const Message& operator=(const Message& other)
	  {
		  if (this == &other)
			  return *this;
		  this->object_ = other.object_;
		  std::memcpy(data_, other.data(), other.length());
		  this->body_length_ = other.body_length_;
		  return *this;
	  }

	  Message():body_length_(0)
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

	  int length() const
	  {
		return header_length + body_length_;
	  }

	  int max_length()
	  {
		  return header_length + max_body_length;
	  }

	  const char* body() const
	  {
		return data_ + header_length;
	  }

	  char* body()
	  {
		return data_ + header_length;
	  }

	  int body_length() const
	  {
		return body_length_;
	  }

	  void body_length(int new_length)
	  {
		  body_length_ = new_length;
		  if (body_length_ > max_body_length)  // data length too long
		  {
			  body_length_ = max_body_length;
		  }
	  }

	  bool decode_header() {
		  MsgHeader* msg = (MsgHeader*)data_;
		  body_length_ = msg->body_len;
		  if (body_length_ > max_body_length)  // range out
		  {
			  body_length_ = 0;
			  return false;
		  }
		  return true;
	  }

	  void encode_header(MsgHeader& msg) 
	  {
		  std::memcpy(data_, &msg, header_length);
	  }

	  void setNetObject(NetObjectPtr obj)
	  {
		  this->object_ = obj;
	  }

	  auto getNetObject() const
	  {
		  return object_;
	  }

	  void clear()
	  {
		  std::memset(data_, 0, header_length + max_body_length);
		  this->body_length_ = 0;
	  }
	private:
	  char data_[header_length + max_body_length];
	  int body_length_;
	  NetObjectWeakPtr object_;
	};
}

#endif // __MESSAGE_HPP__
