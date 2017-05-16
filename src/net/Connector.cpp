
#include <boost/bind.hpp>
#include <net/Connector.h>
#include <base/Logger.h>

Connector::Connector(const EventLoopPtr& loop, const boost::asio::ip::tcp::endpoint& peer):
loop_(loop),
serverAddr_(peer),
connect_(false),
state_(kDisconnected),
retryDelayMs_(kInitRetryDelayMs)
{

}
Connector::~Connector()
{

}

void Connector::start()  // can be called in any thread
{  
	connect_ = true;
	if(loop_){
		loop_->runInLoop(boost::bind(&Connector::startInLoop, this));
	}
}

void Connector::startInLoop()
{
	assert(state_ == kDisconnected);
	if(!connect_)
		return;
	if(loop_){
		loop_->assertInLoopThread();
		SocketPtr sockPtr(new boost::asio::ip::tcp::socket(*loop_));
		socketWeakPtr_ = sockPtr;
		setState(kConnecting);
		sockPtr->async_connect(serverAddr_, boost::bind(&Connector::handleConnect, this, sockPtr, _1));
	}
}

void Connector::handleConnect(SocketPtr socketPtr, const boost::system::error_code& error)
{
	if (!error){
		// Connect succeeded.
		setState(kConnected);
		if(newConnectionCallback_){
			newConnectionCallback_(socketPtr);
		}
	}else{
		setState(kDisconnected);
		LogError(0, "connect fail :" << error.message());
		retry();
	}
}

void Connector::retry()
{

	SocketPtr socketPtr = socketWeakPtr_.lock();
	if(!socketPtr){
		LogError(0, "retry fail socketWeakPtr_ obj is deaded");
		return;
	}
	socketPtr->close();
	assert(state_ == kDisconnected);
	if (connect_){
		LogInfo(0, "Connector::retry - Retry connecting to " << serverAddr_.address().to_string()
			<< " in " << retryDelayMs_ << " milliseconds. ");
		if(loop_){
			reConnectTimer_ = loop_->runAfter(retryDelayMs_,
				boost::bind(&Connector::startInLoop, this));
			retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
		}
	}
	else{
		LogDebug(0, "do not connect");
	}
}

void Connector::restart()  // must be called in loop thread
{
	if(loop_){
		loop_->assertInLoopThread();
		setState(kDisconnected);
		retryDelayMs_ = kInitRetryDelayMs;
		connect_ = true;
		startInLoop();
	}
}
void Connector::stop()  // can be called in any thread
{
	connect_ = false;
	if(loop_){
		loop_->queueInLoop(boost::bind(&Connector::stopInLoop, this));
		loop_->cancel(reConnectTimer_);
	}
	// FIXME: cancel timer
	
}

//这里的stop只是停止重连，不是关闭连接
void Connector::stopInLoop()
{
	if(loop_){
		loop_->assertInLoopThread();
		if (state_ == kConnecting){
			setState(kDisconnected);
			retry();
		}
	}
}
