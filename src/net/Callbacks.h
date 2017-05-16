
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
class TcpConnection;
class Buffer;
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef boost::posix_time::ptime Timestamp;
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
typedef boost::weak_ptr<boost::asio::ip::tcp::socket> SocketWeakPtr;

typedef boost::function<void()> TimerCallback;
typedef boost::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef boost::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef boost::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
typedef boost::function<void (const TcpConnectionPtr&, Buffer*,Timestamp)> MessageCallback;
typedef boost::function<void (SocketPtr sock, EventLoopPtr sockBindedLoop)> NewConnectionCallback;