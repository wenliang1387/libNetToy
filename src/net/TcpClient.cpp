// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <base/Logger.h>
#include <net/TcpClient.h>


#include <boost/bind.hpp>

#include <stdio.h>  // snprintf


// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

void removeConnection(EventLoopPtr loop, const TcpConnectionPtr& conn)
{
	loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn.get()));
}

void removeConnector(const ConnectorPtr& connector)
{
	//connector->
}

TcpClient::TcpClient(const EventLoopPtr& loop,
					 const boost::asio::ip::tcp::endpoint& serverAddr,
					 const string& nameArg)
					 : loop_(loop),
					 connector_(new Connector(loop, serverAddr)),
					 name_(nameArg),
					 connectionCallback_(defaultConnectionCallback),
					 messageCallback_(defaultMessageCallback),
					 retry_(false),
					 connect_(true),
					 nextConnId_(1)
{
	connector_->setNewConnectionCallback(
		boost::bind(&TcpClient::newConnection, this, _1));
	// FIXME setConnectFailedCallback
	LogInfo(0, "TcpClient::TcpClient[" << name_<< "] - connector " << get_pointer(connector_));
}

TcpClient::~TcpClient()
{
	LogInfo(0, "TcpClient::~TcpClient[" << name_<< "] - connector " << get_pointer(connector_));
	TcpConnectionPtr conn;
	bool unique = false;
	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		unique = connection_.unique();
		conn = connection_;
	}
	if (conn){
		assert(loop_ == conn->getLoop());
		// FIXME: not 100% safe, if we are in different thread
		CloseCallback cb = boost::bind(&TcpClient::removeConnection, this, _1);
		loop_->runInLoop(boost::bind(&TcpConnection::setCloseCallback, conn, cb));
		if (unique){
			conn->forceClose();
		}
	}else{
		connector_->stop();
		// FIXME: HACK
		//loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
	}
}

void TcpClient::connect()
{
	// FIXME: check state
	LogInfo(0, "TcpClient::connect[" << name_ << "] - connecting to "
		<< connector_->serverIpPort());
	connect_ = true;
	connector_->start();
}

void TcpClient::disconnect()
{
	connect_ = false;

	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		if (connection_){
			connection_->shutdown();
		}
	}
}

void TcpClient::stop()
{
	connect_ = false;
	connector_->stop();
}

void TcpClient::newConnection(SocketPtr socketPtr)
{
	loop_->assertInLoopThread();
	
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", endPoint2ipPort(socketPtr->remote_endpoint()).c_str(), nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;

	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary

	//(const EventLoopPtr& loop,
	//	const string& name,
	//	SocketPtr socketPtr)
	TcpConnectionPtr conn(new TcpConnection(loop_,connName,socketPtr));

	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(boost::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		connection_ = conn;
	}
	conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
	if (retry_ && connect_)
	{
		LogInfo(0, "TcpClient::connect[" << name_ << "] - Reconnecting to "
			<< connector_->serverIpPort());
		connector_->restart();
	}
}

