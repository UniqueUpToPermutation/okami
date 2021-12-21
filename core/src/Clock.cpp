#include <okami/Clock.hpp>


namespace okami::core {
    Clock::Clock() {
        Restart();
    }
    void Clock::Restart() {
        mStartTime = std::chrono::high_resolution_clock().now();
        mLastCall = mStartTime;
    }

    Time Clock::GetTime() 
    {
        Time time;
        auto CurrTime  = std::chrono::high_resolution_clock::now();
        time.mTotalTime = std::chrono::duration_cast<std::chrono::duration<double>>(CurrTime - mStartTime).count();
        time.mTimeElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(CurrTime - mLastCall).count();
        mLastCall = CurrTime;
        return time;
    }
}