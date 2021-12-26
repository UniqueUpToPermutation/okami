#pragma once

#include <okami/System.hpp>

namespace okami::core {
    struct DestroyTag { 
    };

    class Destroyer final : public ISystem {
    public:
        void RegisterInterfaces(InterfaceCollection& interfaces) override;
        void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void SetFrame(Frame& frame) override;
        void RequestSync(SyncObject& syncObject) override;
        void Fork(Frame& frame, 
            SyncObject& syncObject,
            const Time& time) override;
        void Join(Frame& frame) override;
        void Wait() override;
    };
}