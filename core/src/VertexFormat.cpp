#include <okami/VertexFormat.hpp>

namespace okami::core {
    int GetSize(ValueType v) {
		switch (v) {
			case ValueType::FLOAT32:
				return 4;
			case ValueType::FLOAT16:
				return 2;
			case ValueType::INT8:
				return 1;
			case ValueType::INT16:
				return 2;
			case ValueType::INT32:
				return 4;
			case ValueType::UINT8:
				return 1;
			case ValueType::UINT16:
				return 2;
			case ValueType::UINT32:
				return 4;
			default:
				return -1;
		}
	}

	VertexFormat VertexFormat::PositionUVNormalTangent() {
		VertexFormat layout;
		layout.mPosition = 0;
		layout.mUVs = {1};
		layout.mNormal = 2;
		layout.mTangent = 3;

		std::vector<LayoutElement> layoutElements = {
			LayoutElement(0, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(1, 0, 2, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(2, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(3, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}

	VertexFormat VertexFormat::PositionUVNormal() {
		VertexFormat layout;
		layout.mPosition = 0;
		layout.mUVs = {1};
		layout.mNormal = 2;

		std::vector<LayoutElement> layoutElements = {
			LayoutElement(0, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(1, 0, 2, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(2, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}

	VertexFormat VertexFormat::PositionUVNormalTangentBitangent() {
		VertexFormat layout;
		layout.mPosition = 0;
		layout.mUVs = {1};
		layout.mNormal = 2;
		layout.mTangent = 3;
		layout.mBitangent = 4;

		std::vector<LayoutElement> layoutElements = {
			LayoutElement(0, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(1, 0, 2, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(2, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(3, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(4, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}

	VertexFormat VertexFormat::PositionUV() {
		VertexFormat layout;
		layout.mPosition = 0;
		layout.mUVs = {1};

		std::vector<LayoutElement> layoutElements = {
			LayoutElement(0, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
			LayoutElement(1, 0, 2, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX),
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}

	VertexFormat VertexFormat::Position() {
		VertexFormat layout;
		layout.mPosition = 0;

		std::vector<LayoutElement> layoutElements = {
			LayoutElement(0, 0, 3, ValueType::FLOAT32, false, InputElementFrequency::PER_VERTEX)
		};

		layout.mElements = std::move(layoutElements);
		return layout;
	}
}