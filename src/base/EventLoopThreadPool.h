
#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

//#include <muduo/base/Types.h>

#include <vector>
#include <string>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <base/EventLoopThread.h>

using std::size_t;
using std::string;
class EventLoopThreadPool : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoopPtr)> ThreadInitCallback;

	EventLoopThreadPool(const EventLoopPtr& baseLoop, const string& nameArg);
	~EventLoopThreadPool();
	void setThreadNum(int numThreads) { numThreads_ = numThreads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	// valid after calling start()
	/// round-robin
	EventLoopPtr getNextLoop();

	/// with the same hash code, it will always return the same EventLoop
	EventLoopPtr getLoopForHash(size_t hashCode);

	std::vector<EventLoopPtr> getAllLoops();

	bool started() const
	{ return started_; }

	const string& name() const
	{ return name_; }

private:

	EventLoopPtr baseLoop_;
	string name_;
	bool started_;
	int numThreads_;
	int next_;
	boost::ptr_vector<EventLoopThread> threads_;
	std::vector<EventLoopPtr> loops_;
};


#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H
