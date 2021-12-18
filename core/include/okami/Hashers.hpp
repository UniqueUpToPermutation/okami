#pragma once

#include <filesystem>
#include <entt/entt.hpp>

namespace okami::core {
    struct PathHash {
        std::size_t operator()(const std::filesystem::path& p) const noexcept {
            return std::filesystem::hash_value(p);
        }
    };

    struct TypeHash {
        std::size_t operator()(const entt::meta_type& t) const noexcept {
            return std::hash<entt::id_type>()(t.id());
        }
    };
}