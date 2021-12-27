#pragma once

#include <okami/PlatformDefs.hpp>

#include <chrono>

namespace okami::core {

    struct Time {
        double mTimeElapsed;
        double mTotalTime;
    };

    class Clock
    {
    public:
        Clock();
        void Restart();
        Time GetTime();

    private:
        std::chrono::high_resolution_clock::time_point mStartTime;
        std::chrono::high_resolution_clock::time_point mLastCall;
    };
}