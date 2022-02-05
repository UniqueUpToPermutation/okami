#pragma once

#include <okami/Graphics.hpp>
#include <okami/Camera.hpp>
#include <okami/Geometry.hpp>
#include <okami/Embed.hpp>

#include <DeviceContext.h>
#include <RenderDevice.h>
#include <SwapChain.h>
#include <BasicMath.hpp>
#include <RefCntAutoPtr.hpp>

namespace DG = Diligent;

namespace okami::graphics::diligent {

    struct GeometryImpl {
        std::vector<DG::RefCntAutoPtr<DG::IBuffer>>
            mVertexBuffers;
        DG::RefCntAutoPtr<DG::IBuffer>
            mIndexBuffer;
    };

    struct TextureImpl {
        DG::RefCntAutoPtr<DG::ITexture>
            mTexture;
    };

    inline TextureImpl* GetTextureImpl(core::Texture* texture) {
        return reinterpret_cast<TextureImpl*>(texture->GetBackend());
    }

    inline GeometryImpl* GetGeometryImpl(core::Geometry* geo) {
        return reinterpret_cast<GeometryImpl*>(geo->GetBackend());
    }

    struct RenderModuleParams {
        std::vector<RenderPass> mRequestedRenderPasses;
        core::IVirtualFileSystem* mFileSystem = nullptr;
        DG::RefCntAutoPtr<DG::ITexture> mDefaultTexture;
    };

    struct RenderModuleGlobals {
        DG::float4x4 mView;
        DG::float4x4 mProjection;
        DG::float2 mViewportSize;
        DG::float3 mViewOrigin;
        DG::float3 mViewDirection;
        DG::float3 mWorldUp;
        core::Camera mCamera;
        core::Time mTime;
    };

    class IRenderPassFormatProvider {
    public:
        virtual DG::TEXTURE_FORMAT GetFormat(RenderAttribute attrib) = 0;
        virtual DG::TEXTURE_FORMAT GetDepthFormat(const RenderPass& pass) = 0;
    };

    class IRenderModule : public IGraphicsObject {
    public:
        virtual void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device,
            const RenderModuleParams& params) = 0;
        virtual void Update(
            bool bAllowBlock = false) = 0;
        virtual void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& view,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) = 0;
        virtual void WaitUntilReady(core::SyncObject& obj) = 0;
        virtual void Shutdown() = 0;
        virtual void RegisterVertexFormats(core::VertexLayoutRegistry& registry) = 0;
        virtual void RegisterResourceInterfaces(core::ResourceInterface& resourceInterface) = 0;

        void* GetBackend() override final;
    };

    class IOverlayModule : 
        public IRenderModule, 
        public IRenderCanvasAttachment {
    public:
        void* GetUserData() override final;
        void RegisterVertexFormats(core::VertexLayoutRegistry& registry) override final;
        void RegisterResourceInterfaces(core::ResourceInterface& resourceInterface) override final;
    };
}