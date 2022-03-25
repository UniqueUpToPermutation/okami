#pragma once

#include <BasicMath.hpp>

#include <okami/diligent/Buffers.hpp>

namespace HLSL {
    using namespace Diligent;
    #include <shaders/Interface/Common.hlsl>
    #include <shaders/Interface/Lights.hlsl>
    #include <shaders/Interface/Material.hlsl>
}

namespace okami::graphics::diligent {
    namespace DG = Diligent;

    class IGlobalsBufferProvider {
    public:
        virtual DynamicUniformBuffer<HLSL::SceneGlobals>*
            GetGlobalsBuffer() = 0;
    };
};

