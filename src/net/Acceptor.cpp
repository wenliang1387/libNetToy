
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
//���Acceptorû��io�¼�ѭ���̳߳صĻ��ͽ�accept����socketȫ�����䵽��acceptor�������߳���ȥ
//���Acceptor�ж������io�̵߳Ļ��ͽ�accept����socket���䵽io�̳߳���ȥ
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
		LogDebug(0, "accept��������");
		// Accept succeeded.
		if(newConnectionCallback_){
			newConnectionCallback_(newSock, newSockBindedLoop_);
		}
		listen();
	}else{
		LogError(0, "accept error:" << error.message());
	}
	
}