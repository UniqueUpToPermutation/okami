#include <okami/diligent/SpriteBatch.hpp>
#include <okami/diligent/Shader.hpp>

namespace okami::graphics::diligent {
    SpriteShaders SpriteShaders::LoadDefaults(
			DG::IRenderDevice* device, 
			core::IVirtualFileSystem* system) {

		ShaderParams vsParams("SpriteBatch/SpriteBatch.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Sprite Batch VS");

		ShaderParams gsParams("SpriteBatch/SpriteBatch.gsh",
			DG::SHADER_TYPE_GEOMETRY,
			"Sprite Batch GS");

		ShaderParams psParams("SpriteBatch/SpriteBatch.psh",
			DG::SHADER_TYPE_PIXEL,
			"Sprite Batch PS");

        SpriteShaders shaders;
		shaders.mVS.Attach(CompileEmbeddedShader(device, vsParams, system, false));
		shaders.mGS.Attach(CompileEmbeddedShader(device, gsParams, system, false));
		shaders.mPS.Attach(CompileEmbeddedShader(device, psParams, system, false));
		return shaders;
	}

    SpriteBatchPipeline::SpriteBatchPipeline(
		DG::IRenderDevice* device,
        DynamicUniformBuffer<HLSL::SceneGlobals>& globals,
        SpriteShaders& shaders,
        DG::TEXTURE_FORMAT backbufferFormat,
        DG::TEXTURE_FORMAT depthbufferFormat,
        uint samples,
        DG::FILTER_TYPE filterType) {

		DG::SamplerDesc SamDesc
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		DG::IPipelineState* result = nullptr;

		// Create Irradiance Pipeline
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Sprite Batch Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = backbufferFormat;
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
		GraphicsPipeline.DSVFormat 					  = depthbufferFormat;

		DG::RenderTargetBlendDesc blendState;
		blendState.BlendEnable = true;
		blendState.BlendOp = DG::BLEND_OPERATION_ADD;
		blendState.BlendOpAlpha = DG::BLEND_OPERATION_ADD;
		blendState.DestBlend = DG::BLEND_FACTOR_INV_SRC_ALPHA;
		blendState.SrcBlend = DG::BLEND_FACTOR_SRC_ALPHA;
		blendState.DestBlendAlpha = DG::BLEND_FACTOR_ONE;
		blendState.SrcBlendAlpha = DG::BLEND_FACTOR_ONE;

		GraphicsPipeline.BlendDesc.RenderTargets[0] = blendState;

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = 1;

		uint stride = sizeof(SpriteBatchVSInput);

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 4, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, 
                stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 4, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, 
                stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, 
                stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(3, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, 
                stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(4, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, 
                stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(5, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, 
                stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
		};

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = shaders.mVS.RawPtr();
		PSOCreateInfo.pGS = shaders.mGS.RawPtr();
		PSOCreateInfo.pPS = shaders.mPS.RawPtr();

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		// clang-format off
		DG::ShaderResourceVariableDesc Vars[] = 
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables    = Vars;

		// clang-format off
		DG::ImmutableSamplerDesc ImtblSamplers[] =
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture", SamDesc}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);
		
		mPipeline = result;
		mShaders = shaders;

		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "cbuf_Globals")
			->Set(globals.Get());

		result->Release();
	}

    SpriteBatchState::SpriteBatchState(SpriteBatchPipeline& pipeline) {
        mPipeline = pipeline.mPipeline;

        DG::IShaderResourceBinding* binding = nullptr;
		mPipeline->CreateShaderResourceBinding(&binding, true);
        mShaderBinding.Attach(binding);

		mTextureVariable = 
			binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture");
    }

    SpriteBatch::SpriteBatch(DG::IRenderDevice* device,
		uint batchSize) {

		mBatchSize = batchSize;
		mBatchSizeBytes = batchSize * sizeof(SpriteBatchVSInput);

		DG::BufferDesc desc;
		desc.Name = "Sprite Batch Buffer";
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.Size = mBatchSizeBytes;
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

		DG::IBuffer* buf = nullptr;
		device->CreateBuffer(desc, nullptr, &buf);
		mBuffer.Attach(buf);
	}

    void SpriteBatch::Begin(DG::IDeviceContext* context, const SpriteBatchState* state) {
		mCurrentState = *state;

		context->SetPipelineState(mCurrentState.mPipeline);

		DG::IBuffer* buffers[] = { mBuffer };
		DG::Uint64 offsets[] = { 0 };

		context->SetVertexBuffers(0, 1, buffers, offsets, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, 
			DG::SET_VERTEX_BUFFERS_FLAG_RESET);

		mMapHelper.Map(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);

		mWriteIndex = 0;
		mCurrentContext = context;
		mLastTexture = nullptr;
	}

    void SpriteBatch::Flush() {
		if (mWriteIndex > 0) {
			DG::DrawAttribs attribs;
			attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
			attribs.StartVertexLocation = 0;
			attribs.NumVertices = mWriteIndex;
			
			mMapHelper.Unmap();
			mCurrentState.mTextureVariable->Set(
				mLastTexture->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE));
			mCurrentContext->CommitShaderResources(mCurrentState.mShaderBinding, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			mCurrentContext->Draw(attribs);

			mMapHelper.Map(mCurrentContext, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			
			mWriteIndex = 0;
		}
	}

    void SpriteBatch::End() {
		Flush();
		mMapHelper.Unmap();
		mCurrentState = SpriteBatchState();
	}

	void SpriteBatch::Draw(DG::ITexture* texture, 
        const DG::float3& pos,
		const DG::float2& size, 
        const SpriteRect& rect, 
		const DG::float2& origin, 
        const float rotation, 
		const DG::float4& color) {

		if (!mLastTexture)
			mLastTexture = texture;
		if (mWriteIndex == mBatchSize || (mLastTexture && mLastTexture != texture)) {
			Flush();
			mLastTexture = texture;
		}

		SpriteBatchVSInput* instance = &mMapHelper[mWriteIndex];
		auto& desc = texture->GetDesc();
		DG::float2 dim2d(desc.Width, desc.Height);
		
		DG::float2 uvtop_unscaled(rect.mPosition.x - 0.5f, rect.mPosition.y - 0.5f);
		auto uvbottom_unscaled = uvtop_unscaled + rect.mSize;

		instance->mPos.x = pos.x;
		instance->mPos.y = pos.y;
		instance->mPos.z = pos.z;
		instance->mPos.w = rotation;

		instance->mColor = color;
		instance->mOrigin = origin;
		instance->mSize = size;
		instance->mUVTop = uvtop_unscaled / dim2d;
		instance->mUVBottom = uvbottom_unscaled / dim2d;

		++mWriteIndex;
	}

    void SpriteBatch::Draw(const SpriteBatchCall sprites[], size_t count) {
		for (size_t i = 0; i < count; ++i) {
			auto& sprite = sprites[i];
			Draw(sprite.mTexture, sprite.mPosition, sprite.mSize, sprite.mRect,
				sprite.mOrigin, sprite.mRotation, sprite.mColor);
		}
	}
}