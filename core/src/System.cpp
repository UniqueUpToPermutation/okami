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

    void SystemCollection::Fork(const Time& time) {

        mFrame->SetUpdating(true);

        for (auto& system : mSystems) {
            system->RequestSync(mSyncObject);
        }

        for (auto& system : mSystems) {
            system->Fork(*mFrame, mSyncObject, time);
        }
    }

    void SystemCollection::Join() {
        for (auto& system : mSystems) {
            system->Join(*mFrame);
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

        // Free systems in reverse order
        while (mSystems.size()) {
            mSystems.pop_back();
        }
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

    void ISystem::Request(const entt::meta_type&) {
    }
}