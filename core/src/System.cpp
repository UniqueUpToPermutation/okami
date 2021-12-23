#include <okami/System.hpp>
#include <okami/Destroyer.hpp>
#include <okami/Frame.hpp>

#include <iostream>

namespace okami::core {
    void PrintWarning(const std::string& str) {
        std::cout << "WARNING: " << str << std::endl;
    }

    SystemCollection::SystemCollection() {
        mSystems.emplace_back(std::make_unique<Destroyer>());
    }

    SystemCollection::~SystemCollection() {
        Shutdown();
    }

    void SystemCollection::BeginExecute(const Time& time) {

        mFrame->SetUpdating(true);

        for (auto& system : mSystems) {
            system->RequestSync(mSyncObject);
        }

        for (auto& system : mSystems) {
            system->BeginExecute(*mFrame, mRenderGroup, mUpdateGroup, mSyncObject, time);
        }
    }

    void SystemCollection::EndExecute() {
        mUpdateGroup.wait();

        for (auto& system : mSystems) {
            system->EndExecute(*mFrame);
        }

        mFrame->SetUpdating(false);
    }

    void SystemCollection::Startup() {
        marl::WaitGroup group;

        mInterfaces = InterfaceCollection();

        for (auto& system : mSystems) {
            system->RegisterInterfaces(mInterfaces);
        }

        for (auto& system : mSystems) {
            system->Startup(group);
        }

        group.wait();
    }

    void SystemCollection::Shutdown() {
        
        // Shutdown systems in reverse order
        for (auto it = mSystems.rbegin(); it != mSystems.rend(); ++it) {
            it->get()->Shutdown();
        }

        mSystems.clear();
    }

    void SystemCollection::LoadResources() {
        marl::WaitGroup group;

        for (auto& system : mSystems) {
            system->LoadResources(group);
        }

        group.wait();
    }

    void SystemCollection::SetFrame(Frame& frame) {
        mFrame = &frame;

        for (auto& system : mSystems) {
            system->SetFrame(frame);
        }
    }
}