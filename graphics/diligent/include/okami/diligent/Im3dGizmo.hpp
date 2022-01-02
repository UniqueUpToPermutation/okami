#pragma once

#include <okami/System.hpp>
#include <okami/Graphics.hpp>

namespace okami::graphics {
    struct GizmoSelectTag {
    };

    enum class GizmoSelectionMode {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    class IGizmo {
    public:
        virtual void SetMode(GizmoSelectionMode mode) = 0;
        virtual GizmoSelectionMode GetMode() const = 0;
    };

    std::unique_ptr<core::ISystem> CreateGizmoSystem(
        core::ISystem* im3dSystem);
}