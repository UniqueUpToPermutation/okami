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
        NUM_TYPES
    };

    enum class InputElementFrequency {
        UNDEFINED = 0,
        PER_VERTEX,
        PER_INSTANCE,
        NUM_FREQUENCIES
    };

    struct LayoutElement {
        uint32_t mInputIndex = 0;
        uint32_t mBufferSlot = 0;
        uint32_t mNumComponents = 0;
        ValueType mValueType = ValueType::FLOAT32;
        bool bIsNormalized = true;
        int64_t mRelativeOffset = -1;
        int64_t mStride = -1;
        InputElementFrequency mFrequency = InputElementFrequency::PER_VERTEX;
        uint32_t mInstanceDataStepRate = 1;

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

    struct VertexLayout {
	public:
		std::vector<LayoutElement> mElements;

		int mPosition	= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;

		std::vector<int> mUVs;
		std::vector<int> mColors;
		
		static VertexLayout PositionUVNormalTangent();
		static VertexLayout PositionUVNormal();
		static VertexLayout PositionUVNormalTangentBitangent();

		template <class Archive>
		void serialize(Archive& archive) {
			archive(mElements);

			archive(mPosition);
			archive(mNormal);
			archive(mTangent);
			archive(mBitangent);

			archive(mUVs);
			archive(mColors);
		}
	};
}