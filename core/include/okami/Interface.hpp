#pragma once

#include <okami/System.hpp>

namespace okami::core {

    /*
        Creates a frame system. Frame systems are responsible
        for loading and managing active frames.
    */
    std::unique_ptr<ISystem> FrameSystemFactory(ResourceInterface&);
}