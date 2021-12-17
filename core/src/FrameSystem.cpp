#include <okami/FrameSystem.hpp>
#include <marl/defer.h>

namespace okami::core {
    FrameSystem::FrameSystem(ResourceInterface& resourceInterface) : 
        mManager([this](Frame* frame) { this->OnDestroy(frame); }) {
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

    void FrameSystem::LoadResources(Frame* frame, 
        marl::WaitGroup& waitGroup) {
    }

    void FrameSystem::BeginExecute(Frame* frame, 
        marl::WaitGroup& renderGroup, 
        marl::WaitGroup& updateGroup,
        SyncObject& syncObject,
        const Time& time) {

        updateGroup.add();
        marl::Task managerUpdate([manager = &mManager, updateGroup] {
            defer(updateGroup.done());
            manager->RunBackend();
        }, marl::Task::Flags::SameThread);

        marl::schedule(std::move(managerUpdate));
    }

    void FrameSystem::EndExecute(Frame* frame) {
    }

    Future<Handle<Frame>> FrameSystem::Load(
        const std::filesystem::path& path, 
        const LoadParams<Frame>& params, 
        resource_id_t newResId) {
        
        throw std::runtime_error("Not implemented yet!");
    }

    Future<Handle<Frame>> FrameSystem::Add(Frame&& obj, 
        resource_id_t newResId) {
        return mManager.Add(std::move(obj), newResId, nullptr);
    }

    Future<Handle<Frame>> FrameSystem::Add(Frame&& obj, 
        const std::filesystem::path& path,
        resource_id_t newResId) {
        return mManager.Add(std::move(obj), path, newResId, nullptr);
    }

    std::unique_ptr<ISystem> FrameSystemFactory(ResourceInterface& res) {
        return std::make_unique<FrameSystem>(res);
    }
}