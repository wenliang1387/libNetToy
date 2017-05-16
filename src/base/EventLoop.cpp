
//#define IS_WINDOWS
#include <base/Logger.h>
#include <base/EventLoop.h>
//#ifdef IS_WINDOWS
//__declspec(thread) EventLoop* t_loopInThisThread;
//#else
__thread EventLoop* t_loopInThisThread;
//#endif

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop():
threadId_(boost::this_thread::get_id())
{
	LogDebug(0, "EventLoop() " << this << " in thread " << threadId_);
	if (t_loopInThisThread){
		LogFatal(0, "Another EventLoop " << t_loopInThisThread
			<< " exists in this thread " << threadId_ );
	}
	else{
		t_loopInThisThread = this;
	}
	timerQueue_.reset(new TimerQueue(this));
}
EventLoop::~EventLoop()
{
	LogDebug(0, "~EventLoop():" << threadId_);
}

void EventLoop::assertInLoopThread(){
	if(!isInLoopThread()){
		LogError(0, "assertInLoopThread false should in thread:" << threadId_ << " but not");
		assert(false);
	}
}
