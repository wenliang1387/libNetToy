
#include <base/EventLoopThread.h>
#include <base/Logger.h>

ThreadWithName::~ThreadWithName()
{
	LogDebug(0, "~thread:" << name_);
}
EventLoopThread::EventLoopThread(const std::string& name,const ThreadInitCallback& cb):
callback_(cb),
name_(name)
{

}
EventLoopThread::~EventLoopThread()
{
	if(loop_){
		loop_->quit();
		thread_->join();
	}
	LogDebug(0, "~EventLoopThread()");
}
EventLoopPtr EventLoopThread::startLoop()
{
	thread_.reset(new ThreadWithName(boost::bind(&EventLoopThread::threadFunc, this), name_));
	//这里等条件变量，等EventLoop在线程函数内new出来
	boost::unique_lock<boost::mutex> lock(mutex_);
	while(!loop_)cv_.wait(lock);
	return loop_;
}
void EventLoopThread::threadFunc()
{
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		loop_.reset(new EventLoop);
		//这里通知条件变量
		if(!loop_){
			LogFatal(0, "new EventLoop fail");
			assert(false);
		}else{
			LogInfo(0, "new EventLoop success");
			cv_.notify_one();
		}
	}

	if(callback_){
		callback_(loop_);
	}
	loop_->loop();
}