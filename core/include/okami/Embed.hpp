#pragma once

#include <unordered_map>
#include <filesystem>
#include <functional>

#include <okami/Hashers.hpp>

namespace okami::core {

	typedef std::unordered_map<std::filesystem::path, const char*, PathHash> file_map_t;
	typedef std::function<void(file_map_t*)> embedded_file_loader_t;

	class IVirtualFileSystem {
	public:
		virtual ~IVirtualFileSystem() = default;

		virtual bool Exists(const std::filesystem::path& source) const = 0;
		virtual bool TryFind(const std::filesystem::path& source, std::string* contents) const = 0;
	};

	class EmbeddedFileLoader : public IVirtualFileSystem {
	private:
		file_map_t mInternalShaders;

	public:
		void Add(const embedded_file_loader_t& factory);

        EmbeddedFileLoader() = default;
		~EmbeddedFileLoader() = default;

        EmbeddedFileLoader(const embedded_file_loader_t& factory);
		bool Exists(const std::filesystem::path& source) const override;
		bool TryFind(const std::filesystem::path& source, std::string* contents) const override;
	};
}