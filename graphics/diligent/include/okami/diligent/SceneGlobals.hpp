#pragma once

#include <BasicMath.hpp>

#include <okami/diligent/Buffers.hpp>

namespace HLSL {
    using namespace Diligent;
    #include <shaders/Common.hlsl>
}

namespace okami::graphics::diligent {
    namespace DG = Diligent;

    class IGlobalsBufferProvider {
    public:
        virtual DynamicUniformBuffer<HLSL::SceneGlobals>*
            GetGlobalsBuffer() = 0;
    };
};

