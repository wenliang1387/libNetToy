#ifndef ACCEPTOR_H__
#define ACCEPTOR_H__


#include <boost/asio.hpp>
#include <base/EventLoop.h>
#include <net/Callbacks.h>
#include <base/EventLoopThreadPool.h>


///
/// Acceptor of incoming TCP connections.
///


class Acceptor : boost::noncopyable
{
public:
	Acceptor(EventLoopPtr loop, const boost::asio::ip::tcp::endpoint& listenAddr, bool reuseport = true, EventLoopThreadPool*ioPool = NULL);
	~Acceptor();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	bool listenning() const { return listenning_; }
	void listen();

private:
	void listenInLoop();
	void acceptHandler(SocketPtr newSock, const boost::system::error_code& error);
	boost::asio::ip::tcp::acceptor acceptor_;
	EventLoopPtr loop_;

	NewConnectionCallback newConnectionCallback_;
	bool listenning_;
	EventLoopThreadPool*pool_;
	EventLoopPtr newSockBindedLoop_;
	//int idleFd_;
};

#endif