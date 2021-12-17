#pragma once

#include <okami/System.hpp>
#include <okami/Frame.hpp>
#include <okami/Meta.hpp>

namespace okami::core {

    /*
        Creates a frame system. Frame systems are responsible
        for loading and managing active frames.
    */
    std::unique_ptr<ISystem> FrameSystemFactory(ResourceInterface&);
}