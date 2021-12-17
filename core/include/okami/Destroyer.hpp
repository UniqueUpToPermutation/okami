#pragma once

#include <okami/System.hpp>

namespace okami::core {
    struct DestroyTag { 
    };

    class Destroyer final : public ISystem {
    public:
        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(Frame* frame, 
            marl::WaitGroup& waitGroup) override;
        void BeginExecute(Frame* frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            SyncObject& syncObject,
            const Time& time) override;
        void EndExecute(Frame* frame) override;
    };
}