#include <okami/Frame.hpp>
#include <okami/Embed.hpp>

using namespace entt;

namespace okami::core {
    bool EmbeddedFileLoader::Exists(const std::filesystem::path& source) const {
		auto searchSource = std::filesystem::relative(source, "");

		auto it = mInternalShaders.find(searchSource);
		if (it != mInternalShaders.end()) {
			return true;
		} else {
			return false;
		}
	}

	bool EmbeddedFileLoader::TryFind(const std::filesystem::path& source, std::string* contents) const {
		auto searchSource = std::filesystem::relative(source, "");
		
		// Search internal shaders first
		auto it = mInternalShaders.find(searchSource);
		if (it != mInternalShaders.end()) {
			*contents = it->second;
			return true;
		}
		return false;
	}

	void EmbeddedFileLoader::Add(const embedded_file_loader_t& factory) {
		factory(&mInternalShaders);
	}

    EmbeddedFileLoader::EmbeddedFileLoader(const embedded_file_loader_t& factory) {
        Add(factory);
    }
}