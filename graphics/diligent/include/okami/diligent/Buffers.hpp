#pragma once

#include <RenderDevice.h>
#include <MapHelper.hpp>

#include <cstring>

namespace okami::graphics::diligent {
	namespace DG = Diligent;

    template <typename T>
	class StaticUniformBuffer {
	private:
		DG::RefCntAutoPtr<DG::IBuffer> 	mBuffer;
		size_t							mCount;

	public:
        StaticUniformBuffer() = default;
		StaticUniformBuffer(
            DG::IRenderDevice* device,
            DG::IDeviceContext* context,
            const std::vector<T>& data) : 
				mCount(data.size()) {

			DG::BufferDesc dg_desc;
			dg_desc.Name           = "Static Uniform Buffer";
			dg_desc.Size  		  = sizeof(T) * mCount;
			dg_desc.Usage          = DG::USAGE_IMMUTABLE;
			dg_desc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			dg_desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;

            DG::BufferData dg_data;
            dg_data.DataSize        = sizeof(T) * mCount;
            dg_data.pContext        = context;
            dg_data.pData           = &data[0];

            DG::IBuffer* buf = nullptr;
			device->CreateBuffer(dg_desc, nullptr, &buf);
            mBuffer.Attach(buf);
		}

		inline DG::IBuffer* Get() {
			return mBuffer.RawPtr();
		}

		inline size_t Count() {
			return mCount;
		}
	};

	template <typename T>
	class DynamicUniformBuffer {
	private:
		DG::RefCntAutoPtr<DG::IBuffer> mBuffer;
		size_t mCount;

	public:
        DynamicUniformBuffer() = default;
		DynamicUniformBuffer(
			DG::IRenderDevice* device, 
			const uint count = 1) : 
			mCount(count) {

			DG::BufferDesc dg_desc;
			dg_desc.Name           = "Dyanmic Globals Buffer";
			dg_desc.Size  		  = sizeof(T) * count;
			dg_desc.Usage          = DG::USAGE_DYNAMIC;
			dg_desc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			dg_desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

            DG::IBuffer* buf = nullptr;
			device->CreateBuffer(dg_desc, nullptr, &buf);
            mBuffer.Attach(buf);
		}

		inline size_t Count() const {
			return mCount;
		}

		inline DG::IBuffer* Get() {
			return mBuffer.RawPtr();
		}

		inline void Write(DG::IDeviceContext* context, const T& t) {
			DG::MapHelper<T> data(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			*data = t;
		}

		inline void Write(DG::IDeviceContext* context, const T t[], const uint count) {
			DG::MapHelper<T> data(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			std::memcpy(data, t, sizeof(T) * count);
		}
	};
}