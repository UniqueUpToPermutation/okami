#pragma once

#include <okami/Frame.hpp>
#include <okami/Embed.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/diligent/GraphicsUtils.hpp>
#include <okami/diligent/Buffers.hpp>
#include <okami/diligent/SceneGlobals.hpp>

#include <BasicMath.hpp>
#include <RefCntAutoPtr.hpp>

#define DEFAULT_IM3D_BUFFER_SIZE 2000u

namespace DG = Diligent;

#include <im3d.h>

namespace okami::graphics::diligent {

	class Camera;

	struct Im3dShaders {
		DG::RefCntAutoPtr<DG::IShader> mTrianglesVS;
		DG::RefCntAutoPtr<DG::IShader> mOtherVS;
		DG::RefCntAutoPtr<DG::IShader> mPointsGS;
		DG::RefCntAutoPtr<DG::IShader> mLinesGS;
		DG::RefCntAutoPtr<DG::IShader> mTrianglesPS;
		DG::RefCntAutoPtr<DG::IShader> mLinesPS;
		DG::RefCntAutoPtr<DG::IShader> mPointsPS;

		static Im3dShaders LoadDefault(
            DG::IRenderDevice* device, 
            core::IVirtualFileSystem* system);
	};

	struct Im3dPipeline {
		DG::RefCntAutoPtr<DG::IPipelineState> mPipelineStateVertices;
		DG::RefCntAutoPtr<DG::IPipelineState> mPipelineStateLines;
		DG::RefCntAutoPtr<DG::IPipelineState> mPipelineStateTriangles;
		DG::RefCntAutoPtr<DG::IShaderResourceBinding> mVertexSRB;
		DG::RefCntAutoPtr<DG::IShaderResourceBinding> mLinesSRB;
		DG::RefCntAutoPtr<DG::IShaderResourceBinding> mTriangleSRB;
		Im3dShaders mShaders;

		Im3dPipeline() = default;
		Im3dPipeline(DG::IRenderDevice* device,
			DynamicUniformBuffer<HLSL::SceneGlobals>& globals,
			DG::TEXTURE_FORMAT backbufferColorFormat,
			DG::TEXTURE_FORMAT backbufferDepthFormat,
			uint samples,
			Im3dShaders& shaders,
			bool bDepthEnable);
	};

	class Im3dModule {
	private:
		DG::RefCntAutoPtr<DG::IBuffer> mGeometryBuffer;
		uint mBufferSize;

	public:
		Im3dModule() = default;
		Im3dModule(DG::IRenderDevice* device,
			uint bufferSize = DEFAULT_IM3D_BUFFER_SIZE);

		void Draw(DG::IDeviceContext* deviceContext,
			Im3dPipeline& state,
			Im3d::Context* im3dContext = &Im3d::GetContext());

		inline uint GetBufferSize() const {
			return mBufferSize;
		}
	};
}