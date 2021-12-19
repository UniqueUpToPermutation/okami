#include <okami/Geometry.hpp>

namespace matball {
	#include <embed/matballmesh.hpp>
}

namespace box {
	#include <embed/boxmesh.hpp>
}

namespace bunny {
	#include <embed/bunnymesh.hpp>
}

namespace monkey {
	#include <embed/monkeymesh.hpp>
}

namespace plane {
	#include <embed/planemesh.hpp>
}

namespace sphere {
	#include <embed/spheremesh.hpp>
}

namespace torus {
	#include <embed/torusmesh.hpp>
}

namespace teapot {
	#include <embed/teapotmesh.hpp>
}

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <iostream>

using namespace entt;

namespace okami::core {

    Geometry Geometry::Prefabs::MaterialBall(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				matball::mVertexCount,
				matball::mIndexCount,
				matball::mIndices,
				matball::mPositions,
				matball::mUVs,
				matball::mNormals,
				matball::mTangents,
				matball::mBitangents));
	}

	Geometry Geometry::Prefabs::Box(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				box::mVertexCount,
				box::mIndexCount,
				box::mIndices,
				box::mPositions,
				box::mUVs,
				box::mNormals,
				box::mTangents,
				box::mBitangents));
	}

	Geometry Geometry::Prefabs::Sphere(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				sphere::mVertexCount,
				sphere::mIndexCount,
				sphere::mIndices,
				sphere::mPositions,
				sphere::mUVs,
				sphere::mNormals,
				sphere::mTangents,
				sphere::mBitangents));
	}

	Geometry Geometry::Prefabs::BlenderMonkey(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				monkey::mVertexCount,
				monkey::mIndexCount,
				monkey::mIndices,
				monkey::mPositions,
				monkey::mUVs,
				monkey::mNormals,
				monkey::mTangents,
				monkey::mBitangents));
	}

	Geometry Geometry::Prefabs::Torus(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				torus::mVertexCount,
				torus::mIndexCount,
				torus::mIndices,
				torus::mPositions,
				torus::mUVs,
				torus::mNormals,
				torus::mTangents,
				torus::mBitangents));
	}

	Geometry Geometry::Prefabs::Plane(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				plane::mVertexCount,
				plane::mIndexCount,
				plane::mIndices,
				plane::mPositions,
				plane::mUVs,
				plane::mNormals,
				plane::mTangents,
				plane::mBitangents));
	}

	Geometry Geometry::Prefabs::StanfordBunny(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				bunny::mVertexCount,
				bunny::mIndexCount,
				bunny::mIndices,
				bunny::mPositions,
				bunny::mUVs,
				bunny::mNormals,
				bunny::mTangents,
				bunny::mBitangents));
	}

	Geometry Geometry::Prefabs::UtahTeapot(const VertexLayout& layout) {
		return Geometry(layout, 
			Geometry::DataSource
				<uint32_t, float, float, float>(
				teapot::mVertexCount,
				teapot::mIndexCount,
				teapot::mIndices,
				teapot::mPositions,
				teapot::mUVs,
				teapot::mNormals,
				teapot::mTangents,
				teapot::mBitangents));
	}

    entt::meta_type Geometry::GetType() const {
        return entt::resolve<Geometry>();
    }

    void Geometry::Register() {
        entt::meta<Geometry>()
            .type("Geometry"_hs);
    }

    template <>
	struct V3Packer<aiVector3D> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const aiVector3D* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
		}
	};

	template <>
	struct V2Packer<aiVector3D> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const aiVector3D* src) {
			dest[0] = src->x;
			dest[1] = src->y;
		}
	};

	template <>
	struct I3Packer<aiFace> {
		static constexpr size_t Stride = 1;

		inline static void Pack(uint32_t* dest, const aiFace* src) {
			dest[0] = src->mIndices[0];
			dest[1] = src->mIndices[1];
			dest[2] = src->mIndices[2];
		}
	};

    void ComputeLayoutProperties(
		size_t vertex_count,
		const VertexLayout& layout,
		std::vector<size_t>& offsets,
		std::vector<size_t>& strides,
		std::vector<size_t>& channel_sizes) {
		offsets.clear();
		strides.clear();

		size_t nVerts = vertex_count;

		auto& layoutElements = layout.mElements;

		int channelCount = 0;
		for (auto& layoutItem : layoutElements) {
			channelCount = std::max<int>(channelCount, layoutItem.mBufferSlot + 1);
		}

		channel_sizes.resize(channelCount);
		std::vector<size_t> channel_auto_strides(channelCount);

		std::fill(channel_sizes.begin(), channel_sizes.end(), 0u);
		std::fill(channel_auto_strides.begin(), channel_auto_strides.end(), 0u);

		// Compute offsets
		for (auto& layoutItem : layoutElements) {
			uint size = GetSize(layoutItem.mValueType) * layoutItem.mNumComponents;

			if (layoutItem.mFrequency == InputElementFrequency::PER_VERTEX) {
				uint channel = layoutItem.mBufferSlot;

				size_t offset = 0;

				if (layoutItem.mRelativeOffset == LAYOUT_ELEMENT_AUTO_OFFSET) {
					offset = channel_auto_strides[channel];
				} else {
					offset = layoutItem.mRelativeOffset;
				}

				offsets.emplace_back(offset);
				channel_auto_strides[channel] += size;
			} else {
				offsets.emplace_back(0);
			}
		}

		// Compute strides
		for (int i = 0; i < offsets.size(); ++i) {
			auto& layoutItem = layoutElements[i];
			size_t offset = offsets[i];
			uint size = GetSize(layoutItem.mValueType) * layoutItem.mNumComponents;

			if (layoutItem.mFrequency == InputElementFrequency::PER_VERTEX) {
				uint channel = layoutItem.mBufferSlot;

				size_t stride = 0;

				if (layoutItem.mStride == LAYOUT_ELEMENT_AUTO_STRIDE) {
					stride = channel_auto_strides[channel];
				} else {
					stride = layoutItem.mStride;
				}

				strides.emplace_back(stride);

				size_t lastIndex = offset + size + (nVerts - 1) * stride;

				channel_sizes[channel] = std::max<size_t>(channel_sizes[channel], lastIndex);

			} else {
				strides.emplace_back(0);
			}
		}
	}

    PackIndexing PackIndexing::From(
        const VertexLayout& layout,
        size_t vertex_count) {

        std::vector<size_t> offsets;
        std::vector<size_t> strides;

        PackIndexing indexing;

        auto& layoutElements = layout.mElements;	
        ComputeLayoutProperties(vertex_count, layout, offsets, strides, indexing.mChannelSizes);

        auto verifyAttrib = [](const LayoutElement& element) {
            if (element.mValueType != ValueType::FLOAT32) {
                throw std::runtime_error("Attribute type must be VT_FLOAT32!");
            }
        };

        if (layout.mPosition >= 0) {
            auto& posAttrib = layoutElements[layout.mPosition];
            verifyAttrib(posAttrib);
            indexing.mPositionOffset = offsets[layout.mPosition];
            indexing.mPositionChannel = posAttrib.mBufferSlot;
            indexing.mPositionStride = strides[layout.mPosition];
        }

        for (auto& uv : layout.mUVs) {
            auto& uvAttrib = layoutElements[uv];
            verifyAttrib(uvAttrib);
            indexing.mUVOffsets.emplace_back(offsets[uv]);
            indexing.mUVChannels.emplace_back(uvAttrib.mBufferSlot);
            indexing.mUVStrides.emplace_back(strides[uv]);
        }

        if (layout.mNormal >= 0) {
            auto& normalAttrib = layoutElements[layout.mNormal];
            verifyAttrib(normalAttrib);
            indexing.mNormalOffset = offsets[layout.mNormal];
            indexing.mNormalChannel = normalAttrib.mBufferSlot;
            indexing.mNormalStride = strides[layout.mNormal];
        }

        if (layout.mTangent >= 0) {
            auto& tangentAttrib = layoutElements[layout.mTangent];
            verifyAttrib(tangentAttrib);
            indexing.mTangentOffset = offsets[layout.mTangent];
            indexing.mTangentChannel = tangentAttrib.mBufferSlot;
            indexing.mTangentStride = strides[layout.mTangent];
        }

        if (layout.mBitangent >= 0) {
            auto& bitangentAttrib = layoutElements[layout.mBitangent];
            verifyAttrib(bitangentAttrib);
            indexing.mBitangentOffset = offsets[layout.mBitangent];
            indexing.mBitangentChannel = bitangentAttrib.mBufferSlot;
            indexing.mBitangentStride = strides[layout.mBitangent];
        }

        for (auto& color : layout.mColors) {
            auto& colorAttrib = layoutElements[color];
            verifyAttrib(colorAttrib);
            indexing.mColorOffsets.emplace_back(offsets[color]);
            indexing.mColorChannels.emplace_back(colorAttrib.mBufferSlot);
            indexing.mColorStrides.emplace_back(strides[color]);
        }

        return indexing;
    }

    Geometry::RawData Geometry::RawData::Load(
        const std::filesystem::path& path,
        const VertexLayout& layout) {

        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | 
            aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
            aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | 
            aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Quality;

        const aiScene* scene = importer.ReadFile(path.c_str(), flags);
        
        if (!scene) {
            std::cout << importer.GetErrorString() << std::endl;
            throw std::runtime_error("Failed to load geometry!");
        }

        if (!scene->HasMeshes()) {
            throw std::runtime_error("Geometry has no meshes!");
        }

        size_t nVerts;
		size_t nIndices;

		if (!scene->HasMeshes()) {
			throw std::runtime_error("Assimp scene has no meshes!");
		}

		aiMesh* mesh = scene->mMeshes[0];

		nVerts = mesh->mNumVertices;
		nIndices = mesh->mNumFaces * 3;

        Geometry::DataSource<aiFace, aiVector3D, aiVector3D> data(
			nVerts, nIndices,
			mesh->mFaces,
			mesh->mVertices,
			mesh->mTextureCoords[0],
			mesh->mNormals,
			mesh->mTangents,
			mesh->mBitangents);

        Geometry::RawData result;
		result.Pack<aiFace, aiVector3D, aiVector3D>(layout, data);
        return result;
    }

    Geometry Geometry::Load(
        const std::filesystem::path& path, 
        const VertexLayout& layout) {
        auto data = Geometry::RawData::Load(path, layout);
        return Geometry(std::move(data));
    }
}