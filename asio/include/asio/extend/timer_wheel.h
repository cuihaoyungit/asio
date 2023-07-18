#ifndef TIMERWHEEL_H
#define TIMERWHEEL_H
#pragma once
#include <list>
#include <vector>
#include <functional>

//### 时间轮计时器 ###
// 参考概念资料
// https://blog.csdn.net/u011726005/article/details/108926184
//
namespace TimerWheel {

	class TimerManager;

	class Timer
	{
	public:
		enum TimerType { ONCE, CIRCLE };

		Timer(TimerManager& manager);
		~Timer();

		template<typename Fun>
		void Start(const Fun& fun, unsigned interval, TimerType timeType = CIRCLE);
		void Stop();

		int Id()
		{
			return this->id_;
		}
		void SetId(const int id)
		{
			this->id_ = id;
		}
	private:
		void OnTimer(unsigned long long now);

	private:
		friend class TimerManager;

		TimerManager& manager_;
		TimerType timerType_;
		std::function<void(int)> timerTask_;
		unsigned interval_;
		unsigned long long expires_;

		int vecIndex_;
		std::list<Timer*>::iterator itr_;
		int id_;
	};

	class TimerManager
	{
	public:
		TimerManager();

		static unsigned long long GetCurrentMillisecs();
		void DetectTimers();

	private:
		friend class Timer;
		void AddTimer(Timer* timer);
		void RemoveTimer(Timer* timer);

		int Cascade(int offset, int index);

	private:
		typedef std::list<Timer*> TimeList;
		std::vector<TimeList> tvec_;
		unsigned long long checkTime_;
	};

	template<typename Fun>
	inline void Timer::Start(const Fun& fun, unsigned interval, TimerType timeType)
	{
		Stop();
		interval_ = interval;
		timerFun_ = fun;
		timerType_ = timeType;
		expires_ = interval_ + TimerManager::GetCurrentMillisecs();
		manager_.AddTimer(this);
	}
}

/*
*   Example use:
*	TimerWheel::TimerManager tm;
*	TimerWheel::Timer t(tm);
*	TimerWheel::Timer t2(tm);
*	t.Start(&TimerHandler, 1000);   //t.Start<std::function<void()>>(&TimerHandler, 1000);
*	t2.Start(&TimerHandler2, 2000); //t2.Start<std::function<void()>>(&TimerHandler2, 2000);
*	while (true)
*	{
*		tm.DetectTimers();
*		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
*	}
*/
#endif // TIMERWHEEL_H
