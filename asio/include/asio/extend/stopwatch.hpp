#ifndef __STOPWATCH_HPP__
#define __STOPWATCH_HPP__
#include <cstdio>
#include <ctime>

namespace asio {
    /**
      Simple stopwatch for measuring elapsed time.
    */
    class Stopwatch
    {
    private:
        clock_t m_start;

    public:
        Stopwatch() { start(); }

        void reStart() 
        {
            this->start();
        }
        
        void reset() 
        {
            this->start();
        }
    protected:
        void start() { m_start = clock(); }

    public:
        // ms seconds
        double getElapsedSeconds()
        {
            clock_t now;

            now = clock();

            return (double(now - m_start)) / CLOCKS_PER_SEC;
        }
    };
}



#endif // __STOPWATCH_HPP__
