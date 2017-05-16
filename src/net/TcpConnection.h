#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H


#include <base/EventLoop.h>
#include <net/Callbacks.h>
#include <base/Buffer.h>
#include <string>
#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using std::string;

std::string endPoint2ipPort(const boost::asio::ip::tcp::endpoint& listenAddr);

/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : boost::noncopyable,
	public boost::enable_shared_from_this<TcpConnection>
{
public:
	/// Constructs a TcpConnection with a connected sockfd
	///
	/// User should not create this object.
	TcpConnection(const EventLoopPtr& loop,
		const string& name,
		SocketPtr socketPtr);
	~TcpConnection();

	EventLoopPtr getLoop() const { return loop_; }
	const string& name() const { return name_; }
	const boost::asio::ip::tcp::endpoint& localEndpoint() const { return localEndpoint_; }
	const boost::asio::ip::tcp::endpoint& peerEndpoint() const { return peerEndpoint_; }
	bool connected() const { return state_ == kConnected; }
	bool disconnected() const { return state_ == kDisconnected; }
	
	void setTcpNoDelay(bool on);

	//线程安全
	void send(const void* message, int len);
	void send(const std::string& message);
	void send(Buffer* message);  // this one will swap data
	void shutdown(); // NOT thread safe, no simultaneous calling
	void forceClose();
	//void forceCloseWithDelay(int ms);
	//reading or not
	void startRead();
	void stopRead();
	bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

	void setContext(const boost::any& context)
	{ context_ = context; }

	const boost::any& getContext() const
	{ return context_; }

	boost::any* getMutableContext()
	{ return &context_; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
	{ highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

	std::string localIpPort(){return localIpPort_;}
	std::string peerIpPort(){return peerIpPort_;}

	/// Advanced interface
	Buffer* inputBuffer()
	{ return &inputBuffer_; }

	Buffer* outputBuffer()
	{ return &outputBuffer_; }

	/// Internal use only.
	void setCloseCallback(const CloseCallback& cb)
	{ closeCallback_ = cb; }

	//// called when TcpServer accepts a new connection
	void connectEstablished();   // should be called only once
	//// called when TcpServer has removed me from its map
	void connectDestroyed();  // should be called only once
	void beginFirstRead();
private:
	
	void readHandler(const boost::system::error_code& error,std::size_t bytes_transferred );
	void writeHandler(const boost::system::error_code& error,std::size_t bytes_transferred);
	enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
	//void handleRead(Timestamp receiveTime);
	//void handleWrite();
	void handleClose();
	void handleError(const boost::system::error_code& error);
	// void sendInLoop(string&& message);
	//void sendInLoop(const StringPiece& message);
	//void sendInLoop(const void* message, size_t len);
	void shutdownInLoop();
	// void shutdownAndForceCloseInLoop(double seconds);
	void forceCloseInLoop();
	void setState(StateE s) { state_ = s; }
	const char* stateToString() const;
	void startReadInLoop();
	void stopReadInLoop();

	EventLoopPtr loop_;
	const string name_;
	
	StateE state_;  // FIXME: use atomic variable
	bool reading_;
	// we don't expose those classes to client.
	SocketPtr socket_;
	//boost::scoped_ptr<Channel> channel_;
	const boost::asio::ip::tcp::endpoint localEndpoint_;
	const boost::asio::ip::tcp::endpoint peerEndpoint_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	HighWaterMarkCallback highWaterMarkCallback_;
	CloseCallback closeCallback_;
	size_t highWaterMark_;
	Buffer inputBuffer_;
	Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.
	boost::any context_;
	boost::mutex mutex_;
	std::string localIpPort_;
	std::string peerIpPort_;
	// FIXME: creationTime_, lastReceiveTime_
	//        bytesReceived_, bytesSent_
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;
void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr&,Buffer* buf,Timestamp);

#endif  // MUDUO_NET_TCPCONNECTION_H
