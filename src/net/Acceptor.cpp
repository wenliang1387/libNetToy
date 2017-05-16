
#include <boost/bind.hpp>
#include <base/Logger.h>
#include <net/Acceptor.h>

Acceptor::Acceptor(EventLoopPtr loop, const boost::asio::ip::tcp::endpoint& listenAddr, bool reuseport, EventLoopThreadPool*ioPool):
acceptor_(*loop, listenAddr, reuseport),loop_(loop),listenning_(false), pool_(ioPool)
{

}
Acceptor::~Acceptor()
{
	LogDebug(0, "~Acceptor()");
}

void Acceptor::listen()
{
	if(loop_){
		loop_->runInLoop(boost::bind(&Acceptor::listenInLoop, this));
	}

}
//如果Acceptor没有io事件循环线程池的话就将accept到的socket全都分配到和acceptor监听的线程上去
//如果Acceptor有额外分配io线程的话就将accept到的socket分配到io线程池上去
void Acceptor::listenInLoop()
{
	EventLoopPtr loop = loop_;
	if(loop){
		listenning_ = true;
		SocketPtr sockPtr;
		if(pool_ != NULL){
			EventLoopPtr ioLoop = pool_->getNextLoop();
			newSockBindedLoop_ = ioLoop;
			assert(ioLoop);
			sockPtr.reset(new boost::asio::ip::tcp::socket(*ioLoop));
		}else{
			newSockBindedLoop_ = loop;
			sockPtr.reset(new boost::asio::ip::tcp::socket(*loop));
		}
		acceptor_.async_accept(*sockPtr, boost::bind(&Acceptor::acceptHandler, this, sockPtr, _1));
	}
}


void Acceptor::acceptHandler(SocketPtr newSock, const boost::system::error_code& error)
{
	if (!error){
		LogDebug(0, "accept到新连接");
		// Accept succeeded.
		if(newConnectionCallback_){
			newConnectionCallback_(newSock, newSockBindedLoop_);
		}
		listen();
	}else{
		LogError(0, "accept error:" << error.message());
	}
	
}