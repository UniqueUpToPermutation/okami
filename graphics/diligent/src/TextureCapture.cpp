#include <okami/diligent/TextureCapture.hpp>
#include <okami/diligent/GraphicsUtils.hpp>

#include <cstring>

namespace okami::graphics::diligent {
    TextureCapture::TextureCapture(
        DG::IRenderDevice* device,
        DG::IDeviceContext* context,
        DG::ITexture* texture) {

        DG::FenceDesc desc;
        desc.Name = "Texture Capture Fence";
        desc.Type = DG::FENCE_TYPE_CPU_WAIT_ONLY;

        DG::IFence* fence = nullptr;
        device->CreateFence(desc, &fence);
        mFence.Attach(fence);

        auto texDesc = texture->GetDesc();
        texDesc.CPUAccessFlags = DG::CPU_ACCESS_READ;
        texDesc.Usage = DG::USAGE_STAGING;

        DG::ITexture* stagingTexture = nullptr;
        device->CreateTexture(texDesc, nullptr, &stagingTexture);
        mStagingTexture.Attach(stagingTexture);

        for (size_t slice = 0; slice < texDesc.ArraySize; ++slice) {
            for (size_t mip = 0; mip < texDesc.MipLevels; ++mip) {
                DG::CopyTextureAttribs attribs;
                attribs.DstMipLevel = mip;
                attribs.SrcMipLevel = mip;

                attribs.DstSlice = slice;
                attribs.SrcSlice = slice;

                attribs.DstTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
                attribs.SrcTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

                attribs.pDstTexture = stagingTexture;
                attribs.pSrcTexture = texture;

                context->CopyTexture(attribs);
            }
        }

        context->EnqueueSignal(fence, 1);
    }

    bool TextureCapture::IsReady() {
        return mFence->GetCompletedValue() == 1;
    }

    core::Texture TextureCapture::GetResults(
        DG::IDeviceContext* context) {
        mFence->Wait(1);

        auto& gpuDesc = mStagingTexture->GetDesc();

        core::Texture::Desc cpuDesc;
        cpuDesc.mArraySizeOrDepth = gpuDesc.ArraySizeOrDepth();
        cpuDesc.mFormat = ToOkami(gpuDesc.Format);
        cpuDesc.mHeight = gpuDesc.Height;
        cpuDesc.mMipLevels = gpuDesc.MipLevels;
        cpuDesc.mSampleCount = gpuDesc.SampleCount;
        cpuDesc.mType = ToOkami(gpuDesc.Type);
        cpuDesc.mWidth = gpuDesc.Width;

        core::Texture::Data data = core::Texture::Data::Alloc(cpuDesc);
        auto subresources = cpuDesc.GetSubresourceDescs();

        for (auto& subresource : subresources) {
            DG::MappedTextureSubresource subres;
            context->MapTextureSubresource(
                mStagingTexture.RawPtr(),
                subresource.mMip, 
                subresource.mSlice,
                DG::MAP_TYPE::MAP_READ,
                DG::MAP_FLAG_DO_NOT_WAIT,
                nullptr,
                subres);

            std::memcpy(
                &data.mData[subresource.mSrcOffset],
                subres.pData, 
                subresource.mLength);

            context->UnmapTextureSubresource(
                mStagingTexture.RawPtr(), 
                subresource.mMip, 
                subresource.mSlice);
        }

        return core::Texture(std::move(data));
    }

    TextureCapturePick::TextureCapturePick(
        DG::IRenderDevice* device,
        const DG::TextureDesc& desc,
        size_t maxQueries) {
        DG::FenceDesc fenceDesc;
        fenceDesc.Name = "Texture Capture Pick Fence";
        fenceDesc.Type = DG::FENCE_TYPE_CPU_WAIT_ONLY;

        DG::IFence* fence = nullptr;
        device->CreateFence(fenceDesc, &fence);
        mFence.Attach(fence);

        auto texDesc = desc;
        texDesc.CPUAccessFlags = DG::CPU_ACCESS_READ;
        texDesc.Usage = DG::USAGE_STAGING;
        texDesc.Width = maxQueries;
        texDesc.Height = 1;
        texDesc.Depth = 1;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;

        DG::ITexture* stagingTexture = nullptr;
        device->CreateTexture(texDesc, nullptr, &stagingTexture);
        mStagingTexture.Attach(stagingTexture);

        mMaxQueries = maxQueries;
    }

    void TextureCapturePick::SubmitCommands(
        DG::IDeviceContext* context,
        DG::ITexture* srcTexture,
        const glm::vec3 picks[],
        uint pickCount) {

        assert(IsReady());

        if (pickCount > mMaxQueries) {
            throw std::runtime_error("Number of picks exceeds"
                "max queries.");
        }

        DG::CopyTextureAttribs attribs;
        attribs.DstMipLevel = 0;
        attribs.SrcMipLevel = 0;

        attribs.DstSlice = 0;
        attribs.SrcSlice = 0;

        attribs.DstTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        attribs.SrcTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        attribs.pDstTexture = mStagingTexture;
        attribs.pSrcTexture = srcTexture;

        auto& texDesc = srcTexture->GetDesc();
        uint idx = 0;

        for (uint i = 0; i < pickCount; ++i) {

            auto& pick = picks[i];

            DG::Box box;
            box.MinX = 
                std::clamp<uint>((uint)pick.x, 0, texDesc.Width - 1);
            box.MinY = 
                std::clamp<uint>((uint)pick.y, 0, texDesc.Height - 1); 
            box.MinZ = 
                std::clamp<uint>((uint)pick.z, 0, texDesc.Depth - 1); 
            box.MaxX = box.MinX + 1;
            box.MaxY = box.MinY + 1;
            box.MaxZ = box.MinZ + 1;

            attribs.pSrcBox = &box;
            attribs.DstX = idx;
            attribs.DstY = 0;
            attribs.DstZ = 0;

            context->CopyTexture(attribs);
        }

        ++mCompletedValue;
        context->EnqueueSignal(mFence, mCompletedValue);

        mRequestedQueries = pickCount;
    }

    void TextureCapturePick::GetResults(
        DG::IDeviceContext* context, 
        void* dest) {

        mFence->Wait(mCompletedValue);

        auto& gpuDesc = mStagingTexture->GetDesc();

        DG::MappedTextureSubresource subres;
        context->MapTextureSubresource(
            mStagingTexture.RawPtr(),
            0, 
            0,
            DG::MAP_TYPE::MAP_READ,
            DG::MAP_FLAG_DO_NOT_WAIT,
            nullptr,
            subres);

        size_t byteSize = 
            mRequestedQueries * GetPixelByteSize(gpuDesc.Format);

        std::memcpy(
            dest,
            subres.pData,
            byteSize);

        context->UnmapTextureSubresource(
            mStagingTexture.RawPtr(), 
            0, 
            0);

        mRequestedQueries = 0;
    }

    bool TextureCapturePick::IsReady() {
        return mFence->GetCompletedValue() == mCompletedValue;
    }

    size_t TextureCapturePick::GetMaxQueries() const {
        return mMaxQueries;
    }

    size_t TextureCapturePick::GetRequestedQueryCount() const {
        return mRequestedQueries;
    }
}