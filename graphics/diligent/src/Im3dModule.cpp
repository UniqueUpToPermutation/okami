#include <okami/diligent/Im3dModule.hpp>

#include <MapHelper.hpp>

#include <okami/diligent/Shader.hpp>
#include <okami/diligent/ShaderPreprocessor.hpp>

namespace okami::graphics::diligent {
    
	Im3dShaders Im3dShaders::LoadDefault(DG::IRenderDevice* device, core::IVirtualFileSystem* system) {

        Im3dShaders result;

		ShaderPreprocessorConfig vsTrianglesConfig;
		vsTrianglesConfig.mDefines["TRIANGLES"] = "1";

		ShaderParams vsTrianglesParams(
			"Im3d/shaders/Im3dVS.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Triangle VS",
			vsTrianglesConfig,
			"main");

		ShaderPreprocessorConfig vsPointsConfig;
		vsPointsConfig.mDefines["POINTS"] = "1";

		ShaderParams vsPointsParams(
			"Im3d/shaders/Im3dVS.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Points VS",
			vsPointsConfig,
			"main");

		ShaderPreprocessorConfig vsLinesConfig;
		vsLinesConfig.mDefines["LINES"] = "1";

		ShaderParams vsLinesParams(
			"Im3d/shaders/Im3dVS.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Lines VS",
			vsLinesConfig,
			"main");

		ShaderPreprocessorConfig gsPtConfig;
		gsPtConfig.mDefines["POINTS"] = "1";

		ShaderParams gsPointsParams(
			"Im3d/shaders/Im3dGS.hlsl",
			DG::SHADER_TYPE_GEOMETRY,
			"Im3d Point GS",
			gsPtConfig,
			"main_points"
		);

		ShaderPreprocessorConfig gsLineConfig;
		gsLineConfig.mDefines["LINES"] = "1";

		ShaderParams gsLinesParams(
			"Im3d/shaders/Im3dGS.hlsl",
			DG::SHADER_TYPE_GEOMETRY,
			"Im3d Line GS",
			gsLineConfig,
			"main_lines"
		);

		ShaderPreprocessorConfig psTriangleConfig;
		psTriangleConfig.mDefines["TRIANGLES"] = "1";

		ShaderParams psTriangleParams(
			"Im3d/shaders/Im3dPS.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Triangle PS",
			psTriangleConfig,
			"main_tris"
		);

		ShaderPreprocessorConfig psLinesConfig;
		psLinesConfig.mDefines["LINES"] = "1";

		ShaderParams psLinesParams(
			"Im3d/shaders/Im3dPS.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Lines PS",
			psLinesConfig,
			"main_lines"
		);

		ShaderPreprocessorConfig psPointConfig;
		psPointConfig.mDefines["POINTS"] = "1";

		ShaderParams psPointParams(
			"Im3d/shaders/Im3dPS.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Point PS",
			psPointConfig,
			"main_points"
		);
        
        bool bPrintLines = false;

		result.mTrianglesVS = CompileEmbeddedShader(device, vsTrianglesParams, system, bPrintLines);
		result.mPointsVS = CompileEmbeddedShader(device, vsPointsParams, system, bPrintLines);
		result.mLinesVS = CompileEmbeddedShader(device, vsLinesParams, system, bPrintLines);
		result.mPointsGS = CompileEmbeddedShader(device, gsPointsParams, system, bPrintLines);
		result.mLinesGS = CompileEmbeddedShader(device, gsLinesParams, system, bPrintLines);
		result.mTrianglesPS = CompileEmbeddedShader(device, psTriangleParams, system, bPrintLines);
		result.mLinesPS = CompileEmbeddedShader(device, psLinesParams, system, bPrintLines);
		result.mPointsPS = CompileEmbeddedShader(device, psPointParams, system, bPrintLines);

		return result;
	}

	Im3dPipeline::Im3dPipeline(DG::IRenderDevice* device,
		DynamicUniformBuffer<HLSL::SceneGlobals>& globals,
		DG::TEXTURE_FORMAT backbufferColorFormat,
		DG::TEXTURE_FORMAT backbufferDepthFormat,
		uint samples,
		Im3dShaders& shaders,
		bool bDepthEnable) : mShaders(shaders) {

		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Im3d Triangle Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = backbufferColorFormat;
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = bDepthEnable;
		GraphicsPipeline.DSVFormat 					  = backbufferDepthFormat;

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
		GraphicsPipeline.SmplDesc.Count = samples;

		size_t stride = sizeof(Im3d::VertexData);
		size_t position_offset = offsetof(Im3d::VertexData, m_positionSize);
		size_t color_offset = offsetof(Im3d::VertexData, m_color);

		std::vector<DG::LayoutElement> layoutElements = {
			// Position
			DG::LayoutElement(0, 0, 4, DG::VT_FLOAT32, false, 
				position_offset, stride, 
				DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			// Color
			DG::LayoutElement(1, 0, 4, DG::VT_UINT8, true, 
				color_offset, stride, 
				DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
		};

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = shaders.mTrianglesVS.RawPtr();
		PSOCreateInfo.pGS = nullptr;
		PSOCreateInfo.pPS = shaders.mTrianglesPS.RawPtr();

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		DG::IPipelineState* pipelineStateTris = nullptr;
		device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineStateTris);
		mPipelineStateTriangles.Attach(pipelineStateTris);

		auto contextData = mPipelineStateTriangles->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData");
		contextData->Set(globals.Get());

		// Line Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_LINE_LIST;
		PSOCreateInfo.pVS = shaders.mLinesVS;
		PSOCreateInfo.pGS = shaders.mLinesGS;
		PSOCreateInfo.pPS = shaders.mLinesPS;
		PSODesc.Name = "Im3d Lines Pipeline";

		DG::IPipelineState* pipelineStateLines = nullptr;
		device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineStateLines);
		mPipelineStateLines.Attach(pipelineStateLines);

		contextData = mPipelineStateLines->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData");
		contextData->Set(globals.Get());

		contextData = mPipelineStateLines->GetStaticVariableByName(
			DG::SHADER_TYPE_GEOMETRY, "cbContextData");
		contextData->Set(globals.Get());				

		// Point Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		PSOCreateInfo.pVS = shaders.mPointsVS;
		PSOCreateInfo.pGS = shaders.mPointsGS;
		PSOCreateInfo.pPS = shaders.mPointsPS;
		PSODesc.Name = "Im3d Points Pipeline";

		DG::IPipelineState* pipelineStateVertices = nullptr;
		device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineStateVertices);
		mPipelineStateVertices.Attach(pipelineStateVertices);

		contextData = mPipelineStateVertices->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData");
		contextData->Set(globals.Get());

		contextData = mPipelineStateVertices->GetStaticVariableByName(
			DG::SHADER_TYPE_GEOMETRY, "cbContextData");
		contextData->Set(globals.Get());

		mShaders = shaders;

		DG::IShaderResourceBinding* vertBinding = nullptr;
		mPipelineStateVertices->CreateShaderResourceBinding(&vertBinding, true);
		mVertexSRB.Attach(vertBinding);

		DG::IShaderResourceBinding* lineBinding = nullptr;
		mPipelineStateLines->CreateShaderResourceBinding(&lineBinding, true);
		mLinesSRB.Attach(lineBinding);

		DG::IShaderResourceBinding* triangleBinding = nullptr;
		mPipelineStateTriangles->CreateShaderResourceBinding(&triangleBinding, true);
		mTriangleSRB.Attach(triangleBinding);
	}

	Im3dModule::Im3dModule(DG::IRenderDevice* device, 
			uint bufferSize) :
		mGeometryBuffer(nullptr),
		mBufferSize(bufferSize) {

		DG::BufferDesc CBDesc;
		CBDesc.Name 			= "Im3d Geometry Buffer";
		CBDesc.Size 			= sizeof(Im3d::VertexData) * bufferSize;
		CBDesc.Usage 			= DG::USAGE_DYNAMIC;
		CBDesc.BindFlags 		= DG::BIND_VERTEX_BUFFER;
		CBDesc.CPUAccessFlags	= DG::CPU_ACCESS_WRITE;

		DG::IBuffer* geoBuf = nullptr;
		device->CreateBuffer(CBDesc, nullptr, &geoBuf);
		mGeometryBuffer.Attach(geoBuf);
	}

	void Im3dModule::Draw(DG::IDeviceContext* deviceContext,
		Im3dPipeline& pipeline,
		Im3d::Context* im3dContext) {
		uint drawListCount = im3dContext->getDrawListCount();

		DG::RefCntAutoPtr<DG::IPipelineState> currentPipelineState;

		DG::Uint64 offsets[] = {0};
		DG::IBuffer* vBuffers[] = { mGeometryBuffer.RawPtr()};

		deviceContext->SetVertexBuffers(0, 1, 
			vBuffers, offsets, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			DG::SET_VERTEX_BUFFERS_FLAG_RESET);

		for (uint i = 0; i < drawListCount; ++i) {
			auto drawList = &im3dContext->getDrawLists()[i];

			switch (drawList->m_primType) {
				case Im3d::DrawPrimitiveType::DrawPrimitive_Triangles:
					if (currentPipelineState != pipeline.mPipelineStateTriangles) {
						currentPipelineState = pipeline.mPipelineStateTriangles;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(pipeline.mTriangleSRB.RawPtr(), 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				case Im3d::DrawPrimitive_Lines:
					if (currentPipelineState != pipeline.mPipelineStateLines) {
						currentPipelineState = pipeline.mPipelineStateLines;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(pipeline.mLinesSRB.RawPtr(), 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				case Im3d::DrawPrimitive_Points:
					if (currentPipelineState != pipeline.mPipelineStateVertices) {
						currentPipelineState = pipeline.mPipelineStateVertices;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(pipeline.mVertexSRB.RawPtr(), 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				default:
					break;
			}

			uint currentIndx = 0;
			while (currentIndx < drawList->m_vertexCount) {
				uint vertsToRender = std::min(mBufferSize, drawList->m_vertexCount - currentIndx);

				{
					DG::MapHelper<Im3d::VertexData> vertexMap(deviceContext, mGeometryBuffer, 
						DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
					std::memcpy(vertexMap, &drawList->m_vertexData[currentIndx],
						sizeof(Im3d::VertexData) * vertsToRender);
				}

				DG::DrawAttribs drawAttribs;
				drawAttribs.NumVertices = vertsToRender;
				drawAttribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;

				deviceContext->Draw(drawAttribs);

				currentIndx += vertsToRender;
			}
		}
	}
}