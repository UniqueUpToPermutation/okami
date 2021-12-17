#include <okami/System.hpp>
#include <okami/Destroyer.hpp>
#include <okami/Frame.hpp>

namespace okami::core {
    SystemCollection::SystemCollection() {
        mSystems.emplace(std::make_unique<Destroyer>());
    }

    void SystemCollection::BeginExecute(Frame* frame, const Time& time) {
        mFrame = frame;
        mFrame->SetUpdating(true);

            for (auto& system : mSystems) {
            system->BeginExecute(frame, mRenderGroup, mUpdateGroup, mSyncObject, time);
        }
    }

    void SystemCollection::EndExecute() {
        mUpdateGroup.wait();

        for (auto& system : mSystems) {
            system->EndExecute(mFrame);
        }

        mFrame->SetUpdating(false);
        mFrame = nullptr;
    }

    void SystemCollection::Startup() {
        marl::WaitGroup group;

        for (auto& system : mSystems) {
            system->Startup(group);
        }

        group.wait();
    }

    void SystemCollection::Shutdown() {
        for (auto& system : mSystems) {
            system->Shutdown();
        }
    }

    void SystemCollection::LoadResources(Frame* frame) {
        marl::WaitGroup group;

        for (auto& system : mSystems) {
            system->LoadResources(frame, group);
        }

        group.wait();
    }
}