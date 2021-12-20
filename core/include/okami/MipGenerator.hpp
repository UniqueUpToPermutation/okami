#pragma once

#include <cmath>

namespace okami::core {
	typedef void (*mip_generator_2d_t)(uint32_t NumChannels,
		bool IsSRGB,
		const void* pFineMip,
		uint32_t FineMipStride,
		uint32_t FineMipWidth,
		uint32_t FineMipHeight,
		void* pCoarseMip,
		uint32_t CoarseMipStride,
		uint32_t CoarseMipWidth,
		uint32_t CoarseMipHeight);

    inline float LinearToSRGB(float x)
	{
		return x <= 0.0031308 ? x * 12.92f : 1.055f * std::pow(x, 1.f / 2.4f) - 0.055f;
	}

	inline float SRGBToLinear(float x)
	{
		return x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
	}

	class LinearToSRGBMap
	{
	public:
		LinearToSRGBMap() noexcept
		{
			for (uint i = 0; i < m_ToSRBG.size(); ++i)
			{
				m_ToSRBG[i] = LinearToSRGB(static_cast<float>(i) / 255.f);
			}
		}

		float operator[](uint8_t x) const
		{
			return m_ToSRBG[x];
		}

	private:
		std::array<float, 256> m_ToSRBG;
	};

	class SRGBToLinearMap
	{
	public:
		SRGBToLinearMap() noexcept
		{
			for (uint i = 0; i < m_ToLinear.size(); ++i)
			{
				m_ToLinear[i] = SRGBToLinear(static_cast<float>(i) / 255.f);
			}
		}

		float operator[](uint8_t x) const
		{
			return m_ToLinear[x];
		}

	private:
		std::array<float, 256> m_ToLinear;
	};

	float LinearToSRGB(uint8_t x)
	{
		static const LinearToSRGBMap map;
		return map[x];
	}

	float SRGBToLinear(uint8_t x)
	{
		static const SRGBToLinearMap map;
		return map[x];
	}

	template <typename ChannelType>
	ChannelType SRGBAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3)
	{
		static constexpr float NormVal = static_cast<float>(std::numeric_limits<ChannelType>::max());

		float fc0 = static_cast<float>(c0) / NormVal;
		float fc1 = static_cast<float>(c1) / NormVal;
		float fc2 = static_cast<float>(c2) / NormVal;
		float fc3 = static_cast<float>(c3) / NormVal;

		float fLinearAverage = (SRGBToLinear(fc0) + SRGBToLinear(fc1) + SRGBToLinear(fc2) + SRGBToLinear(fc3)) / 4.f;
		float fSRGBAverage   = LinearToSRGB(fLinearAverage) * NormVal;

		static constexpr float MinVal = static_cast<float>(std::numeric_limits<ChannelType>::min());
		static constexpr float MaxVal = static_cast<float>(std::numeric_limits<ChannelType>::max());

		fSRGBAverage = std::max(fSRGBAverage, MinVal);
		fSRGBAverage = std::min(fSRGBAverage, MaxVal);

		return static_cast<ChannelType>(fSRGBAverage);
	}

	template <typename ChannelType>
	ChannelType LinearAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3)
	{
		static_assert(std::numeric_limits<ChannelType>::is_integer && !std::numeric_limits<ChannelType>::is_signed, "Unsigned integers are expected");
		return static_cast<ChannelType>((static_cast<uint32_t>(c0) + static_cast<uint32_t>(c1) + static_cast<uint32_t>(c2) + static_cast<uint32_t>(c3)) / 4);
	}

	template <>
	float LinearAverage<float>(float c0, float c1, float c2, float c3) {
		return (c0 + c1 + c2 + c3) / 4.0f;
	}

	template <typename ChannelType>
	void ComputeCoarseMip2D(uint32_t    NumChannels,
						bool            IsSRGB,
						const void*     pFineMip,
						uint32_t        FineMipStride,
						uint32_t        FineMipWidth,
						uint32_t        FineMipHeight,
						void*           pCoarseMip,
						uint32_t        CoarseMipStride,
						uint32_t        CoarseMipWidth,
						uint32_t        CoarseMipHeight)
	{
		assert(FineMipWidth > 0 && FineMipHeight > 0 && FineMipStride > 0);
		assert(CoarseMipWidth > 0 && CoarseMipHeight > 0 && CoarseMipStride > 0);

		for (uint32_t row = 0; row < CoarseMipHeight; ++row)
		{
			auto src_row0 = row * 2;
			auto src_row1 = std::min(row * 2 + 1, FineMipHeight - 1);

			auto pSrcRow0 = (reinterpret_cast<const ChannelType*>(pFineMip) + src_row0 * FineMipStride);
			auto pSrcRow1 = (reinterpret_cast<const ChannelType*>(pFineMip) + src_row1 * FineMipStride);

			for (uint32_t col = 0; col < CoarseMipWidth; ++col)
			{
				auto src_col0 = col * 2;
				auto src_col1 = std::min(col * 2 + 1, FineMipWidth - 1);

				for (uint32_t c = 0; c < NumChannels; ++c)
				{
					auto Chnl00 = pSrcRow0[src_col0 * NumChannels + c];
					auto Chnl01 = pSrcRow0[src_col1 * NumChannels + c];
					auto Chnl10 = pSrcRow1[src_col0 * NumChannels + c];
					auto Chnl11 = pSrcRow1[src_col1 * NumChannels + c];

					auto& DstCol = (reinterpret_cast<ChannelType*>(pCoarseMip) + row * CoarseMipStride)[col * NumChannels + c];
					
                    if (IsSRGB)
						DstCol = SRGBAverage(Chnl00, Chnl01, Chnl10, Chnl11);
					else
						DstCol = LinearAverage(Chnl00, Chnl01, Chnl10, Chnl11);
				}
			}
		}
	}
}