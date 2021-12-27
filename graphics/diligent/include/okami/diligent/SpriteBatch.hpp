#pragma once

#include <okami/Resource.hpp>
#include <okami/Embed.hpp>

#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/SceneGlobals.hpp>

#include <RenderDevice.h>
#include <RefCntAutoPtr.hpp>
#include <BasicMath.hpp>

#define DEFAULT_SPRITE_BATCH_SIZE 1000

namespace okami::graphics::diligent {

	struct SpriteBatchVSInput
	{
		// Use W component as rotation
		DG::float4 mPos;
		DG::float4 mColor;

		DG::float2 mUVTop;
		DG::float2 mUVBottom;

		DG::float2 mSize;
		DG::float2 mOrigin;
	};


    namespace DG = Diligent;

    struct SpriteShaders {
		DG::RefCntAutoPtr<DG::IShader> mVS;
		DG::RefCntAutoPtr<DG::IShader> mGS;
		DG::RefCntAutoPtr<DG::IShader> mPS;

		SpriteShaders() = default;

		inline SpriteShaders(
            DG::IShader* vs, 
			DG::IShader* gs, 
			DG::IShader* ps) {
            mVS.Attach(vs);
            mGS.Attach(gs);
            mPS.Attach(ps);
		}

		static SpriteShaders LoadDefaults(
			DG::IRenderDevice* device, 
			core::IVirtualFileSystem* system);
	};

    struct SpriteBatchPipeline {
	    DG::RefCntAutoPtr<DG::IPipelineState> mPipeline;
		SpriteShaders mShaders;

		SpriteBatchPipeline() = default;
		SpriteBatchPipeline(DG::IRenderDevice* device,
			DynamicUniformBuffer<HLSL::SceneGlobals>& globals,
            SpriteShaders& shaders,
			DG::TEXTURE_FORMAT backbufferFormat,
			DG::TEXTURE_FORMAT depthbufferFormat,
			uint samples,
			DG::FILTER_TYPE filterType);
    };

	struct SpriteBatchState {
		DG::IShaderResourceVariable* mTextureVariable = nullptr;
		DG::RefCntAutoPtr<DG::IShaderResourceBinding> mShaderBinding;
		DG::RefCntAutoPtr<DG::IPipelineState> mPipeline;

		SpriteBatchState() = default;
		SpriteBatchState(SpriteBatchPipeline& pipeline);
	};

    struct SpriteRect {
		DG::float2 mPosition;
		DG::float2 mSize;

		inline SpriteRect() {
		}

		inline SpriteRect(const DG::float2& position, const DG::float2& size) :
			mPosition(position),
			mSize(size) {
		}

		inline SpriteRect(float upperX, float upperY, float sizeX, float sizeY) :
			mPosition(upperX, upperY),
			mSize(sizeX, sizeY) {
		}
	};

    struct SpriteBatchCall {
		DG::ITexture* mTexture; 
		DG::float3 mPosition;
		DG::float2 mSize;
		SpriteRect mRect;
		DG::float2 mOrigin; 
		float mRotation;
		DG::float4 mColor;
	};

    class SpriteBatch {
	private:
		DG::RefCntAutoPtr<DG::IBuffer> mBuffer;

		SpriteBatchState mCurrentState;

		DG::ITexture* mLastTexture = nullptr;
		DG::IDeviceContext* mCurrentContext = nullptr;

		uint mWriteIndex;
		uint mBatchSize;
		uint mBatchSizeBytes;

		DG::MapHelper<SpriteBatchVSInput> mMapHelper;

	public:
		SpriteBatch() = default;
		SpriteBatch(DG::IRenderDevice* device,
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE);
	
		void Begin(DG::IDeviceContext* context, 
			const SpriteBatchState* state);
		void Flush();
		void End();

		void Draw(const SpriteBatchCall sprites[], size_t count);
		void Draw(DG::ITexture* texture, const DG::float3& pos,
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color);

		inline void Draw(DG::ITexture* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float3& pos) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect) {
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), dimensions, 
            SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), dimensions, 
            SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos,
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, DG::float3(pos.x, pos.y, 0.0f), rect.mSize, rect,
				origin, rotation, color); 
		}
	};
}