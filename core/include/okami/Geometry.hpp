#pragma once

#include <okami/VertexLayout.hpp>
#include <okami/Resource.hpp>
#include <okami/ResourceInterface.hpp>
#include <okami/System.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
#include <cstring>

namespace okami::core {
	class Geometry;

	template <>
	struct LoadParams<Geometry>;

    struct BufferDesc {
        uint32_t mSizeInBytes;
    };

    struct BufferData {
        BufferDesc mDesc;
        std::vector<uint8_t> mBytes;
    };

	struct BoundingBox {
		glm::vec3 mLower;
		glm::vec3 mUpper;
	};

	struct BoundingBox2D {
		glm::vec2 mLower;
		glm::vec2 mUpper;
	};

    class Geometry final : public Resource {
	public:
		enum class Type {
			UNDEFINED,
			STATIC_MESH,
			NUM_TYPES
		};

		struct Attribs {
			uint32_t mNumVertices = 0;
		};

		struct IndexedAttribs {
			ValueType mIndexType = ValueType::UNDEFINED;
			uint32_t mNumIndices = 0;
		};
		
		struct Desc {
			VertexLayout mLayout;
			Attribs mAttribs;
			IndexedAttribs mIndexedAttribs;
			bool bIsIndexed;

			Type mType = Type::UNDEFINED;
		};

		template <typename IndexType = uint32_t,
			typename Vec2Type = glm::vec2,
			typename Vec3Type = glm::vec3,
			typename Vec4Type = glm::vec4>
		struct Data {
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

			static Data<IndexType, 
				Vec2Type, 
				Vec3Type, 
				Vec4Type> Load(
					const std::filesystem::path& path,
					const VertexLayout& layout);
		};

		template <typename IndexType = uint32_t,
			typename Vec2Type = glm::vec2,
			typename Vec3Type = glm::vec3,
			typename Vec4Type = glm::vec4>
		struct DataSource {
			size_t mVertexCount;
			size_t mIndexCount;

			const IndexType* mIndices;
			const Vec3Type* mPositions;
			std::vector<const Vec2Type*> mUVs;
			const Vec3Type* mNormals;
			const Vec3Type* mTangents;
			const Vec3Type* mBitangents;
			std::vector<const Vec4Type*> mColors;

			DataSource(
				const Data<IndexType, Vec2Type, Vec3Type, Vec4Type>& data) :
				mVertexCount(data.mPositions.size()),
				mIndexCount(data.mIndices.size()),
				mIndices(data.mIndices.size() > 0 ? &data.mIndices[0] : nullptr),
				mPositions(data.mPositions.size() > 0 ? &data.mPositions[0] : nullptr),
				mNormals(data.mNormals.size() > 0 ? &data.mNormals[0] : nullptr),
				mTangents(data.mTangents.size() > 0 ? &data.mTangents[0] : nullptr),
				mBitangents(data.mBitangents.size() > 0 ? &data.mBitangents[0] : nullptr) {

				for (auto& uv : data.mUVs) 
					mUVs.emplace_back(&uv[0]);
				
				for (auto& colors : data.mColors) 
					mColors.emplace_back(&colors[0]);
			}

			DataSource(
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

			DataSource(
				size_t vertexCount,
				size_t indexCount,
				const IndexType indices[],
				const Vec3Type positions[],
				const Vec2Type uvs[] = nullptr,
				const Vec3Type normals[] = nullptr,
				const Vec3Type tangents[] = nullptr,
				const Vec3Type bitangents[] = nullptr,
				const Vec4Type colors[] = nullptr) :
				DataSource(vertexCount,
					indexCount,
					indices,
					positions,
					std::vector<const Vec2Type*>{uvs},
					normals,
					tangents,
					bitangents,
					std::vector<const Vec4Type*>{colors}) {
			}
			
			DataSource(size_t vertexCount,
				const Vec3Type positions[],
				const std::vector<const Vec2Type*>& uvs = {},
				const Vec3Type normals[] = nullptr,
				const Vec3Type tangents[] = nullptr,
				const Vec3Type bitangents[] = nullptr,
				const std::vector<const Vec4Type*>& colors = {}) :
				DataSource(vertexCount,
					0,
					nullptr,
					positions,
					uvs,
					normals,
					tangents,
					bitangents,
					colors) {
			}

			DataSource(size_t vertexCount,
				const Vec3Type positions[],
				const Vec2Type uvs[] = nullptr,
				const Vec3Type normals[] = nullptr,
				const Vec3Type tangents[] = nullptr,
				const Vec3Type bitangents[] = nullptr,
				const Vec4Type colors[] = nullptr) : 
				DataSource(vertexCount,
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

		struct RawData {
			Geometry::Desc mDesc;
			std::vector<BufferData> mVertexBuffers;
			BufferData mIndexBuffer;
			BoundingBox mBoundingBox;

			template <typename I3T, typename V2T, typename V3T, typename V4T>
			void Pack(const VertexLayout& layout,
				const DataSource<I3T, V2T, V3T, V4T>& data);

			template <typename I3T, typename V2T, typename V3T, typename V4T>
				Data<I3T, V2T, V3T, V4T> Unpack() const;

			static RawData Load(const std::filesystem::path& path,
				const VertexLayout& layout);
		};

	private:
		RawData mData;
		uint64_t mFlags;

	public:
		inline RawData& DataCPU() {
			return mData;
		}

		inline void Clear() {
			mData = RawData();
		}

		inline Geometry(RawData&& data) 
			: mData(std::move(data)) {
		}

		template <typename IndexType,
			typename Vec2Type,
			typename Vec3Type,
			typename Vec4Type> 
		inline Geometry(
			const VertexLayout& layout,
			const DataSource<IndexType, 
				Vec2Type, 
				Vec3Type, 
				Vec4Type>& data) {
			mData.Pack(layout, data);
		}

		template <typename IndexType,
			typename Vec2Type,
			typename Vec3Type,
			typename Vec4Type> 
		inline Geometry(
			const VertexLayout& layout,
			const Data<IndexType, 
				Vec2Type, 
				Vec3Type, 
				Vec4Type>& data) {
			mData.Pack(layout, DataSource<IndexType,
				Vec2Type,
				Vec3Type,
				Vec4Type>(data));
		}

		Geometry() = default;
		Geometry(const Geometry&) = delete;
		Geometry& operator=(const Geometry&) = delete;
		Geometry(Geometry&&) = default;
		Geometry& operator=(Geometry&&) = default;

		inline const Desc& Desc() const {
			return mData.mDesc;
		}

		entt::meta_type GetType() const override;

		static void Register();

		struct Prefabs {
			static Geometry MaterialBall(const VertexLayout& layout);
			static Geometry Box(const VertexLayout& layout);
			static Geometry Sphere(const VertexLayout& layout);
			static Geometry BlenderMonkey(const VertexLayout& layout);
			static Geometry Torus(const VertexLayout& layout);
			static Geometry Plane(const VertexLayout& layout);
			static Geometry StanfordBunny(const VertexLayout& layout);
			static Geometry UtahTeapot(const VertexLayout& layout);
		};

		static Geometry Load(const std::filesystem::path& path, 
			const VertexLayout& layout);
    };

	template <>
	struct LoadParams<Geometry> {
		VertexLayout mLayout;
		Geometry::Type mType = Geometry::Type::UNDEFINED;
	};

	template <typename T>
	struct V4Packer;

	template <typename T>
	struct V3Packer;

	template <typename T>
	struct V2Packer;

	template <typename T>
	struct I3Packer;

	template <>
	struct V4Packer<glm::vec4> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const glm::vec4* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
			dest[3] = src->w;
		}

		inline static void Unpack(glm::vec4* dest, const float* src) {
			dest->x = src[0];
			dest->y = src[1];
			dest->z = src[2];
			dest->w = src[3];
		}
	};

	template <>
	struct V4Packer<float> {
		static constexpr size_t Stride = 4;

		inline static void Pack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
			dest[3] = src[3];
		}

		inline static void Unpack(float* dest, const float* src) {
			Pack(dest, src);
		}
	};

	template <>
	struct V3Packer<glm::vec3> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const glm::vec3* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
		}

		inline static void Unpack(glm::vec3* dest, const float* src) {
			dest->x = src[0];
			dest->y = src[1];
			dest->z = src[2];
		}
	};

	template <>
	struct V3Packer<float> {
		static constexpr size_t Stride = 3;

		inline static void Pack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
		}

		inline static void Unpack(float* dest, const float* src) {
			Pack(dest, src);
		}
	};

	template <>
	struct V2Packer<glm::vec2> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const glm::vec2* src) {
			dest[0] = src->x;
			dest[1] = src->y;
		}

		inline static void Unpack(glm::vec2* dest, const float* src) {
			dest->x = src[0];
			dest->y = src[1];
		}
	};

	template <>
	struct V2Packer<float> {
		static constexpr size_t Stride = 2;

		inline static void Pack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
		}

		inline static void Unpack(float* dest, const float* src) {
			Pack(dest, src);
		}
	};

	template <>
	struct I3Packer<uint32_t> {
		static constexpr size_t Stride = 3;

		inline static void Pack(uint32_t* dest, const uint32_t* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
		}

		inline static void Unpack(uint32_t* dest, const uint32_t* src) {
			Pack(dest, src);
		}
	};

	template <
		typename destT, 
		typename srcT, 
		void(*MoveFunc)(destT*, const srcT*)>
	void ArraySliceCopy(destT* dest, const srcT* source, 
		size_t destStride, size_t srcStride, size_t count) {
		size_t destIndex = 0;
		size_t srcIndex = 0;
		
		for (size_t i = 0; i < count; ++i, destIndex += destStride, srcIndex += srcStride) {
			(*MoveFunc)(&dest[destIndex], &source[srcIndex]);
		}
	}

	template <
		typename destT, 
		typename srcT, 
		void(*MoveFunc)(destT*, const srcT*)>
	void ArraySliceCopyToBytes(uint8_t* dest, const srcT* source, 
		size_t destStrideBytes, size_t srcStride, size_t count) {
		size_t destIndex = 0;
		size_t srcIndex = 0;
		
		for (size_t i = 0; i < count; ++i, destIndex += destStrideBytes, srcIndex += srcStride) {
			(*MoveFunc)(reinterpret_cast<destT*>(&dest[destIndex]), &source[srcIndex]);
		}
	}

	template <
		typename destT, 
		typename srcT, 
		void(*MoveFunc)(destT*, const srcT*)>
	void ArraySliceCopyFromBytes(destT* dest, const uint8_t* source, 
		size_t destStride, size_t srcStrideBytes, size_t count) {
		size_t destIndex = 0;
		size_t srcIndex = 0;
		
		for (size_t i = 0; i < count; ++i, destIndex += destStride, srcIndex += srcStrideBytes) {
			(*MoveFunc)(&dest[destIndex], reinterpret_cast<const srcT*>(&source[srcIndex]));
		}
	}

	template <typename T, size_t dim>
	void ArraySliceCopy(T* dest, const T* source,
		size_t destStride, size_t srcStride, size_t count) {
		size_t destIndex = 0;
		size_t srcIndex = 0;
		for (size_t i = 0; i < count; ++i, destIndex += destStride, srcIndex += srcStride) {
			for (size_t component = 0; component < dim; ++component) {
				dest[destIndex + component] = source[srcIndex + component];
			}
		}
	}

	template <typename T, size_t dim>
	void ArraySliceBoundingBox(T* arr, 
		size_t stride, size_t count, 
		std::array<T, dim>& lower, std::array<T, dim>& upper) {
		std::fill(upper.begin(), upper.end(), -std::numeric_limits<T>::infinity());
		std::fill(lower.begin(), lower.end(), std::numeric_limits<T>::infinity());

		size_t srcIndex = 0;
		for (size_t i = 0; i < count; ++i, srcIndex += stride) {
			for (size_t component = 0; component < dim; ++component) {
				upper[component] = std::max<T>(upper[component], arr[srcIndex]);
				lower[component] = std::min<T>(lower[component], arr[srcIndex]);
			}
		}
	}

	template <typename T, size_t dim>
	void ArraySliceBoundingBoxBytes(uint8_t* arr, 
		size_t stride, size_t count, 
		std::array<T, dim>& lower, 
		std::array<T, dim>& upper) {
		std::fill(upper.begin(), upper.end(), -std::numeric_limits<T>::infinity());
		std::fill(lower.begin(), lower.end(), std::numeric_limits<T>::infinity());

		size_t srcIndex = 0;
		for (size_t i = 0; i < count; ++i, srcIndex += stride) {
			auto arr_cast = reinterpret_cast<T*>(&arr[i]);
			for (size_t component = 0; component < dim; ++component) {
				upper[component] = std::max<T>(upper[component], arr_cast[component]);
				lower[component] = std::min<T>(lower[component], arr_cast[component]);
			}
		}
	}

	inline BoundingBox ArraySliceBoundingBox(float* arr,
		size_t stride, size_t count) {
		std::array<float, 3> upper;
		std::array<float, 3> lower;

		ArraySliceBoundingBox<float, 3>(arr, stride, count, lower, upper);

		BoundingBox result;
		result.mLower = glm::vec3(lower[0], lower[1], lower[2]);
		result.mUpper = glm::vec3(upper[0], upper[1], upper[2]);
		return result;
	}

	inline BoundingBox ArraySliceBoundingBoxBytes(uint8_t* arr,
		size_t stride, size_t count) {
		std::array<float, 3> upper;
		std::array<float, 3> lower;

		ArraySliceBoundingBoxBytes<float, 3>(arr, stride, count, lower, upper);

		BoundingBox result;
		result.mLower = glm::vec3(lower[0], lower[1], lower[2]);
		result.mUpper = glm::vec3(upper[0], upper[1], upper[2]);
		return result;
	}

	template <typename destT, size_t componentCount>
	void ArraySliceFill(destT* dest,
		const destT& value,
		size_t stride,
		size_t vertex_count) {

		for (size_t i = 0; i < vertex_count; i += stride) {
			for (size_t component = 0; component < componentCount; ++component) {
				dest[i + component] = value;
			}
		}
	}

	template <typename destT, size_t componentCount>
	void ArraySliceFillBytes(uint8_t* dest,
		const destT& value,
		size_t stride,
		size_t vertex_count) {

		for (size_t i = 0; i < vertex_count; i += stride) {
			auto dest_cast = reinterpret_cast<destT*>(&dest[i]);
			for (size_t component = 0; component < componentCount; ++component) {
				dest_cast[component] = value;
			}
		}
	}

	// Given a vertex layout, compute the offsets, strides, and channel sizes
	// of each of the geometry elements in the layout
	void ComputeLayoutProperties(
		size_t vertex_count,
		const VertexLayout& layout,
		std::vector<size_t>& offsets,
		std::vector<size_t>& strides,
		std::vector<size_t>& channel_sizes);

	// A structure that holds all indexing strides and offsets for
	// different geometry elements (i.e., position, normal, etc.).
	struct PackIndexing {
		int mPositionOffset = -1;
		int mPositionChannel = -1;
		int mPositionStride = -1;

		std::vector<int> mUVOffsets = {};
		std::vector<int> mUVChannels = {};
		std::vector<int> mUVStrides = {};

		int mNormalOffset = -1;
		int mNormalChannel = -1;
		int mNormalStride = -1;

		int mTangentOffset = -1;
		int mTangentChannel = -1;
		int mTangentStride = -1;

		int mBitangentOffset = -1;
		int mBitangentChannel = -1;
		int mBitangentStride = -1;

		std::vector<int> mColorOffsets = {};
		std::vector<int> mColorChannels = {};
		std::vector<int> mColorStrides = {};

		std::vector<size_t> mChannelSizes;

		static PackIndexing From(const VertexLayout& layout,
			size_t vertex_count);
	};

	template <typename I3T, typename V2T, typename V3T, typename V4T>
	void Geometry::RawData::Pack(const VertexLayout& layout,
		const Geometry::DataSource<I3T, V2T, V3T, V4T>& data) {

		size_t vertex_count = data.mVertexCount;
		size_t index_count = data.mIndexCount;

		auto indexing = PackIndexing::From(layout, vertex_count);

		auto& channel_sizes = indexing.mChannelSizes;
		uint channelCount = channel_sizes.size();
	
		std::vector<std::vector<uint8_t>> vert_buffers(channelCount);
		for (uint i = 0; i < channelCount; ++i)
			vert_buffers[i] = std::vector<uint8_t>(channel_sizes[i]);

		std::vector<uint8_t> indx_buffer_raw(index_count * sizeof(uint32_t));
		uint32_t* indx_buffer = (uint32_t*)(&indx_buffer_raw[0]);

		BoundingBox aabb;

		bool bHasNormals = data.mNormals != nullptr;
		bool bHasPositions = data.mPositions != nullptr;
		bool bHasBitangents = data.mBitangents != nullptr;
		bool bHasTangents = data.mTangents != nullptr;

		if (indexing.mPositionOffset >= 0) {
			auto& channel = vert_buffers[indexing.mPositionChannel];
			auto arr = &channel[indexing.mPositionOffset];
			if (bHasPositions) {
				ArraySliceCopyToBytes<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mPositions[0], indexing.mPositionStride, V3Packer<V3T>::Stride, vertex_count);
				aabb = ArraySliceBoundingBoxBytes(arr, (size_t)indexing.mPositionStride, vertex_count);
			} else {
				PrintWarning("Pipeline expects positions, but model has none!");
				ArraySliceFillBytes<float, 3>(arr, 0.0f, indexing.mPositionStride, vertex_count);
				aabb.mLower = glm::vec3(0.0f, 0.0f, 0.0f);
				aabb.mUpper = glm::vec3(0.0f, 0.0f, 0.0f);
			}
		}

		for (uint iuv = 0; iuv < indexing.mUVChannels.size(); ++iuv) {
			auto& vertexChannel = vert_buffers[indexing.mUVChannels[iuv]];
			auto arr = &vertexChannel[indexing.mUVOffsets[iuv]];
			if (iuv < data.mUVs.size()) {
				ArraySliceCopyToBytes<float, V2T, &V2Packer<V2T>::Pack>(
					arr, &data.mUVs[iuv][0], indexing.mUVStrides[iuv], V2Packer<V2T>::Stride, vertex_count);
			} else {
				PrintWarning("Pipeline expects UVs, but model has none!");
				ArraySliceFillBytes<float, 2>(arr, 0.0f, indexing.mUVStrides[iuv], vertex_count);
			}
		}

		if (indexing.mNormalOffset >= 0) {
			auto& channel = vert_buffers[indexing.mNormalChannel];
			auto arr = &channel[indexing.mNormalOffset];
			if (bHasNormals) {
				ArraySliceCopyToBytes<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mNormals[0], indexing.mNormalStride, V3Packer<V3T>::Stride, vertex_count);
			} else {
				PrintWarning("Warning: Pipeline expects normals, but model has none!");
				ArraySliceFillBytes<float, 3>(arr, 0.0f, indexing.mNormalStride, vertex_count);
			}
		}

		if (indexing.mTangentOffset >= 0) {
			auto& channel = vert_buffers[indexing.mTangentChannel];
			auto arr = &channel[indexing.mTangentOffset];
			if (bHasTangents) {
				ArraySliceCopyToBytes<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mTangents[0], indexing.mTangentStride, 
						V3Packer<V3T>::Stride, vertex_count);
			} else {
				PrintWarning("Warning: Pipeline expects tangents, but model has none!");
				ArraySliceFillBytes<float, 3>(arr, 0.0f, indexing.mTangentStride, vertex_count);
			}
		}

		if (indexing.mBitangentOffset >= 0) {
			auto& channel = vert_buffers[indexing.mBitangentChannel];
			auto arr = &channel[indexing.mBitangentOffset];
			if (bHasBitangents) {
				ArraySliceCopyToBytes<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mBitangents[0], indexing.mBitangentStride, 
						V3Packer<V3T>::Stride, vertex_count);
			} else {
				PrintWarning("Warning: Pipeline expects bitangents, but model has none!");
				ArraySliceFillBytes<float, 3>(arr, 0.0f, indexing.mBitangentStride, vertex_count);
			}
		}

		for (uint icolor = 0; icolor < indexing.mColorChannels.size(); ++icolor) {
			auto& vertexChannel = vert_buffers[indexing.mColorChannels[icolor]];
			auto arr = &vertexChannel[indexing.mColorOffsets[icolor]];
			if (icolor < data.mColors.size()) {
				ArraySliceCopyToBytes<float, V4T, &V4Packer<V4T>::Pack>(
					arr, &data.mColors[icolor][0], indexing.mColorStrides[icolor], 
						V4Packer<V4T>::Stride, vertex_count);
			} else {
				PrintWarning("Warning: Pipeline expects colors, but model has none!");
				ArraySliceFillBytes<float, 4>(arr, 1.0f, indexing.mColorStrides[icolor], vertex_count);
			}
		}

		if (data.mIndices != nullptr && data.mIndexCount > 0) {
			ArraySliceCopy<uint32_t, I3T, &I3Packer<I3T>::Pack>(
				&indx_buffer[0], &data.mIndices[0], 3, 
				I3Packer<I3T>::Stride, index_count / 3);
		}

		std::vector<BufferDesc> bufferDescs;

		for (uint i = 0; i < channelCount; ++i) {
			BufferDesc vertexBufferDesc;
			vertexBufferDesc.mSizeInBytes = vert_buffers[i].size();
			bufferDescs.emplace_back(vertexBufferDesc);
		}

		BufferDesc indexBufferDesc;
		indexBufferDesc.mSizeInBytes = indx_buffer_raw.size();

		IndexedAttribs indexedAttribs;
		indexedAttribs.mIndexType = ValueType::UINT32;
		indexedAttribs.mNumIndices = indx_buffer_raw.size() / sizeof(uint32_t);
		
		Attribs attribs;
		attribs.mNumVertices = vertex_count;

		// Write to output raw geometry
		mDesc.bIsIndexed = (data.mIndices != nullptr && data.mIndexCount > 0);
		mDesc.mAttribs = attribs;
		mDesc.mIndexedAttribs = indexedAttribs;
		mDesc.mLayout = layout;

		mVertexBuffers.clear();

		for (uint i = 0; i < channelCount; ++i) {
			BufferData bufferData;
			bufferData.mBytes = std::move(vert_buffers[i]);
			bufferData.mDesc = bufferDescs[i];
			mVertexBuffers.emplace_back(std::move(bufferData));
		}

		BufferData indxBufferData;
		indxBufferData.mBytes = std::move(indx_buffer_raw);
		indxBufferData.mDesc = indexBufferDesc;
		mIndexBuffer = std::move(indxBufferData);

		mBoundingBox = aabb;
	}

	template <typename I3T, typename V2T, typename V3T, typename V4T>
		Geometry::Data<I3T, V2T, V3T, V4T> Geometry::RawData::Unpack() const {
		Geometry::Data<I3T, V2T, V3T, V4T> result;

		size_t vertex_count = 0;
		if (mDesc.bIsIndexed) {
			if (mDesc.mIndexedAttribs.mIndexType != ValueType::UINT32) {
				throw std::runtime_error("Index type must be VT_UINT32!");
			}

			result.mIndices.resize(mDesc.mIndexedAttribs.mNumIndices);
			std::memcpy(&result.mIndices[0], &mIndexBuffer.mBytes[0],
				sizeof(uint32_t) * mDesc.mIndexedAttribs.mNumIndices);
		}
		
		vertex_count = mDesc.mAttribs.mNumVertices;
		auto& layout = mDesc.mLayout;
		auto indexing = PackIndexing::From(layout, vertex_count);

		if (layout.mPosition >= 0) {
			result.mPositions.resize(V3Packer<V3T>::Stride * vertex_count);
			auto arr = 
				&mVertexBuffers[indexing.mPositionChannel].mBytes[indexing.mPositionOffset];
			ArraySliceCopyFromBytes<V3T, float, &V3Packer<V3T>::Unpack>(
				&result.mPositions[0], arr, V3Packer<V3T>::Stride, 
				indexing.mPositionStride, vertex_count);
		}

		result.mUVs.resize(indexing.mUVChannels.size());
		for (uint iuv = 0; iuv < indexing.mUVChannels.size(); ++iuv) {
			result.mUVs[iuv].resize(V2Packer<V2T>::Stride * vertex_count);
			auto arr = 
				&mVertexBuffers[indexing.mUVChannels[iuv]].mBytes[indexing.mUVOffsets[iuv]];
			ArraySliceCopyFromBytes<V2T, float, &V2Packer<V2T>::Unpack>(
				&result.mUVs[iuv][0], arr, V2Packer<V2T>::Stride,
				indexing.mUVStrides[iuv], vertex_count);
		}

		if (layout.mNormal >= 0) {
			result.mNormals.resize(V3Packer<V3T>::Stride * vertex_count);
			auto arr = 
				&mVertexBuffers[indexing.mNormalChannel].mBytes[indexing.mNormalOffset];
			ArraySliceCopyFromBytes<V3T, float, &V3Packer<V3T>::Unpack>(
				&result.mNormals[0], arr, V3Packer<V3T>::Stride,
				 indexing.mNormalStride, vertex_count);
		}

		if (layout.mTangent >= 0) {
			result.mTangents.resize(V3Packer<V3T>::Stride * vertex_count);
			auto arr = 
				&mVertexBuffers[indexing.mTangentChannel].mBytes[indexing.mTangentOffset];
			ArraySliceCopyFromBytes<V3T, float, &V3Packer<V3T>::Unpack>(
				&result.mTangents[0], arr, V3Packer<V3T>::Stride, 
				indexing.mTangentStride, vertex_count);
		}

		if (layout.mBitangent >= 0) {
			result.mBitangents.resize(V3Packer<V3T>::Stride * vertex_count);
			auto arr = 
				&mVertexBuffers[indexing.mBitangentChannel].mBytes[indexing.mBitangentOffset];
			ArraySliceCopyFromBytes<V3T, float, &V3Packer<V3T>::Unpack>(
				&result.mBitangents[0], arr, V3Packer<V3T>::Stride, 
				indexing.mBitangentStride, vertex_count);
		}

		result.mColors.resize(indexing.mColorChannels.size());
		for (uint icolor = 0; icolor < indexing.mColorChannels.size(); ++icolor) {
			result.mColors[icolor].resize(V4Packer<V4T>::Stride * vertex_count);
			auto arr = 
				&mVertexBuffers[indexing.mColorChannels[icolor]].mBytes[indexing.mColorOffsets[icolor]];
			ArraySliceCopyFromBytes<V4T, float, &V4Packer<V4T>::Unpack>(
				&result.mColors[icolor][0], arr, V4Packer<V4T>::Stride,
				indexing.mColorStrides[icolor], vertex_count);
		}

		return result;
	}

	template <typename IndexType,
			typename Vec2Type,
			typename Vec3Type,
			typename Vec4Type>
	Geometry::Data<IndexType, 
				Vec2Type, 
				Vec3Type, 
				Vec4Type> 
		Geometry::Data<IndexType, 
		Vec2Type, 
		Vec3Type, 
		Vec4Type>::Load(
			const std::filesystem::path& path,
			const VertexLayout& layout) {
		auto rawData = Geometry::RawData::Load(path, layout);
		return rawData.Unpack<IndexType, Vec2Type, Vec3Type, Vec4Type>();
	}

	class IVertexLayoutProvider {
    public:
        virtual const VertexLayout& GetVertexLayout(
            const entt::meta_type& type) const = 0;
        
        template <typename T>
        inline const VertexLayout& GetVertexLayout() const {
            return GetVertexLayout(entt::resolve<T>());
        }
    };

    class VertexLayoutRegistry {
    private:
        std::unordered_map<entt::meta_type, VertexLayout, TypeHash> 
            mVertexLayouts;
    public:
        template <typename T>
        void Register(VertexLayout layout) {
            mVertexLayouts.emplace(entt::resolve<T>(), std::move(layout));
        }

        const VertexLayout& Get(const entt::meta_type& type) const {
            auto it = mVertexLayouts.find(type);

            if (it != mVertexLayouts.end()) {
                return it->second;
            } else {
                throw std::runtime_error("This type does not have"
                    " an associated vertex layout!");
            }
        }

		template <typename T>
		const VertexLayout& Get() const {
			return Get(entt::resolve<T>());
		}
    };
}