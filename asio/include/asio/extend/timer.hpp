//
// timer.hpp
// add by [11/21/2022 cuihaoyun]
// wrap cpp11 thread

#ifndef __TIMER_HPP__
#define __TIMER_HPP__
#include <asio/extend/worker.hpp>

namespace asio {

	class AsioTimer : public Worker
	{
	public:
		AsioTimer() ASIO_NOEXCEPT
			:m_timer(m_ioc), m_worker(m_ioc) {}
		~AsioTimer() {}

		void Run() override {
			m_ioc.run();
		}

		void Stop() {
			if (!m_worker.get_io_context().stopped())
			{
				m_worker.get_io_context().stop();
			}
		}

		asio::steady_timer& GetTimer() {
			return m_timer;
		}
	protected:
	private:
		asio::io_context m_ioc;
		asio::steady_timer m_timer;
		asio::io_context::work m_worker;
	};

}


#endif // __TIMER_HPP__
