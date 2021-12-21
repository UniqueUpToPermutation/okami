#pragma once

#include <okami/Resource.hpp>
#include <okami/ResourceInterface.hpp>
#include <okami/VertexFormat.hpp>

#include <glm/vec4.hpp>

namespace okami::core {

    enum class ResourceDimension {
        Buffer,
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray
    };

    inline bool IsArray(ResourceDimension dim) {
        if (dim == ResourceDimension::Texture1D ||
            dim == ResourceDimension::Texture2D ||
            dim == ResourceDimension::Texture3D) {
            return false;
        } else {
            return true;
        }
    }

    struct TextureFormat {
        uint32_t mChannels;
        ValueType mValueType;
        bool bNormalized;
        bool bLinear;

        static TextureFormat RGBA32_FLOAT();
        static TextureFormat RGBA32_UINT();
        static TextureFormat RGBA32_SINT();
        static TextureFormat RGB32_FLOAT();
        static TextureFormat RGB32_UINT();
        static TextureFormat RGB32_SINT();
        static TextureFormat RG32_FLOAT();
        static TextureFormat RG32_UINT();
        static TextureFormat RG32_SINT();
        static TextureFormat RGBA8_UINT();
        static TextureFormat RGBA8_UNORM();
        static TextureFormat RGBA8_SINT();
        static TextureFormat RGBA8_SNORM();
        static TextureFormat SRGBA8_UNORM();

        inline uint32_t GetPixelByteSize() const {
            return GetSize(mValueType) * mChannels;
        }
    };

    class Texture;

    template <>
	struct LoadParams<Texture> {
		bool bIsSRGB = false;
		bool bGenerateMips = true;
		
		inline LoadParams() {
		}

		inline LoadParams(
            bool isSRGB, 
			bool generateMips) :
			bIsSRGB(isSRGB),
			bGenerateMips(generateMips) {
		}

		template <class Archive>
		void save(Archive& archive) const {
			archive(bGenerateMips);
			archive(bIsSRGB);
		}

		template <class Archive>
		void load(Archive& archive) {
			archive(bGenerateMips);
			archive(bIsSRGB);
		}
	};

    uint MipCount(
		const uint width, 
		const uint height);

	uint MipCount(
		const uint width, 
		const uint height, 
		const uint depth);

	struct TextureSubResDataDesc {
		size_t mDepthStride;
		size_t mSrcOffset;
		size_t mStride;
        size_t mLength;
	};

    class Texture : public Resource {
    public:
        struct Desc {
            ResourceDimension mType;
            uint32_t mWidth = 1;
            uint32_t mHeight = 1;
            uint32_t mArraySizeOrDepth = 1;
            TextureFormat mFormat;
            uint32_t mMipLevels = 1;
            uint32_t mSampleCount = 1;

            inline uint32_t GetPixelByteSize() const {
                return mFormat.GetPixelByteSize();
            }

            uint32_t GetByteSize() const;
            std::vector<TextureSubResDataDesc> GetSubresourceDescs() const;

            uint32_t GetMipCount() const {
                if (mMipLevels == 0) {
                    return MipCount(mWidth, mHeight, mArraySizeOrDepth);
                } else 
                    return mMipLevels;
            }

            uint32_t GetDepth() const {
                if (IsArray(mType)) {
                    return 1u;
                } else {
                    return mArraySizeOrDepth;
                }
            }

            uint32_t GetArraySize() const {
                if (IsArray(mType)) {
                    return mArraySizeOrDepth;
                } else {
                    return 1u;
                }
            }
        };

        struct Data {
            Desc mDesc;
            std::vector<uint8_t> mData;

            void GenerateMips();

            static Data Alloc(const Desc& desc);

            static Data Load(
                const std::filesystem::path& path,
                const LoadParams<Texture>& params);

            inline void DeallocCPU() {
                mData.clear();
            }
        };
    
    private:
        Data mData;

    public:
        inline Data& DataCPU() {
            return mData;
        }

        inline const Data& DataCPU() const {
            return mData;
        }

        inline void Clear() {
            mData = Data();
        }

        inline void DeallocCPU() {
            mData.DeallocCPU();
        }

        inline Texture(Data&& data) :
            mData(std::move(data)) {
        }

        Texture() = default;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = default;
        Texture& operator=(Texture&&) = default;

        inline const Desc& GetDesc() const {
            return mData.mDesc;
        }

        entt::meta_type GetType() const override;

        static void Register();

        static Texture Load(
            const std::filesystem::path& path,
            const LoadParams<Texture>& params);

        struct Prefabs {
            static Texture SolidColor(
                uint width,
                uint height,
                const glm::vec4& color);
        };
    };
}