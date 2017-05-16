
#include <base/TimerQueue.h>
#include <base/Logger.h>


TimerQueue::TimerQueue(boost::asio::io_service* loop):loop_(loop)
{}
timerWeakPtr TimerQueue::addTimer(const TimerCallback& cb,
					  boost::posix_time::milliseconds when, int interval)
{
	timerPtr tmpPtr(new boost::asio::deadline_timer(*loop_));
	timerWeakPtr weakTimer(tmpPtr);
	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		timerQueue_.insert(std::pair<timerPtr, TimerCallback>(tmpPtr, cb));
		tmpPtr->expires_from_now(when);
		tmpPtr->async_wait(boost::bind(&TimerQueue::handleTimer, this, weakTimer, interval, _1));
	}
	return weakTimer;
}

TimerQueue::~TimerQueue()
{
}

//一定是在事件循环线程调用的，不用加锁
void TimerQueue::handleTimer(timerWeakPtr pTimer,  int interval, const boost::system::error_code& error)
{
	timerPtr deadTimer = pTimer.lock();
	if(deadTimer){
		std::map<timerPtr, TimerCallback>::iterator it = timerQueue_.find(deadTimer);
		assert(it != timerQueue_.end());

		if(!error){
			it->second();
			if(interval == 0){
				//一次性定时
				//把timer从队列中清除，timer的生命周期结束
				timerQueue_.erase(it);
			}else{
				//周期性定时
				deadTimer->expires_from_now(boost::posix_time::milliseconds(interval));
				//根据interval来刷新timer的过期时间
				deadTimer->async_wait(boost::bind(&TimerQueue::handleTimer, this, pTimer, interval, _1));
			}
		}else{
			//清除掉timer
			deadTimer->cancel();
			timerQueue_.erase(it);
			LogError(0, "定时器错误(" << interval << "):" << error.message());
		}
	}else{
		LogInfo(0, "定时器(" << interval << ")已被取消");
	}
}
void TimerQueue::cancel(timerWeakPtr timer)
{
	timerPtr timerToKill = timer.lock();
	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		if(timerToKill){
			timerToKill->cancel();
			std::map<timerPtr, TimerCallback>::iterator it = timerQueue_.find(timerToKill);
			if(it != timerQueue_.end()){
				timerQueue_.erase(it);
			}
		}
	}
}