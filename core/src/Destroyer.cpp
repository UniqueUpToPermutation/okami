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
    void Destroyer::LoadResources(Frame* frame, 
        marl::WaitGroup& waitGroup) {
    }
    void Destroyer::BeginExecute(Frame* frame, 
        marl::WaitGroup& renderGroup, 
        marl::WaitGroup& updateGroup,
        SyncObject& syncObject,
        const Time& time) {
    }
    void Destroyer::EndExecute(Frame* frame) {
        auto& registry = frame->Registry();
        auto toDelete = registry.view<DestroyTag>();

        // Destroy everything with the destroy tag.
        for (auto e : toDelete) {
            registry.destroy(e);
        }
    }
}