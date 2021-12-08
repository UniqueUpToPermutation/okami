#pragma once

#include <okami/VertexLayout.hpp>
#include <okami/ResourcePool.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace okami::core {
    struct BufferDesc {
        uint32_t mSizeInBytes;
    };

    struct BufferData {
        BufferDesc mDesc;
        std::vector<uint8_t> mBytes;
    };

    enum class GeometryType {
        UNDEFINED,
        STATIC_MESH,
        NUM_TYPES
    };
    
    struct GeometryDesc {
        VertexLayout mLayout;
        GeometryType mType = GeometryType::UNDEFINED;
    };

    template <typename IndexType = uint32_t,
		typename Vec2Type = glm::vec2,
		typename Vec3Type = glm::vec3,
		typename Vec4Type = glm::vec4>
	struct StructuredGeometryData {
		std::vector<IndexType> mIndices;
		std::vector<Vec3Type> mPositions;
		std::vector<std::vector<Vec2Type>> mUVs;
		std::vector<Vec3Type> mNormals;
		std::vector<Vec3Type> mTangents;
		std::vector<Vec3Type> mBitangents;
		std::vector<std::vector<Vec4Type>> mColors;

		template <typename Archive>
		void serialize(Archive& arr) {
			arr(mIndices);
			arr(mPositions);
			arr(mUVs);
			arr(mNormals);
			arr(mTangents);
			arr(mBitangents);
			arr(mColors);
		}
	};

    template <typename IndexType = uint32_t,
		typename Vec2Type = glm::vec2,
		typename Vec3Type = glm::vec3,
		typename Vec4Type = glm::vec4>
	struct GeometryDataSource {
		size_t mVertexCount;
		size_t mIndexCount;

		const IndexType* mIndices;
		const Vec3Type* mPositions;
		std::vector<const Vec2Type*> mUVs;
		const Vec3Type* mNormals;
		const Vec3Type* mTangents;
		const Vec3Type* mBitangents;
		std::vector<const Vec4Type*> mColors;

		GeometryDataSource(
			const GeometryData<IndexType, Vec2Type, Vec3Type, Vec4Type>& data) :
			mVertexCount(data.mPositions.size()),
			mIndexCount(data.mIndices.size()),
			mPositions(data.mPositions.size() > 0 ? &data.mPositions[0] : nullptr),
			mNormals(data.mNormals.size() > 0 ? &data.mNormals[0] : nullptr),
			mTangents(data.mTangents.size() > 0 ? &data.mTangents[0] : nullptr),
			mBitangents(data.mBitangents.size() > 0 ? &data.mBitangents[0] : nullptr) {

			for (auto& uv : data.mUVs) 
				mUVs.emplace_back(&uv[0]);
			
			for (auto& colors : data.mColors) 
				mColors.emplace_back(&colors[0]);
		}

		GeometryDataSource(
			size_t vertexCount,
			size_t indexCount,
			const IndexType indices[],
			const Vec3Type positions[],
			const std::vector<const Vec2Type*>& uvs = {},
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const std::vector<const Vec4Type*>& colors = {}) : 
			mVertexCount(vertexCount),
			mIndexCount(indexCount),
			mIndices(indices),
			mPositions(positions),
			mUVs(uvs),
			mNormals(normals),
			mTangents(tangents),
			mBitangents(bitangents),
			mColors(colors) {
		}

		GeometryDataSource(
			size_t vertexCount,
			size_t indexCount,
			const IndexType indices[],
			const Vec3Type positions[],
			const Vec2Type uvs[] = nullptr,
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const Vec4Type colors[] = nullptr) :
			GeometryDataSource(vertexCount,
				indexCount,
				indices,
				positions,
				std::vector<const Vec2Type*>{uvs},
				normals,
				tangents,
				bitangents,
				std::vector<const Vec4Type*>{colors}) {
		}
		
		GeometryDataSource(size_t vertexCount,
			const Vec3Type positions[],
			const std::vector<const Vec2Type*>& uvs = {},
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const std::vector<const Vec4Type*>& colors = {}) :
			GeometryDataSource(vertexCount,
				0,
				nullptr,
				positions,
				uvs,
				normals,
				tangents,
				bitangents,
				colors) {
		}

		GeometryDataSource(size_t vertexCount,
			const Vec3Type positions[],
			const Vec2Type uvs[] = nullptr,
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const Vec4Type colors[] = nullptr) : 
			GeometryDataSource(vertexCount,
				0,
				nullptr,
				positions,
				{uvs},
				normals,
				tangents,
				bitangents,
				{colors}) {
		}

		bool HasIndices() const {
			return mIndices != nullptr;
		}

		bool HasPositions() const {
			return mPositions != nullptr;
		}

		bool HasNormals() const {
			return mNormals != nullptr;
		}

		bool HasTangents() const {
			return mTangents != nullptr;
		}

		bool HasBitangents() const {
			return mBitangents != nullptr;
		}
	};

    struct GeometryData {
        GeometryDesc mDesc;
        std::vector<BufferData> mVertexBuffers;
        BufferData mIndexBuffer;
    };

    class Geometry : public ResourceHandle<GeometryDesc> {
    };
}