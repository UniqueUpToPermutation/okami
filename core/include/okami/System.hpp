#pragma once 

#include <okami/Frame.hpp>
#include <marl/waitgroup.h>

namespace okami::core {
    class ISystem {
    public:
        virtual void Startup(marl::WaitGroup& waitGroup) = 0;
        virtual void Shutdown(marl::WaitGroup& waitGroup) = 0;
        virtual void LoadResources(Frame* frame, marl::WaitGroup& waitGroup) = 0;
        virtual void Execute(Frame* frame, marl::WaitGroup& waitGroup) = 0;
    };
}