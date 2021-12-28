#pragma once

#include <okami/Frame.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/diligent/SpriteBatch.hpp>
#include <okami/diligent/GraphicsUtils.hpp>

namespace okami::graphics::diligent {
    typedef DG::ITexture*(*get_texture_impl_t)(void*);

    class SpriteModule {
    private:
        SpriteBatchPipeline mPipeline;
        SpriteBatchState mState;
        SpriteBatch mBatch;

        struct RenderCall  {
            DG::float3 mPosition;
            float mRotation;
            DG::float2 mScale;
            core::Sprite mSprite;
        };

    public:
        SpriteModule() = default;

        void Startup(DG::IRenderDevice* device, 
            DG::ISwapChain* swapChain,
            DynamicUniformBuffer<HLSL::SceneGlobals>& globals,
            core::IVirtualFileSystem* vfilesystem);
        void RequestSync(core::SyncObject& syncObject);
        void Shutdown();

        template <get_texture_impl_t GetImpl>
        void Render(DG::IDeviceContext* context,  
            core::Frame& frame,
            core::SyncObject& syncObject) {

            syncObject.WaitUntilFinished<core::Transform>();
            syncObject.WaitUntilFinished<core::Sprite>();

            auto view = frame.Registry().view
                <core::Transform, core::Sprite>();

            std::vector<RenderCall> calls;
            calls.reserve(view.size_hint());

            for (auto e : view) {
                const auto& transform = view.get<core::Transform>(e);
                const auto& sprite = view.get<core::Sprite>(e);
                const auto& rot = transform.mRotation;

                RenderCall call;
                call.mPosition = ToDiligent(transform.mTranslation);
                call.mRotation = 
                    std::atan2(rot.w, rot.z) * 2.0;
                call.mScale.x = transform.mScale.x;
                call.mScale.y = transform.mScale.y;
                call.mSprite = sprite;

                calls.emplace_back(std::move(call));
            }

            std::sort(calls.begin(), calls.end(), 
                [](const RenderCall& c1, const RenderCall& c2) {
                return c1.mSprite.mTexture.Ptr() < c2.mSprite.mTexture.Ptr();
            });
            std::stable_sort(calls.begin(), calls.end(),
                [](const RenderCall& c1, const RenderCall& c2) {
                return c1.mSprite.mLayer < c2.mSprite.mLayer;
            });

            mBatch.Begin(context, &mState);
            for (auto& c : calls) {
                const auto& desc = c.mSprite.mTexture->GetDesc();
                DG::float2 size(desc.mWidth, desc.mHeight);
                DG::float2 scaledSize = c.mScale * size;
                
                SpriteRect rect{
                    DG::float2{0.0f, 0.0f},
                    size
                };

                mBatch.Draw(
                    GetImpl(c.mSprite.mTexture->GetBackend()), 
                    c.mPosition,
                    scaledSize, 
                    rect,
                    ToDiligent(c.mSprite.mOrigin), 
                    c.mRotation, 
                    ToDiligent(c.mSprite.mColor));
            }
            mBatch.End();
        }
    };
}