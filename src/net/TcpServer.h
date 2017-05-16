// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <net/TcpConnection.h>
#include <net/Acceptor.h>
#include <base/EventLoopThreadPool.h>


#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoopWeakPtr)> ThreadInitCallback;
	enum Option
	{
		kNoReusePort,
		kReusePort,
	};

	//TcpServer(EventLoop* loop, const InetAddress& listenAddr);
	TcpServer(const EventLoopPtr& loop,
		const boost::asio::ip::tcp::endpoint& listenAddr,
		const string& nameArg,
		Option option = kNoReusePort);
	~TcpServer();  // force out-line dtor, for scoped_ptr members.

	const string& ipPort() const { return ipPort_; }
	const string& name() const { return name_; }
	EventLoopPtr getLoop() const { return loop_; }

	/// Set the number of threads for handling input.
	///
	/// Always accepts new connection in loop's thread.
	/// Must be called before @c start
	/// @param numThreads
	/// - 0 means all I/O in loop's thread, no thread will created.
	///   this is the default value.
	/// - 1 means all I/O in another thread.
	/// - N means a thread pool with N threads, new connections
	///   are assigned on a round-robin basis.
	void setThreadNum(int numThreads);
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadInitCallback_ = cb; }
	/// valid after calling start()
	boost::shared_ptr<EventLoopThreadPool> threadPool()
	{ return threadPool_; }

	/// Starts the server if it's not listenning.
	///
	/// It's harmless to call it multiple times.
	/// Thread safe.
	//应该在baseLoop所在的线程调用
	void start();

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
	void newConnection(SocketPtr sock, EventLoopPtr sockBindedLoop);
	/// Thread safe.
	void removeConnection(const TcpConnectionPtr& conn);
	/// Not thread safe, but in loop
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	typedef std::map<string, TcpConnectionPtr> ConnectionMap;

	EventLoopPtr loop_;  // the acceptor loop
	const string ipPort_;
	const string name_;
	boost::shared_ptr<EventLoopThreadPool> threadPool_;
	boost::scoped_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	ThreadInitCallback threadInitCallback_;
	// always in loop thread
	int nextConnId_;
	ConnectionMap connections_;
};

#endif  // MUDUO_NET_TCPSERVER_H
