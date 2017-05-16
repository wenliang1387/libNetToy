
#include <boost/bind.hpp>
#include <stdio.h>

#include <base/EventLoopThreadPool.h>
#include <base/Logger.h>


EventLoopThreadPool::EventLoopThreadPool(const EventLoopPtr& baseLoop, const string& nameArg)
: baseLoop_(baseLoop),
name_(nameArg),
started_(false),
numThreads_(0),
next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	//no need to care about EventLoop's life it is hold by shared_ptr
	LogDebug(0, "~EventLoopThreadPool()");
	//for(std::vector<EventLoopPtr>::iterator it = loops_.begin(); it != loops_.end(); ++it){
	//	it->stop();
	//}
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	assert(!started_);
	baseLoop_->assertInLoopThread();

	started_ = true;

	for (int i = 0; i < numThreads_; ++i){
		//std::vector<char> buf[name_.size() + 32];
		char buf[255] = {0};
		
		snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
		EventLoopThread* t = new EventLoopThread(buf, cb);
		threads_.push_back(t);
		loops_.push_back(t->startLoop());
	}
	if (numThreads_ == 0 && cb){
		cb(baseLoop_);
	}
}

EventLoopPtr EventLoopThreadPool::getNextLoop()
{
	baseLoop_->assertInLoopThread();
	assert(started_);
	EventLoopPtr loop(baseLoop_);

	if (!loops_.empty())
	{
		// round-robin
		loop = loops_[next_];
		++next_;
		if ((size_t)next_ >= loops_.size()){
			next_ = 0;
		}
	}
	return loop;
}

EventLoopPtr EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
	baseLoop_->assertInLoopThread();
	EventLoopPtr loop(baseLoop_);

	if (!loops_.empty()){
		loop = loops_[hashCode % loops_.size()];
	}
	return loop;
}

std::vector<EventLoopPtr> EventLoopThreadPool::getAllLoops()
{
	baseLoop_->assertInLoopThread();
	assert(started_);
	if (loops_.empty()){
		return std::vector<EventLoopPtr>(1, baseLoop_);
	}else{
		return loops_;
	}
}
