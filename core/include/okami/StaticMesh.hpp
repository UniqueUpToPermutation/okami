#pragma once

#include <okami/Geometry.hpp>

namespace okami::core {
    struct StaticMesh {
        Handle<Geometry> mGeometry;
    };
}