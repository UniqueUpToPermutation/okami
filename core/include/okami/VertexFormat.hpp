#pragma once

#include <vector>
#include <cstdint>

namespace okami::core {

    enum class ValueType : uint8_t {
        UNDEFINED = 0,
        INT8,
        INT16,
        INT32,
        UINT8,
        UINT16,
        UINT32,
        FLOAT16,
        FLOAT32,
        NULL_T,
        NUM_TYPES
    };

    int GetSize(ValueType v);

    enum class InputElementFrequency {
        UNDEFINED = 0,
        PER_VERTEX,
        PER_INSTANCE,
        NUM_FREQUENCIES
    };

    static const uint32_t LAYOUT_ELEMENT_AUTO_OFFSET = 0xFFFFFFFF;
    static const uint32_t LAYOUT_ELEMENT_AUTO_STRIDE = 0xFFFFFFFF;

    struct LayoutElement {
        uint32_t mInputIndex = 0;
        uint32_t mBufferSlot = 0;
        uint32_t mNumComponents = 0;
        ValueType mValueType = ValueType::FLOAT32;
        bool bIsNormalized = true;
        uint32_t mRelativeOffset = LAYOUT_ELEMENT_AUTO_OFFSET;
        uint32_t mStride = LAYOUT_ELEMENT_AUTO_STRIDE;
        InputElementFrequency mFrequency = InputElementFrequency::PER_VERTEX;
        uint32_t mInstanceDataStepRate = 1;

        LayoutElement() = default;
        LayoutElement(
            uint32_t inputIndex,
            uint32_t blufferSlot,
            uint32_t numComponents,
            ValueType valueType,
            bool isNormalized,
            uint32_t relativeOffset,
            uint32_t stride,
            InputElementFrequency frequency,
            uint32_t instanceDataStepRate) :
            mInputIndex(inputIndex),
            mBufferSlot(blufferSlot),
            mNumComponents(numComponents),
            mValueType(valueType),
            bIsNormalized(isNormalized),
            mRelativeOffset(relativeOffset),
            mStride(stride),
            mFrequency(frequency),
            mInstanceDataStepRate(instanceDataStepRate) {
        }

        LayoutElement(
            uint32_t inputIndex,
            uint32_t blufferSlot,
            uint32_t numComponents,
            ValueType valueType) :
            mInputIndex(inputIndex),
            mBufferSlot(blufferSlot),
            mNumComponents(numComponents),
            mValueType(valueType) {
        }

        LayoutElement(
            uint32_t inputIndex,
            uint32_t blufferSlot,
            uint32_t numComponents,
            ValueType valueType,
            bool isNormalized,
            InputElementFrequency frequency) :
            mInputIndex(inputIndex),
            mBufferSlot(blufferSlot),
            mNumComponents(numComponents),
            mValueType(valueType),
            bIsNormalized(isNormalized),
            mFrequency(frequency) {
        }

        template <class Archive>
		void serialize(Archive& archive) {
            archive(mInputIndex);
            archive(mBufferSlot);
            archive(mNumComponents);
            archive(mValueType);
            archive(bIsNormalized);
            archive(mRelativeOffset);
            archive(mStride);
            archive(mFrequency);
            archive(mInstanceDataStepRate);
        }
    };

    enum class Topology {
        TRIANGLE_LIST,
        TRIANGLE_STRIP,
        POINT_LIST,
        LINE_LIST,
        LINE_STRIP
    };

    struct VertexFormat {
	public:
		std::vector<LayoutElement> mElements;

		int mPosition	= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;

        Topology mTopology = Topology::TRIANGLE_LIST;

		std::vector<int> mUVs;
		std::vector<int> mColors;
		
		static VertexFormat PositionUVNormalTangent();
		static VertexFormat PositionUVNormal();
		static VertexFormat PositionUVNormalTangentBitangent();
        static VertexFormat PositionUV();
        static VertexFormat Position();

		template <class Archive>
		void serialize(Archive& archive) {
			archive(mElements);

			archive(mPosition);
			archive(mNormal);
			archive(mTangent);
			archive(mBitangent);

			archive(mUVs);
			archive(mColors);
            archive(mTopology);
		}
	};
}