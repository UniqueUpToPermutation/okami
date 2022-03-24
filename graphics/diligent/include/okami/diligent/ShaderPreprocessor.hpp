#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include <okami/Embed.hpp>
#include <okami/Hashers.hpp>

namespace okami::graphics::diligent {
	struct ShaderPreprocessorConfig {
		std::unordered_map<std::string, std::string> mDefines;
		std::vector<std::string> mHeaderItems;

		std::string Stringify(const ShaderPreprocessorConfig* overrides) const;
	};

	struct ShaderPreprocessorOutput {
		std::vector<std::string> mSources;
		std::string mContent;
	};
	
	class ShaderPreprocessor {
	private:
		static void Load(const std::filesystem::path& source,
			core::IVirtualFileSystem* fileLoader,
			const ShaderPreprocessorConfig* defaults,
			const ShaderPreprocessorConfig* overrides,
			std::stringstream* streamOut,
			ShaderPreprocessorOutput* output,
			bool bAddLineNumbers,
			std::unordered_set<std::filesystem::path, core::PathHash>* alreadyVisited);

	public:
		static void Load(const std::filesystem::path& source,
			core::IVirtualFileSystem* fileLoader,
			ShaderPreprocessorOutput* output, 
			const ShaderPreprocessorConfig* defaults, 
			bool bAddLineNumbers,
			const ShaderPreprocessorConfig* overrides = nullptr);
	};
}