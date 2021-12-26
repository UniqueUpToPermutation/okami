#include <okami/FrameSystem.hpp>
#include <marl/defer.h>

namespace okami::core {
    FrameSystem::FrameSystem(ResourceInterface& resourceInterface) : 
        mManager(
            []() { return Frame(); },
            [this](Frame* frame) { this->OnDestroy(frame); }) {
        resourceInterface.Register(this);
    }

    void FrameSystem::OnDestroy(Frame* frame) {
        delete frame;
    }

    void FrameSystem::RegisterInterfaces(InterfaceCollection& interfaces) {
    }

    void FrameSystem::Startup(marl::WaitGroup& waitGroup) {
    }

    void FrameSystem::Shutdown() {
        mManager.Shutdown();
    }

    void FrameSystem::RequestSync(SyncObject& syncObject) {
    }

    void FrameSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }

    void FrameSystem::SetFrame(Frame& frame) {
    }

    void FrameSystem::Fork(Frame& frame, 
        SyncObject& syncObject,
        const Time& time) {

        mUpdateFinished.clear();
        marl::Task managerUpdate([manager = &mManager, updateFinished = mUpdateFinished] {
            defer(updateFinished.signal());
            manager->RunBackend();
        }, marl::Task::Flags::SameThread);

        marl::schedule(std::move(managerUpdate));
    }

    void FrameSystem::Join(Frame& frame) {
        Wait();
    }

    void FrameSystem::Wait() {
        mUpdateFinished.wait();
    }

    Handle<Frame> FrameSystem::Load(
        const std::filesystem::path& path, 
        const LoadParams<Frame>& params, 
        resource_id_t newResId) {
        
        throw std::runtime_error("Not implemented yet!");
    }

    Handle<Frame> FrameSystem::Add(Frame&& obj, 
        resource_id_t newResId) {
        return mManager.Add(std::move(obj), newResId, nullptr);
    }

    Handle<Frame> FrameSystem::Add(Frame&& obj, 
        const std::filesystem::path& path,
        resource_id_t newResId) {
        return mManager.Add(std::move(obj), path, newResId, nullptr);
    }

    std::unique_ptr<ISystem> FrameSystemFactory(ResourceInterface& res) {
        return std::make_unique<FrameSystem>(res);
    }
}