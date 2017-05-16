#ifndef EVENT_LOOP_THREAD__
#define EVENT_LOOP_THREAD__
#include <base/EventLoop.h>
#include <string>
#include <boost/function.hpp>

class ThreadWithName:public boost::thread
{
	typedef boost::function<void()> Func;
public:
	ThreadWithName(Func f, std::string name = std::string())
	:boost::thread(f),name_(name)
	{}
	~ThreadWithName();
	const std::string& name(){return name_;} 
private:
	std::string name_;
};



typedef boost::function<void(EventLoopPtr)> ThreadInitCallback;

class EventLoopThread : boost::noncopyable
{
	typedef boost::scoped_ptr<ThreadWithName> ThreadPtr;
public:
	EventLoopThread(const std::string& threadName = std::string(),const ThreadInitCallback& cb = ThreadInitCallback());
	~EventLoopThread();
	EventLoopPtr startLoop();
	const std::string& name(){return name_;} 

private:
	void threadFunc();
	EventLoopPtr loop_;
	ThreadPtr thread_;
	boost::mutex mutex_;
	boost::condition_variable cv_;

	ThreadInitCallback callback_;
	std::string name_;
};

#endif