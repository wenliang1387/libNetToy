
#include <sstream>
#include <net/TcpServer.h>
#include <base/Logger.h>


TcpServer::TcpServer(const EventLoopPtr& loop,
		  const boost::asio::ip::tcp::endpoint& listenAddr,
		  const string& nameArg,
		  Option option):
loop_(loop),
ipPort_(endPoint2ipPort(listenAddr)),
name_(nameArg),
threadPool_(new EventLoopThreadPool(loop_, nameArg)),
acceptor_(new Acceptor(loop_, listenAddr, option == kReusePort, threadPool_.get())),
connectionCallback_(defaultConnectionCallback),
messageCallback_(defaultMessageCallback),
nextConnId_(1)
{
	acceptor_->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	LogDebug(0, "TcpServer::~TcpServer [" << name_ << "] destructing");
	loop_->assertInLoopThread();
	
	for (ConnectionMap::iterator it(connections_.begin());
		it != connections_.end(); ++it)
	{
		TcpConnectionPtr conn = it->second;
		it->second.reset();
		EventLoopPtr ioLoop = conn->getLoop();
		if(ioLoop){
			ioLoop->runInLoop(
				boost::bind(&TcpConnection::connectDestroyed, conn));
		}
		conn.reset();
	}
	if(!loop_->stopped()){
		loop_->quit();
		LogDebug(0, "quit loop in ~TcpServer()");
	}else{
		LogDebug(0, "quited loop in ~TcpServer()");
	}
}

void TcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
	threadPool_->start(threadInitCallback_);
	assert(!acceptor_->listenning());
	loop_->runInLoop(boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
}

//acceptor的setNewConnectionCallback 参数为接收的socket对象以及它关联的EventLoop
void TcpServer::newConnection(SocketPtr sock, EventLoopPtr sockBindedLoop)
{
	loop_->assertInLoopThread();
	EventLoopPtr ioLoop = sockBindedLoop;
	if(!ioLoop){
		LogError(0, "get ioLoop from EventLoopThreadPool fail");
		assert(false);
		return;
	}
	std::stringstream ss;
	ss << name_ << ipPort_ << "#" << nextConnId_;
	++nextConnId_;
	std::string connName = ss.str();
	LogInfo(0, "newConnection:" << connName);
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sock));
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));
	ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	// FIXME: unsafe
	loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LogInfo(0, "TcpServer::removeConnectionInLoop ["<< name_ << "] - connection " << conn->name());
	size_t n = connections_.erase(conn->name());
	(void)n;
	assert(n == 1);
	EventLoopPtr ioLoop = conn->getLoop();
	if(ioLoop){
		ioLoop->runInLoop(
			boost::bind(&TcpConnection::connectDestroyed, conn));
	}
}
