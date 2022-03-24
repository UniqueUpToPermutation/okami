#pragma once

#include <okami/Graphics.hpp>
#include <okami/Camera.hpp>
#include <okami/Geometry.hpp>
#include <okami/Embed.hpp>
#include <okami/ResourceManager.hpp>

#include <DeviceContext.h>
#include <RenderDevice.h>
#include <SwapChain.h>
#include <BasicMath.hpp>
#include <RefCntAutoPtr.hpp>

namespace DG = Diligent;

namespace okami::graphics::diligent {

    struct GeometryBackend {
        core::Geometry::Desc mDesc;
        std::vector<DG::RefCntAutoPtr<DG::IBuffer>>
            mVertexBuffers;
        DG::RefCntAutoPtr<DG::IBuffer>
            mIndexBuffer;
        std::unique_ptr<marl::Event> mEvent;

        inline GeometryBackend() :
            mEvent(std::make_unique<marl::Event>(
                marl::Event::Mode::Manual)) {    
        }
        inline GeometryBackend(const core::Geometry::Desc& desc) :
            mDesc(desc),
            mEvent(std::make_unique<marl::Event>(
                marl::Event::Mode::Manual)) {
        }
    };

    struct TextureBackend {
        DG::RefCntAutoPtr<DG::ITexture>
            mTexture;
        std::unique_ptr<marl::Event> mEvent;

        inline TextureBackend() : 
            mEvent(std::make_unique<marl::Event>(
                marl::Event::Mode::Manual)) {
        }
    };

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
        virtual void Update(core::ResourceManager* resourceManager) = 0;
        virtual bool IsIdle() = 0;
        virtual void WaitOnPendingTasks() = 0;
        virtual void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& view,
            const RenderCanvas& target,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) = 0;
        virtual void WaitUntilReady(core::SyncObject& obj) = 0;
        virtual void Shutdown() = 0;
        virtual void RegisterVertexFormats(core::VertexLayoutRegistry& registry) = 0;
        virtual void RegisterResourceInterfaces(core::ResourceManager& resourceInterface) = 0;

        void* GetBackend() override final;
    };

    class IOverlayModule : 
        public IRenderModule, 
        public IRenderCanvasAttachment {
    public:
        void* GetUserData() override final;
        void RegisterVertexFormats(core::VertexLayoutRegistry& registry) override final;
        void RegisterResourceInterfaces(core::ResourceManager& resourceInterface) override final;
    };
}