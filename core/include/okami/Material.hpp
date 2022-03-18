#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/Resource.hpp>
#include <okami/Texture.hpp>

namespace okami::core {
    class BaseMaterial;

    template <>
    struct LoadParams<BaseMaterial> {
    };

    class BaseMaterial : public Resource {
    public:
        struct Data {
            resource_id_t mAlbedo = INVALID_RESOURCE;
            glm::vec4 mAlbedoFactor = 
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        };
    
    private:
        Data mData;

    public:
        inline const Data& GetData() const {
            return mData;
        }

        BaseMaterial() = default;

        inline BaseMaterial(const Data& data) :
            mData(data) {
        }

        entt::meta_type GetType() const override;
        bool HasLoadParams() const override;
		std::filesystem::path GetPath() const override;
    
        static void Register();
    };
}