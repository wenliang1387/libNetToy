#ifndef EVENT_LOOP_H__
#define EVENT_LOOP_H__


#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/any.hpp>
#include <base/TimerQueue.h>
#include <boost/scoped_ptr.hpp>


class EventLoop:public boost::asio::io_service, boost::noncopyable
{
	typedef boost::scoped_ptr<boost::asio::io_service::work> workPtr;
	typedef boost::scoped_ptr<TimerQueue> timerQueuePtr;
	typedef boost::function<void()> Functor;
public:
	EventLoop();
	virtual ~EventLoop();
	inline void loop(){work_.reset(new boost::asio::io_service::work(*this));run();}
	inline void quit(){stop();}
	inline timerWeakPtr runAfter(int delayMs, const TimerCallback& cb);
	inline timerWeakPtr runEvery(int intervalMs, const TimerCallback& cb);
	inline void cancel(timerWeakPtr toKill);
	inline bool isInLoopThread(){return (threadId_ == boost::this_thread::get_id());}
	void assertInLoopThread();
	inline void queueInLoop(const Functor& cb){post(cb);}
	inline void runInLoop(const Functor& cb);
	inline void setContext(const boost::any& context){ context_ = context; }
	inline const boost::any& getContext() const{ return context_; }
	inline boost::any* getMutableContext(){ return &context_; }
	static EventLoop* getEventLoopOfCurrentThread();
private:
	workPtr work_;
	boost::thread::id threadId_;
	timerQueuePtr timerQueue_;
	boost::any context_;
};

typedef boost::weak_ptr<EventLoop> EventLoopWeakPtr;
typedef boost::shared_ptr<EventLoop> EventLoopPtr;



inline timerWeakPtr EventLoop::runAfter(int delayMs, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, boost::posix_time::milliseconds(delayMs), 0);
}
inline timerWeakPtr EventLoop::runEvery(int intervalMs, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, boost::posix_time::milliseconds(intervalMs), intervalMs);
}
inline void EventLoop::cancel(timerWeakPtr toKill)
{
	timerQueue_->cancel(toKill);
}

inline void EventLoop::runInLoop(const Functor& cb)
{
	if (isInLoopThread()){
		cb();
	}else{
		queueInLoop(cb);
	}
}


#endif
