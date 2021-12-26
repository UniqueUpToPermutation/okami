#include <okami/Frame.hpp>
#include <okami/Destroyer.hpp>

namespace okami::core {
    void Destroyer::RegisterInterfaces(InterfaceCollection& interfaces) {
    }
    void Destroyer::Startup(marl::WaitGroup& waitGroup) {
    }
    void Destroyer::Shutdown() {
    }
    void Destroyer::RequestSync(SyncObject& syncObject) {
    }
    void Destroyer::LoadResources(marl::WaitGroup& waitGroup) {
    }
    void Destroyer::SetFrame(Frame& frame) {
    }
    void Destroyer::Fork(Frame& frame,
        SyncObject& syncObject,
        const Time& time) {
    }
    void Destroyer::Wait() {
    }
    void Destroyer::Join(Frame& frame) {
        auto& registry = frame.Registry();
        auto toDelete = registry.view<DestroyTag>();

        // Destroy everything with the destroy tag.
        for (auto e : toDelete) {
            registry.destroy(e);
        }
    }
}