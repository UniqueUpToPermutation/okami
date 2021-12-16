#include <okami/System.hpp>

#include <okami/Destroyer.hpp>
#include <okami/FrameSystem.hpp>

namespace okami::core {
    SystemCollection::SystemCollection() {
        mSystems.emplace(std::make_unique<Destroyer>());
    }
}