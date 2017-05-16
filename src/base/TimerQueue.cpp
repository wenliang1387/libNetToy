
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

//һ�������¼�ѭ���̵߳��õģ����ü���
void TimerQueue::handleTimer(timerWeakPtr pTimer,  int interval, const boost::system::error_code& error)
{
	timerPtr deadTimer = pTimer.lock();
	if(deadTimer){
		std::map<timerPtr, TimerCallback>::iterator it = timerQueue_.find(deadTimer);
		assert(it != timerQueue_.end());

		if(!error){
			it->second();
			if(interval == 0){
				//һ���Զ�ʱ
				//��timer�Ӷ����������timer���������ڽ���
				timerQueue_.erase(it);
			}else{
				//�����Զ�ʱ
				deadTimer->expires_from_now(boost::posix_time::milliseconds(interval));
				//����interval��ˢ��timer�Ĺ���ʱ��
				deadTimer->async_wait(boost::bind(&TimerQueue::handleTimer, this, pTimer, interval, _1));
			}
		}else{
			//�����timer
			deadTimer->cancel();
			timerQueue_.erase(it);
			LogError(0, "��ʱ������(" << interval << "):" << error.message());
		}
	}else{
		LogInfo(0, "��ʱ��(" << interval << ")�ѱ�ȡ��");
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