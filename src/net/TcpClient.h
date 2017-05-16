// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include <boost/noncopyable.hpp>

#include <boost/thread.hpp>
#include <net/TcpConnection.h>
#include <net/Connector.h>
#include <net/Callbacks.h>

typedef boost::shared_ptr<Connector> ConnectorPtr;

class TcpClient : boost::noncopyable
{
public:
	TcpClient(const EventLoopPtr& loop,
		const boost::asio::ip::tcp::endpoint& serverAddr,
		const string& nameArg);
	~TcpClient();  // force out-line dtor, for scoped_ptr members.

	void connect();
	void disconnect();
	void stop();

	TcpConnectionPtr connection() const
	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		return connection_;
	}

	EventLoopPtr getLoop() const { return loop_; }
	bool retry() const { return retry_; }
	void enableRetry() { retry_ = true; }

	const string& name() const
	{ return name_; }

	/// Set connection callback.
	/// Not thread safe.
	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	/// Set message callback.
	/// Not thread safe.
	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	/// Set write complete callback.
	/// Not thread safe.
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

private:
	/// Not thread safe, but in loop
	void newConnection(SocketPtr socketPtr);

	/// Not thread safe, but in loop
	void removeConnection(const TcpConnectionPtr& conn);

	EventLoopPtr loop_;
	ConnectorPtr connector_; // avoid revealing Connector
	const string name_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	bool retry_;   // atomic
	bool connect_; // atomic
	// always in loop thread
	int nextConnId_;
	mutable boost::mutex mutex_;
	TcpConnectionPtr connection_; // @GuardedBy mutex_
};



#endif  // MUDUO_NET_TCPCLIENT_H
