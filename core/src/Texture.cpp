#include <okami/Texture.hpp>
#include <okami/MipGenerator.hpp>

#include <cmath>
#include <cstring>

#include <lodepng.h>
#include <cmath>

using namespace entt;

namespace okami::core {

	TextureFormat TextureFormat::RGBA32_FLOAT() {
		return TextureFormat{
			4, ValueType::FLOAT32, false, true
		};
	}
    TextureFormat TextureFormat::RGBA32_UINT() {
		return TextureFormat{
			4, ValueType::UINT32, false, true
		};
	}
    TextureFormat TextureFormat::RGBA32_SINT() {
		return TextureFormat{
			4, ValueType::INT32, false, true
		};
	}
    TextureFormat TextureFormat::RGB32_FLOAT() {
		return TextureFormat{
			3, ValueType::FLOAT32, false, true
		};
	}
    TextureFormat TextureFormat::RGB32_UINT() {
		return TextureFormat{
			3, ValueType::UINT32, false, true
		};
	}
    TextureFormat TextureFormat::RGB32_SINT() {
		return TextureFormat{
			3, ValueType::INT32, false, true
		};
	}
    TextureFormat TextureFormat::RG32_FLOAT() {
		return TextureFormat{
			2, ValueType::FLOAT32, false, true
		};
	}
    TextureFormat TextureFormat::RG32_UINT() {
		return TextureFormat{
			2, ValueType::UINT32, false, true
		};
	}
    TextureFormat TextureFormat::RG32_SINT() {
		return TextureFormat{
			2, ValueType::INT32, false, true
		};
	}
    TextureFormat TextureFormat::RGBA8_UINT() {
		return TextureFormat{
			4, ValueType::UINT8, false, true
		};
	}
    TextureFormat TextureFormat::RGBA8_UNORM() {
		return TextureFormat{
			4, ValueType::UINT8, true, true
		};
	}
    TextureFormat TextureFormat::RGBA8_SINT() {
		return TextureFormat{
			4, ValueType::INT8, false, true
		};
	}
    TextureFormat TextureFormat::RGBA8_SNORM() {
		return TextureFormat{
			4, ValueType::INT8, true, true
		};
	}
    TextureFormat TextureFormat::SRGBA8_UNORM() {
		return TextureFormat{
			4, ValueType::UINT8, true, false
		};
	}

    uint MipCount(
		const uint width, 
		const uint height) {
		return (uint)(1 + std::floor(std::log2(std::max(width, height))));
	}

	uint MipCount(
		const uint width, 
		const uint height, 
		const uint depth) {
		return (uint)(1 + std::floor(
			std::log2(std::max(width, std::max(height, depth)))));
	}
    
    entt::meta_type Texture::GetType() const {
        return entt::resolve<Texture>();
    }

    void Texture::Register() {
        entt::meta<Texture>()
            .type("Texture"_hs);
    }

    std::vector<TextureSubResDataDesc> 
        Texture::Desc::GetSubresourceDescs() const {

        std::vector<TextureSubResDataDesc> descs;

        auto pixelSize = GetPixelByteSize();
		size_t mip_count = GetMipCount();

	    // Compute subresources and sizes
		size_t currentOffset = 0;
		for (size_t iarray = 0; iarray < mArraySizeOrDepth; ++iarray) {
			for (size_t imip = 0; imip < mip_count; ++imip) {
				size_t mip_width = mWidth;
				size_t mip_height = mHeight;
				size_t mip_depth = mArraySizeOrDepth;

				mip_width = std::max<size_t>(mip_width >> imip, 1u);
				mip_height = std::max<size_t>(mip_height >> imip, 1u);
				mip_depth = std::max<size_t>(mip_depth >> imip, 1u);

                size_t increment = mip_width * mip_height * 
                    mip_depth * pixelSize * mSampleCount;

                TextureSubResDataDesc subDesc;
				subDesc.mSrcOffset = currentOffset;
				subDesc.mDepthStride = mip_width * mip_height * pixelSize * mSampleCount;
				subDesc.mStride = mip_width * pixelSize * mSampleCount;
				subDesc.mLength = increment;
                descs.emplace_back(subDesc);
  
				currentOffset += increment;
			}
		}

        return descs;
    }

    uint32_t Texture::Desc::GetByteSize() const {
        auto pixelSize = GetPixelByteSize();
		size_t mip_count = GetMipCount();

	    // Compute subresources and sizes
		size_t currentOffset = 0;
		for (size_t iarray = 0; iarray < mArraySizeOrDepth; ++iarray) {
			for (size_t imip = 0; imip < mip_count; ++imip) {
				size_t mip_width = mWidth;
				size_t mip_height = mHeight;
				size_t mip_depth = mArraySizeOrDepth;

				mip_width = std::max<size_t>(mip_width >> imip, 1u);
				mip_height = std::max<size_t>(mip_height >> imip, 1u);
				mip_depth = std::max<size_t>(mip_depth >> imip, 1u);

                size_t increment = mip_width * mip_height * 
                    mip_depth * pixelSize * mSampleCount;
				currentOffset += increment;
			}
		}

        return currentOffset;
    }

    Texture::Data Texture::Data::Alloc(const Texture::Desc& desc) {
        size_t sz = desc.GetByteSize();

        Texture::Data result;
        result.mDesc = desc;
        result.mData.resize(sz);

        return result;
    }

    void Texture::Data::GenerateMips() {

        size_t mipCount = mDesc.mMipLevels;
		bool isSRGB = !mDesc.mFormat.bLinear;
		size_t pixelSize = mDesc.GetPixelByteSize();
		uint componentCount = mDesc.mFormat.mChannels * mDesc.mSampleCount;
		auto valueType = mDesc.mFormat.mValueType;

        auto subresources = mDesc.GetSubresourceDescs();

        size_t arrayLength = 1;

        switch (mDesc.mType) {
            case ResourceDimension::Texture2D:
                arrayLength = 1;
                break;
            case ResourceDimension::Texture2DArray:
            case ResourceDimension::TextureCube:
            case ResourceDimension::TextureCubeArray:
                arrayLength = mDesc.mArraySizeOrDepth;
                break;
            default:
                throw std::runtime_error("GenerateMips does not support this resource dimension type!");
        }

		uint iSubresource = 0;
		for (size_t arrayIndex = 0; arrayIndex < arrayLength; ++arrayIndex) {
			auto& baseSubDesc = subresources[iSubresource];
			auto last_mip_data = &mData[baseSubDesc.mSrcOffset];
			++iSubresource;

			for (size_t i = 1; i < mipCount; ++i) {
				auto& subDesc = subresources[iSubresource];
				auto new_mip_data = &mData[subDesc.mSrcOffset];

				uint fineWidth = std::max<uint>(1u, mDesc.mWidth >> (i - 1));
				uint fineHeight = std::max<uint>(1u, mDesc.mHeight >> (i - 1));
				uint coarseWidth = std::max<uint>(1u, mDesc.mWidth >> i);
				uint coarseHeight = std::max<uint>(1u, mDesc.mHeight >> i);

				uint fineStride = fineWidth * pixelSize;
				uint coarseStride = coarseWidth * pixelSize;

				mip_generator_2d_t mip_gen;

				switch (valueType) {
					case ValueType::UINT8:
						mip_gen = &ComputeCoarseMip2D<uint8_t>;
						break;
					case ValueType::UINT16:
						mip_gen = &ComputeCoarseMip2D<uint16_t>;
						break;
					case ValueType::UINT32:
						mip_gen = &ComputeCoarseMip2D<uint32_t>;
						break;
					case ValueType::FLOAT32:
						mip_gen = &ComputeCoarseMip2D<float>;
						break;
					default:
						throw std::runtime_error("Mip generation for texture type is not supported!");
				}

				mip_gen(componentCount, isSRGB, last_mip_data, fineStride, 
					fineWidth, fineHeight, new_mip_data, 
					coarseStride, coarseWidth, coarseHeight);

				last_mip_data = new_mip_data;
				++iSubresource;
			}
		}
    }

    Texture::Data LoadFromBytes_RGBA8_UNORM(
        const LoadParams<Texture>& params, 
		const std::vector<uint8_t>& image,
		uint32_t width, uint32_t height) {
        
        std::vector<uint8_t> rawData;
		std::vector<TextureSubResDataDesc> subDatas;
		rawData.resize(image.size() * 2);

		size_t currentIndx = 0;

		TextureFormat format;

		if (params.bIsSRGB) {
            format = TextureFormat::SRGBA8_UNORM();
		} else {
			format = TextureFormat::RGBA8_UNORM();
		}

        Texture::Desc desc;
        desc.mWidth = width;
        desc.mHeight = height;
        desc.mMipLevels = params.bGenerateMips ? MipCount(width, height) : 1;
        desc.mFormat = format;
        desc.mType = ResourceDimension::Texture2D;
        desc.mArraySizeOrDepth = 1;

        auto result = Texture::Data::Alloc(desc);
        auto subresources = desc.GetSubresourceDescs();

		std::memcpy(&result.mData[0], &image[0], subresources[0].mLength);

		if (params.bGenerateMips)
			result.GenerateMips();

        return result;
    }

    Texture::Data LoadPNG(
        const std::filesystem::path& path,
        const LoadParams<Texture>& params) {

        Texture::Data data;
		std::vector<uint8_t> image;
		uint32_t width, height;
		uint32_t error = lodepng::decode(image, width, height, path);

		//if there's an error, display it
		if (error)
			throw std::runtime_error(lodepng_error_text(error));

		//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
		//State state contains extra information about the PNG such as text chunks, ...
		else 
			return LoadFromBytes_RGBA8_UNORM(params, image, width, height);
    }

    Texture::Data Texture::Data::Load(
        const std::filesystem::path& path,
        const LoadParams<Texture>& params) {
        
        auto ext = path.extension();

        if (ext == ".png") {
            return LoadPNG(path, params);
        } else {
            throw std::runtime_error("Unsupported file type!");
        }
    }

    Texture Texture::Load(
        const std::filesystem::path& path,
        const LoadParams<Texture>& params) {

        return Texture(Texture::Data::Load(path, params));
    }

	Texture Texture::Prefabs::SolidColor(
		uint width,
		uint height,
		const glm::vec4& color) {

		Texture::Desc desc;
		desc.mType = ResourceDimension::Texture2D;
		desc.mWidth = width;
		desc.mHeight = height;
		desc.mFormat = TextureFormat::SRGBA8_UNORM();

		auto data = Texture::Data::Alloc(desc);

		for (uint y = 0; y < height; ++y) {
			for (uint x = 0; x < width; ++x) {
				auto ptr = &data.mData[(x + y * width) * 4];
				ptr[0] = static_cast<uint8_t>(
					std::min<float>(1.0f, std::max<float>(0.0f, color.x)) * 255.0f);
				ptr[1] = static_cast<uint8_t>(
					std::min<float>(1.0f, std::max<float>(0.0f, color.y)) * 255.0f);
				ptr[2] = static_cast<uint8_t>(
					std::min<float>(1.0f, std::max<float>(0.0f, color.z)) * 255.0f);
				ptr[3] = static_cast<uint8_t>(
					std::min<float>(1.0f, std::max<float>(0.0f, color.w)) * 255.0f);
			}
		}

		return Texture(std::move(data));
	}
}