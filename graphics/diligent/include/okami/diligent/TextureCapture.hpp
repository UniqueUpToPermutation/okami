#pragma once

#include <RenderDevice.h>
#include <DeviceContext.h>
#include <Texture.h>
#include <Fence.h>
#include <RefCntAutoPtr.hpp>

#include <okami/Texture.hpp>
#include <glm/vec3.hpp>

namespace DG = Diligent;

namespace okami::graphics::diligent {

    // Primarily for debugging purposes
    class TextureCapture {
    private:
        DG::RefCntAutoPtr<DG::IFence> mFence;
        DG::RefCntAutoPtr<DG::ITexture> mStagingTexture;
    
    public:
        TextureCapture(
            DG::IRenderDevice* device,
            DG::IDeviceContext* context,
            DG::ITexture* texture);

        TextureCapture(TextureCapture&&) = default;
        TextureCapture(const TextureCapture&) = delete;

        TextureCapture& operator=(TextureCapture&&) = default;
        TextureCapture& operator=(const TextureCapture&) = delete;

        bool IsReady();
        core::Texture GetResults(DG::IDeviceContext* context);
    };

    class TextureCapturePick {
    private:
        DG::RefCntAutoPtr<DG::IFence> mFence;
        DG::RefCntAutoPtr<DG::ITexture> mStagingTexture;
        size_t mMaxQueries = 0;
        size_t mRequestedQueries = 0;
        DG::Uint64 mCompletedValue = 0;

    public:
        TextureCapturePick(
            DG::IRenderDevice* device,
            const DG::TextureDesc& desc,
            size_t maxQueries);

        TextureCapturePick() = default;
        TextureCapturePick(const TextureCapturePick&) = delete;
        TextureCapturePick(TextureCapturePick&&) = default;
        TextureCapturePick& operator=(const TextureCapturePick&) = delete;
        TextureCapturePick& operator=(TextureCapturePick&&) = default;

        void SubmitCommands(
            DG::IDeviceContext* context,
            DG::ITexture* srcTexture,
            const glm::vec3 picks[],
            uint pickCount);

        bool IsReady();
        size_t GetMaxQueries() const;
        size_t GetRequestedQueryCount() const;

        void GetResults(DG::IDeviceContext* context, void* dest);
    };
}