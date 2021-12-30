#pragma once

#include <okami/diligent/ShaderPreprocessor.hpp>

#include <EngineFactory.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <DeviceContext.h>
#include <SwapChain.h>
#include <BasicMath.hpp>

#include <string>

namespace okami::graphics::diligent {
	namespace DG = Diligent;

	class RawShader;

	struct ShaderParams {
		std::string mSource;
		DG::SHADER_TYPE mShaderType;
		std::string mName;		
		ShaderPreprocessorConfig mOverrides;
		std::string mEntryPoint;
		bool bCache = false;

		ShaderParams(const std::string& source,
			DG::SHADER_TYPE type,
			const std::string& name,
			const ShaderPreprocessorConfig& overrides = ShaderPreprocessorConfig(),
			const std::string& entryPoint = "main",
			bool cache = false) :
			mSource(source),
			mShaderType(type),
			mName(name),
			mOverrides(overrides),
			mEntryPoint(entryPoint),
			bCache(cache) {
		}
	};

	class RawShader {
	private:
		std::string mShaderSource;
		std::string mEntryPoint;
		std::string mName;
		DG::ShaderCreateInfo mCreateInfo;

	public:
		inline RawShader() {
		}

		inline RawShader(const ShaderPreprocessorOutput& preprocessorOutput,
			DG::SHADER_TYPE type, const std::string& name, const std::string& entryPoint) :
			mShaderSource(preprocessorOutput.mContent),
			mEntryPoint(entryPoint),
			mName(name) {
			mCreateInfo.Desc.ShaderType = type;
			mCreateInfo.SourceLanguage = DG::SHADER_SOURCE_LANGUAGE_HLSL;
			mCreateInfo.UseCombinedTextureSamplers = true;
		}

		DG::IShader* ToDiligent(DG::IRenderDevice* device);
	};

	DG::IShader* CompileShader(DG::IRenderDevice* device, 
		const ShaderPreprocessorOutput& preprocessorOutput,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint);

	DG::IShader* CompileEmbeddedShader(DG::IRenderDevice* device,
		const std::string& source,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint,
		core::IVirtualFileSystem* fileLoader,
		bool bAddLineNumbers,
        const ShaderPreprocessorConfig* config = nullptr);

	inline DG::IShader* CompileEmbeddedShader(DG::IRenderDevice* device,
		const ShaderParams& params,
		core::IVirtualFileSystem* fileLoader,
		bool bAddLineNumbers) {
		return CompileEmbeddedShader(device, params.mSource,
			params.mShaderType,
			params.mName,
			params.mEntryPoint,
			fileLoader,
			bAddLineNumbers,
            &params.mOverrides);
	}
}