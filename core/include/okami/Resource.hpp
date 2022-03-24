#pragma once
#include <atomic>

#include <entt/entt.hpp>

#include <marl/event.h>

#include <okami/PlatformDefs.hpp>

#include <filesystem>

namespace okami {

	namespace core {
		template <typename T>
		struct LoadParams {
		};
	}

	typedef int64_t resource_id_t;
	constexpr resource_id_t INVALID_RESOURCE = -1;

	namespace core {
		class ResourceManager;
	}

    class Resource {
    private:
		resource_id_t mId = INVALID_RESOURCE;

	protected:
		inline void SetResourceId(resource_id_t value) {
			mId = value;
		}

    public:
		virtual ~Resource() = default;

		inline resource_id_t GetResourceId() const {
			return mId;
		}

		virtual entt::meta_type GetType() const = 0;
		virtual bool HasLoadParams() const = 0;
		virtual std::filesystem::path GetPath() const = 0;

		friend class core::ResourceManager;
    };
}