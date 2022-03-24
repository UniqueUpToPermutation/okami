#pragma once

#include <okami/Frame.hpp>
#include <okami/Transform.hpp>
#include <okami/ResourceBackend.hpp>
#include <okami/GraphicsComponents.hpp>

#include <okami/diligent/SpriteBatch.hpp>
#include <okami/diligent/GraphicsUtils.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/RenderModule.hpp>

namespace okami::graphics::diligent {

    template <typename backendT>
    using get_texture_impl_t = DG::ITexture*(*)(backendT*);

    template <typename textureBackendT, 
        get_texture_impl_t<textureBackendT> getTexture>
    class SpriteModule : public IRenderModule {
    private:
        SpriteBatchPipeline mPipeline;
        SpriteBatchState mState;
        SpriteBatch mBatch;
        core::ResourceBackend<
            core::Texture, textureBackendT>* mTextures;

        struct RenderCall  {
            DG::float3 mPosition;
            float mRotation;
            DG::float2 mScale;
            core::Sprite mSprite;
        };

    public:
        SpriteModule(core::ResourceBackend<
            core::Texture, textureBackendT>* textures) :
            mTextures(textures) {
        }

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device,
            const RenderModuleParams& params) override {

            core::InterfaceCollection interfaces(renderer);
            auto globalBuffersProvider = 
                interfaces.Query<IGlobalsBufferProvider>();
            auto renderPassFormatProvider = 
                interfaces.Query<IRenderPassFormatProvider>();

            if (!globalBuffersProvider) {
                throw std::runtime_error("Renderer does not implement"
                    " IGlobalsBufferProvider!");
            }

            if (!renderPassFormatProvider) {
                throw std::runtime_error("Renderer does not implement"
                    " IRenderPassFormatProvider!");
            }

            auto globals = globalBuffersProvider->GetGlobalsBuffer();

            auto colorBufferFormat = 
                renderPassFormatProvider->GetFormat(RenderAttribute::COLOR);
            auto depthBufferFormat = 
                renderPassFormatProvider->GetDepthFormat(RenderPass::Final());

            auto shaders = SpriteShaders::LoadDefaults(
                device, params.mFileSystem);
            mPipeline = SpriteBatchPipeline(
                device,
                *globals,
                shaders,
                colorBufferFormat,
                depthBufferFormat,
                1,
                DG::FILTER_TYPE_LINEAR);

            mState = SpriteBatchState(mPipeline);
            mBatch = SpriteBatch(device);
        }

        void Update(core::ResourceManager* resourceManager) override {
        }

        bool IsIdle() override {
            return true;
        }

        void WaitOnPendingTasks() override {
        }

        void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& rv,
            const RenderCanvas& target,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) override {
            auto view = frame.Registry().view
                <core::Transform, core::Sprite>();

            std::vector<RenderCall> calls;
            calls.reserve(view.size_hint());

            for (auto e : view) {
                const auto& transform = view.get<const core::Transform>(e);
                const auto& sprite = view.get<const core::Sprite>(e);
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
                return c1.mSprite.mTexture < c2.mSprite.mTexture;
            });
            std::stable_sort(calls.begin(), calls.end(),
                [](const RenderCall& c1, const RenderCall& c2) {
                return c1.mSprite.mLayer < c2.mSprite.mLayer;
            });

            mBatch.Begin(context, &mState);
            for (auto& c : calls) {
                auto& backend = mTextures->Get(c.mSprite.mTexture);
                DG::ITexture* texture = getTexture(&backend);

                const auto& desc = texture->GetDesc();
                DG::float2 size(desc.Width, desc.Height);
                DG::float2 scaledSize = c.mScale * size;
                
                SpriteRect rect{
                    DG::float2{0.0f, 0.0f},
                    size
                };

                mBatch.Draw(
                    texture, 
                    c.mPosition,
                    scaledSize, 
                    rect,
                    ToDiligent(c.mSprite.mOrigin), 
                    c.mRotation, 
                    ToDiligent(c.mSprite.mColor));
            }
            mBatch.End();
        }

        void WaitUntilReady(core::SyncObject& obj) override {
            obj.WaitUntilFinished<core::Transform>();
            obj.WaitUntilFinished<core::Sprite>();
        }

        void Shutdown() override {
            mBatch = SpriteBatch();
            mState = SpriteBatchState();
            mPipeline = SpriteBatchPipeline();
        }

        void RegisterVertexFormats(
            core::VertexLayoutRegistry& registry) override {
        }

        void RegisterResourceInterfaces(
            core::ResourceManager& resourceInterface) override {
        }
    };
}