
#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <base/EventLoopThread.h>
#include <net/TcpConnection.h>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
typedef boost::weak_ptr<boost::asio::ip::tcp::socket> SocketWeakPtr;

class Connector : boost::noncopyable,
	public boost::enable_shared_from_this<Connector>
{
public:
	typedef boost::function<void (SocketPtr socketPtr)> NewConnectionCallback;

	Connector(const EventLoopPtr& loop, const boost::asio::ip::tcp::endpoint& peer);
	~Connector();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	void start();  // can be called in any thread
	void restart();  // must be called in loop thread
	void stop();  // can be called in any thread

	const boost::asio::ip::tcp::endpoint& serverAddress() const { return serverAddr_; }
	const std::string serverIpPort(){return endPoint2ipPort(serverAddr_);}

private:
	enum States { kDisconnected, kConnecting, kConnected };
	static const int kMaxRetryDelayMs = 30*1000;
	static const int kInitRetryDelayMs = 500;

	void setState(States s) { state_ = s; }
	void startInLoop();
	void stopInLoop();
	void connect();
	//void connecting(int sockfd);
	void handleWrite();
	void handleError();
	void handleConnect(SocketPtr socketWeakPtr, const boost::system::error_code& error);
	void retry();


	EventLoopPtr loop_;
	boost::asio::ip::tcp::endpoint serverAddr_;
	bool connect_; // atomic
	States state_;  // FIXME: use atomic variable
	NewConnectionCallback newConnectionCallback_;
	int retryDelayMs_;
	SocketWeakPtr socketWeakPtr_;
	timerWeakPtr reConnectTimer_;
};


#endif  // MUDUO_NET_CONNECTOR_H
