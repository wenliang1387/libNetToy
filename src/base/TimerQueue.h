
#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
typedef boost::shared_ptr<boost::asio::deadline_timer> timerPtr;
typedef boost::weak_ptr<boost::asio::deadline_timer> timerWeakPtr;
typedef boost::function<void()> TimerCallback;
class TimerQueue : boost::noncopyable
{
public:
	TimerQueue(boost::asio::io_service* loop);
	~TimerQueue();

	///thread safe. Usually be called from other threads.
	//interval ºÁÃëÎªµ¥Î»
	timerWeakPtr addTimer(const TimerCallback& cb,boost::posix_time::milliseconds when, int interval);
	//thread safe
	void cancel(timerWeakPtr timer);
private:
	void handleTimer(timerWeakPtr pTimer, int interval, const boost::system::error_code& error);
	std::map<timerPtr, TimerCallback> timerQueue_;
	boost::asio::io_service* loop_;
	boost::mutex mutex_;
};
#endif  // MUDUO_NET_TIMERQUEUE_H
