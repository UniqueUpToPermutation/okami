#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/Resource.hpp>
#include <okami/Texture.hpp>

#include <glm/vec3.hpp>

namespace okami::core {
    class BaseMaterial;

    template <>
    struct LoadParams<BaseMaterial> {
    };

    enum class SurfaceType {
        FLAT,
        LAMBERT,
        PHONG,
        COOK_TORRENCE
    };

    struct MetaSurfaceDesc {
        resource_id_t mAlbedo = INVALID_RESOURCE;
        resource_id_t mRoughness = INVALID_RESOURCE;
        resource_id_t mMetallic = INVALID_RESOURCE;
        resource_id_t mNormal = INVALID_RESOURCE;

        glm::vec3 mAlbedoFactor = glm::vec3(1.0f, 1.0f, 1.0f);
        float mRoughnessFactor = 1.0f;
        float mMetallicFactor = 0.0f;
        float mSpecularPower = 1.0f;

        SurfaceType mType = SurfaceType::FLAT;
    };

    struct FlatSurface {
        resource_id_t mTexture = INVALID_RESOURCE;
        glm::vec3 mTextureColor = glm::vec3(1.0f, 1.0f, 1.0f);

        MetaSurfaceDesc ToMeta() const {
            MetaSurfaceDesc desc;
            desc.mAlbedo = mTexture;
            desc.mAlbedoFactor = mTextureColor;
            desc.mType = SurfaceType::FLAT;
            return desc;
        };
    };
    
    struct LambertSurface {
        resource_id_t mAlbedo = INVALID_RESOURCE;
        resource_id_t mNormal = INVALID_RESOURCE;
        glm::vec3 mAlbedoFactor = glm::vec3(1.0f, 1.0f, 1.0f);

        MetaSurfaceDesc ToMeta() const {
            MetaSurfaceDesc desc;
            desc.mAlbedo = mAlbedo;
            desc.mNormal = mNormal;
            desc.mAlbedoFactor = mAlbedoFactor;
            desc.mType = SurfaceType::LAMBERT;
            return desc;
        };
    };

    struct PhongSurface {
        resource_id_t mAlbedo = INVALID_RESOURCE;
        resource_id_t mNormal = INVALID_RESOURCE;
        resource_id_t mSpecular = INVALID_RESOURCE;

        glm::vec3 mAlbedoFactor = glm::vec3(1.0f, 1.0f, 1.0f);
        float mSpecularPower = 1.0f;
        float mSpecularFactor = 1.0f;

        MetaSurfaceDesc ToMeta() const {
            MetaSurfaceDesc desc;
            desc.mAlbedo = mAlbedo;
            desc.mNormal = mNormal;
            desc.mRoughness = mSpecular;
            desc.mAlbedoFactor = mAlbedoFactor;
            desc.mSpecularPower = mSpecularPower;
            desc.mRoughnessFactor = 1.0 - mSpecularFactor;
            desc.mType = SurfaceType::PHONG;
            return desc;
        }
    };

    template <typename GeometryType, 
        typename Surface = MetaSurfaceDesc>
    class Material : public Resource {
    private:
        Surface mSurface;

    public:
        template <typename SurfaceType_>
        Material(const SurfaceType_& surface) :
            mSurface(surface.ToMeta()) {
        }

        Material() = default;
        Material(const Material&) = default;
        Material(Material&&) = default;

        Material& operator=(const Material&) = default;
        Material& operator=(Material&&) = default;
        
        const Surface& GetSurface() const {
            return mSurface;
        }

        entt::meta_type GetType() const override {
            return entt::resolve<Material<GeometryType, SurfaceType>>();
        }

		bool HasLoadParams() const override {
            return false;
        }

		std::filesystem::path GetPath() const override {
            throw std::runtime_error("No path for material!");
        }

        LoadParams<Material<GeometryType, Surface>> GetLoadParams() {
            throw std::runtime_error("No load params for material!");
        }
    };
}