#pragma once

#include <okami/Geometry.hpp>
#include <okami/Material.hpp>

namespace okami::core {
    struct StaticMesh {
        Handle<Geometry> mGeometry;
        Handle<BaseMaterial> mMaterial;
    };
}