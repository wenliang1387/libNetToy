
#include <net/TcpConnection.h>
#include <base/Logger.h>
#include <base/WeakCallback.h>

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	LogInfo(0, conn->localIpPort() << " -> "
		<< conn->peerIpPort() << " is "
		<< (conn->connected()?"UP":"DOWN"));
	// do not call conn->forceClose(), because some users want to register message callback only.
}

void defaultMessageCallback(const TcpConnectionPtr&,
										Buffer* buf,
										Timestamp)
{
	buf->retrieveAll();
}

std::string endPoint2ipPort(const boost::asio::ip::tcp::endpoint& listenAddr)
{
	std::stringstream ss;
	ss << listenAddr.address().to_string() << ":" << listenAddr.port();
	return ss.str();
}

using namespace boost::posix_time;

TcpConnection::TcpConnection(const EventLoopPtr& loop,
			  const string& name,
			  SocketPtr socketPtr):
loop_(loop),
name_(name),     
state_(kConnecting),
reading_(true),
socket_(socketPtr),
localEndpoint_(socket_->local_endpoint()),
peerEndpoint_(socket_->remote_endpoint()),
highWaterMark_(64*1024*1024),
localIpPort_(endPoint2ipPort(localEndpoint_)),
peerIpPort_(endPoint2ipPort(peerEndpoint_))
{
	LogDebug(0, "TcpConnection::ctor[" <<  name_  << "]");
	boost::asio::socket_base::keep_alive option(true);
	socket_->set_option(option);
}

void TcpConnection::beginFirstRead()
{
	socket_->async_read_some(boost::asio::buffer(inputBuffer_.beginWrite(), inputBuffer_.writableBytes()),
		boost::bind(&TcpConnection::readHandler, this, _1, _2));
}

TcpConnection::~TcpConnection()
{
	LogDebug(0, "TcpConnection::dtor[" <<  name_ << "] at " << this
		<< " state=" << stateToString());
	assert(state_ == kDisconnected);
}

const char* TcpConnection::stateToString() const
{
	switch (state_)
	{
	case kDisconnected:
		return "kDisconnected";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	default:
		return "unknown state";
	}
}

void TcpConnection::setTcpNoDelay(bool on)
{
	boost::asio::ip::tcp::no_delay option(on);
	socket_->set_option(option);
}

void TcpConnection::readHandler(const boost::system::error_code& error,std::size_t bytes_transferred )
{
	if(!error){
		ptime t(microsec_clock::local_time());
		inputBuffer_.hasWritten(bytes_transferred);
		if(messageCallback_){
			messageCallback_(shared_from_this(), &inputBuffer_, t);
		}
		if(reading_){
			inputBuffer_.ensureWritableBytes(bytes_transferred);
			socket_->async_read_some(boost::asio::buffer(inputBuffer_.beginWrite(), inputBuffer_.writableBytes()),
				boost::bind(&TcpConnection::readHandler, this, _1, _2));
		}
	}else if(error == boost::asio::error::eof){
		handleClose();
	}else{
		handleError(error);
		//to do 看看会有写什么错误，判断这里要不要继续读数据
		if(reading_){
			inputBuffer_.ensureWritableBytes(bytes_transferred);
			socket_->async_read_some(boost::asio::buffer(inputBuffer_.beginWrite(), inputBuffer_.writableBytes()),
				boost::bind(&TcpConnection::readHandler, this, _1, _2));
		}
	}
}
void TcpConnection::writeHandler(const boost::system::error_code& error,std::size_t bytes_transferred)
{
	if(!error){
		outputBuffer_.retrieve(bytes_transferred);
		if(writeCompleteCallback_){
			writeCompleteCallback_(shared_from_this());
		}
	}else{
		LogError(0, "writeHandler error:" << error.message());
	}
}

void TcpConnection::handleClose()
{
	EventLoopPtr loop = loop_;
	if(loop){
		loop->assertInLoopThread();
		LogDebug(0, "handleClose: state = " << stateToString());
		assert(state_ == kConnected || state_ == kDisconnecting);
		// we don't close fd, leave it to dtor, so we can find leaks easily.
		setState(kDisconnected);
		reading_ = false;
		TcpConnectionPtr guardThis(shared_from_this());
		connectionCallback_(guardThis);
		// must be the last line
		//这个回调是给tcpServer调用的，用来清理掉TcpConnections map 中的TcpConnection对象
		closeCallback_(guardThis);
	}
}

void TcpConnection::handleError(const boost::system::error_code& error)
{
	LogError(0, "TcpConnection::handleError [" << name_ << "] error message:" << error.message());
	//to do 处理这里的busy loop
	handleClose();
}

void TcpConnection::startRead()
{
	if(loop_){
		loop_->runInLoop(boost::bind(&TcpConnection::startReadInLoop, this));
	}
}

void TcpConnection::startReadInLoop()
{
	if(!reading_){
		reading_ = true;
		socket_->async_read_some(boost::asio::buffer(inputBuffer_.beginWrite(), inputBuffer_.writableBytes()),
			boost::bind(&TcpConnection::readHandler, this, _1, _2));
	}
}

void TcpConnection::stopRead()
{
	if(loop_){
		loop_->runInLoop(boost::bind(&TcpConnection::stopReadInLoop, this));
	}
}
void TcpConnection::stopReadInLoop()
{
	if(reading_){
		reading_ = false;
	}
}

//由TcpServer在其acceptor的连接建立回调当中调用
void TcpConnection::connectEstablished()
{
	if(loop_){
		loop_->assertInLoopThread();
		assert(state_ == kConnecting);
		setState(kConnected);
		beginFirstRead();
		connectionCallback_(shared_from_this());
	}
}

void TcpConnection::connectDestroyed()
{
	if(loop_){
		loop_->assertInLoopThread();
		if (state_ == kConnected){
			setState(kDisconnected);
			connectionCallback_(shared_from_this());
		}
	}
}

void TcpConnection::send(const void* message, int len)
{
	send(std::string((const char*)message, (size_t)len));
}

void TcpConnection::send(Buffer* buf)
{
	send(buf->retrieveAllAsString());
}

void TcpConnection::send(const std::string& message)
{
	boost::lock_guard<boost::mutex> lock(mutex_);
	outputBuffer_.append(message.data(), message.size());
	socket_->async_send(boost::asio::buffer(outputBuffer_.peek(), 
		outputBuffer_.readableBytes()), boost::bind(&TcpConnection::writeHandler, this, _1, _2));
}


void TcpConnection::shutdown()
{
	if (state_ == kConnected){
		if(loop_){
			setState(kDisconnecting);
			loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
		}
	}
}
void TcpConnection::shutdownInLoop()
{
	if(!loop_){
		assert(false);
		return;
	}
	loop_->assertInLoopThread();
	socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}

void TcpConnection::forceClose()
{
	if (state_ == kConnected || state_ == kDisconnecting){
		setState(kDisconnecting);
		loop_->runInLoop(boost::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
	}
}

void TcpConnection::forceCloseInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnected || state_ == kDisconnecting){
		// as if we received 0 byte in handleRead();
		handleClose();
	}
}

//void TcpConnection::forceCloseWithDelay(int ms)
//{
//	if (state_ == kConnected || state_ == kDisconnecting){
//		setState(kDisconnecting);
//		// not forceCloseInLoop to avoid race condition
//		loop_->runAfter(ms,makeWeakCallback(shared_from_this(),&TcpConnection::forceClose)); 
//	}
//}
