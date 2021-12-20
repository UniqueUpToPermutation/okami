#include <okami/GraphicsUtils.hpp>
#include <okami/Display.hpp>

#include <RenderDevice.h>
#include <SwapChain.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>

#if D3D11_SUPPORTED
#    include <EngineFactoryD3D11.h>
#endif
#if D3D12_SUPPORTED
#    include <EngineFactoryD3D12.h>
#endif
#if GL_SUPPORTED
#    include <EngineFactoryOpenGL.h>
#endif
#if VULKAN_SUPPORTED
#    include <EngineFactoryVk.h>
#endif
#if METAL_SUPPORTED
#    include <EngineFactoryMtl.h>
#endif

using namespace Diligent;

namespace okami::graphics {
    void CreateDeviceAndSwapChain(
        IDisplay* display,
        INativeWindowProvider* windowProvider,
        IEngineFactory** factory,
        IRenderDevice** device,
        std::vector<IDeviceContext*>& contexts,
        ISwapChain** swapChain,
        const get_engine_initialization_attribs& attribsFunc) {
        SwapChainDesc SCDesc;

        //auto size = display->GetFramebufferSize();
        //auto isFullscreen = display->GetIsFullscreen();
        auto backend = display->GetRequestedBackend();
        auto window = windowProvider->GetWindow();

        RENDER_DEVICE_TYPE deviceType = RENDER_DEVICE_TYPE_UNDEFINED;

        switch (backend) {
            case GraphicsBackend::D3D11:
                deviceType = RENDER_DEVICE_TYPE_D3D11;
                break;
            case GraphicsBackend::D3D12:
                deviceType = RENDER_DEVICE_TYPE_D3D12;
                break;
            case GraphicsBackend::VULKAN:
                deviceType = RENDER_DEVICE_TYPE_VULKAN;
                break;
            case GraphicsBackend::OPENGL:
                deviceType = RENDER_DEVICE_TYPE_GL;
                break;
        }

#ifndef NDEBUG
        int validationLevel = 1;
#else 
        int validationLevel = 0;
#endif

        bool bForceNonSeprblProgs = false;

#if PLATFORM_MACOS
        // We need at least 3 buffers in Metal to avoid massive
        // performance degradation in full screen mode.
        // https://github.com/KhronosGroup/MoltenVK/issues/808
        SCDesc.BufferCount = 3;
#endif

        Uint32 NumImmediateContexts = 0;

        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
            {
#    if ENGINE_DLL
                // Load the dll and import GetEngineFactoryD3D11() function
                auto GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#    endif
                auto* pFactoryD3D11 = GetEngineFactoryD3D11();
                m_pEngineFactory    = pFactoryD3D11;

                EngineD3D11CreateInfo EngineCI;
                EngineCI.GraphicsAPIVersion = {11, 0};

#    ifdef DILIGENT_DEBUG
                EngineCI.SetValidationLevel(VALIDATION_LEVEL_2);
#    endif
                if (validationLevel >= 0)
                    EngineCI.SetValidationLevel(static_cast<VALIDATION_LEVEL>(validationLevel));

                Uint32 NumAdapters = 0;
                pFactoryD3D11->EnumerateAdapters(EngineCI.GraphicsAPIVersion, NumAdapters, nullptr);
                std::vector<GraphicsAdapterInfo> Adapters(NumAdapters);
                if (NumAdapters > 0)
                {
                    pFactoryD3D11->EnumerateAdapters(EngineCI.GraphicsAPIVersion, NumAdapters, Adapters.data());
                }
                else
                {
                    LOG_ERROR_AND_THROW("Failed to find Direct3D11-compatible hardware adapters");
                }

                EngineCI.AdapterId = m_AdapterId;
                if (m_AdapterType == ADAPTER_TYPE_SOFTWARE)
                {
                    for (Uint32 i = 0; i < Adapters.size(); ++i)
                    {
                        if (Adapters[i].Type == m_AdapterType)
                        {
                            EngineCI.AdapterId = i;
                            LOG_INFO_MESSAGE("Found software adapter '", Adapters[i].Description, "'");
                            break;
                        }
                    }
                }

                m_TheSample->ModifyEngineInitInfo({pFactoryD3D11, m_DeviceType, EngineCI, SCDesc});

                m_AdapterAttribs = Adapters[EngineCI.AdapterId];
                if (m_AdapterType != ADAPTER_TYPE_SOFTWARE)
                {
                    Uint32 NumDisplayModes = 0;
                    pFactoryD3D11->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, nullptr);
                    m_DisplayModes.resize(NumDisplayModes);
                    pFactoryD3D11->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, m_DisplayModes.data());
                }

                NumImmediateContexts = std::max(1u, EngineCI.NumImmediateContexts);
                contexts.resize(NumImmediateContexts + EngineCI.NumDeferredContexts);
                pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, device, contexts.data());
                if (!m_pDevice)
                {
                    LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Direct3D11 mode. The API may not be available, "
                                        "or required features may not be supported by this GPU/driver/OS version.");
                }

                if (pWindow != nullptr)
                    pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, contexts[0], SCDesc, FullScreenModeDesc{}, *pWindow, swapChain);
            }
            break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
            {
#    if ENGINE_DLL
                // Load the dll and import GetEngineFactoryD3D12() function
                auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#    endif
                auto* pFactoryD3D12 = GetEngineFactoryD3D12();
                if (!pFactoryD3D12->LoadD3D12())
                {
                    LOG_ERROR_AND_THROW("Failed to load Direct3D12");
                }
                m_pEngineFactory = pFactoryD3D12;

                EngineD3D12CreateInfo EngineCI;
                EngineCI.GraphicsAPIVersion = {11, 0};
                if (validationLevel >= 0)
                    EngineCI.SetValidationLevel(static_cast<VALIDATION_LEVEL>(validationLevel));

                Uint32 NumAdapters = 0;
                pFactoryD3D12->EnumerateAdapters(EngineCI.GraphicsAPIVersion, NumAdapters, nullptr);
                std::vector<GraphicsAdapterInfo> Adapters(NumAdapters);
                if (NumAdapters > 0)
                {
                    pFactoryD3D12->EnumerateAdapters(EngineCI.GraphicsAPIVersion, NumAdapters, Adapters.data());
                }
                else
                {
#    if D3D11_SUPPORTED
                    LOG_ERROR_MESSAGE("Failed to find Direct3D12-compatible hardware adapters. Attempting to initialize the engine in Direct3D11 mode.");
                    m_DeviceType = RENDER_DEVICE_TYPE_D3D11;
                    InitializeDiligentEngine(pWindow);
                    return;
#    else
                    LOG_ERROR_AND_THROW("Failed to find Direct3D12-compatible hardware adapters.");
#    endif
                }

                EngineCI.AdapterId = m_AdapterId;
                if (m_AdapterType == ADAPTER_TYPE_SOFTWARE)
                {
                    for (Uint32 i = 0; i < Adapters.size(); ++i)
                    {
                        if (Adapters[i].Type == m_AdapterType)
                        {
                            EngineCI.AdapterId = i;
                            LOG_INFO_MESSAGE("Found software adapter '", Adapters[i].Description, "'");
                            break;
                        }
                    }
                }

                m_TheSample->ModifyEngineInitInfo({pFactoryD3D12, m_DeviceType, EngineCI, SCDesc});

                m_AdapterAttribs = Adapters[EngineCI.AdapterId];
                if (m_AdapterType != ADAPTER_TYPE_SOFTWARE)
                {
                    Uint32 NumDisplayModes = 0;
                    pFactoryD3D12->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, nullptr);
                    m_DisplayModes.resize(NumDisplayModes);
                    pFactoryD3D12->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, m_DisplayModes.data());
                }

                NumImmediateContexts = std::max(1u, EngineCI.NumImmediateContexts);
                contexts.resize(NumImmediateContexts + EngineCI.NumDeferredContexts);
                pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, device, contexts.data());
                if (!m_pDevice)
                {
                    LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Direct3D12 mode. The API may not be available, "
                                        "or required features may not be supported by this GPU/driver/OS version.");
                }

                if (!m_pSwapChain && pWindow != nullptr)
                    pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, contexts[0], SCDesc, FullScreenModeDesc{}, *pWindow, swapChain);
            }
            break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
            {
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
                // Load the dll and import GetEngineFactoryOpenGL() function
                auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
#    endif
                auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
                *factory             = pFactoryOpenGL;

                EngineGLCreateInfo EngineCI;
                EngineCI.Window = window;

                if (validationLevel >= 0)
                    EngineCI.SetValidationLevel(static_cast<VALIDATION_LEVEL>(validationLevel));

                attribsFunc(deviceType, EngineCI, SCDesc);

                if (bForceNonSeprblProgs)
                    EngineCI.Features.SeparablePrograms = DEVICE_FEATURE_STATE_DISABLED;
                if (EngineCI.NumDeferredContexts != 0)
                {
                    LOG_ERROR_MESSAGE("Deferred contexts are not supported in OpenGL mode");
                    EngineCI.NumDeferredContexts = 0;
                }

                NumImmediateContexts = 1;
                contexts.resize(NumImmediateContexts + EngineCI.NumDeferredContexts);
                pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, device, contexts.data(), SCDesc, swapChain);
                if (!*device)
                {
                    LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in OpenGL mode. The API may not be available, "
                                        "or required features may not be supported by this GPU/driver/OS version.");
                }
            }
            break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
            {
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
                // Load the dll and import GetEngineFactoryVk() function
                auto GetEngineFactoryVk = LoadGraphicsEngineVk();
#    endif
                EngineVkCreateInfo EngineCI;
                if (validationLevel >= 0)
                    EngineCI.SetValidationLevel(static_cast<VALIDATION_LEVEL>(validationLevel));

                auto* pFactoryVk = GetEngineFactoryVk();
                *factory = pFactoryVk;

                attribsFunc(deviceType, EngineCI, SCDesc);

                NumImmediateContexts = std::max(1u, EngineCI.NumImmediateContexts);
                contexts.resize(NumImmediateContexts + EngineCI.NumDeferredContexts);
                pFactoryVk->CreateDeviceAndContextsVk(EngineCI, device, contexts.data());
                if (!*device)
                {
                    LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Vulkan mode. The API may not be available, "
                                        "or required features may not be supported by this GPU/driver/OS version.");
                }

                pFactoryVk->CreateSwapChainVk(*device, contexts[0], SCDesc, window, swapChain);
            }
            break;
#endif


#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
            {
                EngineMtlCreateInfo EngineCI;
                if (validationLevel >= 0)
                    EngineCI.SetValidationLevel(static_cast<VALIDATION_LEVEL>(validationLevel));

                auto* pFactoryMtl = GetEngineFactoryMtl();
                m_pEngineFactory  = pFactoryMtl;

                m_TheSample->ModifyEngineInitInfo({pFactoryMtl, m_DeviceType, EngineCI, SCDesc});

                NumImmediateContexts = std::max(1u, EngineCI.NumImmediateContexts);
                contexts.resize(NumImmediateContexts + EngineCI.NumDeferredContexts);
                pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, device, contexts.data());
                if (!m_pDevice)
                {
                    LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Metal mode. The API may not be available, "
                                        "or required features may not be supported by this GPU/driver/OS version.");
                }

                if (!m_pSwapChain && pWindow != nullptr)
                    pFactoryMtl->CreateSwapChainMtl(m_pDevice, contexts[0], SCDesc, *pWindow, swapChain);
            }
            break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unknown device type");
                break;
        }
    }

    DG::RESOURCE_DIMENSION ToDiligent(core::ResourceDimension dim) {
        switch (dim) {
        case core::ResourceDimension::Buffer:
            return RESOURCE_DIM_BUFFER;
        case core::ResourceDimension::Texture1D:
            return RESOURCE_DIM_TEX_1D;
        case core::ResourceDimension::Texture1DArray:
            return RESOURCE_DIM_TEX_1D_ARRAY;
        case core::ResourceDimension::Texture2D:
            return RESOURCE_DIM_TEX_2D;
        case core::ResourceDimension::Texture2DArray:
            return RESOURCE_DIM_TEX_2D_ARRAY;
        case core::ResourceDimension::Texture3D:
            return RESOURCE_DIM_TEX_3D;
        case core::ResourceDimension::TextureCube:
            return RESOURCE_DIM_TEX_CUBE;
        case core::ResourceDimension::TextureCubeArray:
            return RESOURCE_DIM_TEX_CUBE_ARRAY;
        default:
            throw std::runtime_error("Unrecognized ResourceDimension type!");
        }
    }

    DG::VALUE_TYPE ToDiligent(core::ValueType valueType) {
        switch (valueType) {
        case core::ValueType::INT8:
            return VT_INT8;
        case core::ValueType::INT16:
            return VT_UINT16;
        case core::ValueType::INT32:
            return VT_UINT32;
        case core::ValueType::UINT8:
            return VT_UINT8;
        case core::ValueType::UINT16:
            return VT_UINT16;
        case core::ValueType::UINT32:
            return VT_UINT32;
        case core::ValueType::FLOAT16:
            return VT_FLOAT16;
        case core::ValueType::FLOAT32:
            return VT_FLOAT32;
        default:
            throw std::runtime_error("Unrecognized value type!");
        }
    }

    DG::INPUT_ELEMENT_FREQUENCY ToDiligent(core::InputElementFrequency frequency) {
        switch (frequency) {
        case core::InputElementFrequency::PER_INSTANCE:
            return DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
        case core::InputElementFrequency::PER_VERTEX:
            return DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        default:
            throw std::runtime_error("Uncognized input element frequency!");    
        }
    }

    InputLayoutDiligent ToDiligent(const core::VertexLayout& layout) {
        InputLayoutDiligent result;

        for (auto& element : layout.mElements) {
            DG::LayoutElement dg_element;
            dg_element.BufferSlot = element.mBufferSlot;
            dg_element.Frequency = ToDiligent(element.mFrequency);
            dg_element.InputIndex = element.mInputIndex;
            dg_element.InstanceDataStepRate = element.mInstanceDataStepRate;
            dg_element.IsNormalized = element.bIsNormalized;
            dg_element.NumComponents = element.mNumComponents;
            dg_element.RelativeOffset = element.mRelativeOffset;
            dg_element.Stride = element.mStride;
            dg_element.ValueType = ToDiligent(element.mValueType);
            
            result.mElements.emplace_back(std::move(dg_element));
        }

        return result;
    }

    DG::TEXTURE_FORMAT ToDiligent(const core::TextureFormat& format) {
        switch (format.mValueType) {
        case core::ValueType::FLOAT32:
            switch (format.mChannels) {
            case 1:
                return DG::TEX_FORMAT_R32_FLOAT;
            case 2:
                return DG::TEX_FORMAT_RG32_FLOAT;
            case 3:
                return DG::TEX_FORMAT_RGB32_FLOAT;
            case 4:
                return DG::TEX_FORMAT_RGBA32_FLOAT;
            default:
                throw std::runtime_error("Invalid format!");
            }
        case core::ValueType::FLOAT16:
            switch (format.mChannels) {
            case 1:
                return DG::TEX_FORMAT_R16_FLOAT;
            case 2:
                return DG::TEX_FORMAT_RG16_FLOAT;
            case 4:
                return DG::TEX_FORMAT_RGBA16_FLOAT;
            default:
                throw std::runtime_error("Invalid format!");
            }
        case core::ValueType::UINT8:
            if (format.bNormalized) {
                if (format.bLinear) {
                    switch (format.mChannels) {
                    case 1:
                        return DG::TEX_FORMAT_R8_UNORM;
                    case 2:
                        return DG::TEX_FORMAT_RG8_UNORM;
                    case 4:
                        return DG::TEX_FORMAT_RGBA8_UNORM;
                    default:
                        throw std::runtime_error("Invalid format!");
                    }
                } else {
                    switch (format.mChannels) {
                    case 4:
                        return DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
                    default:
                        throw std::runtime_error("Invalid format!");
                    }
                }
            } else {
                switch (format.mChannels) {
                case 1:
                    return DG::TEX_FORMAT_R8_UINT;
                case 2:
                    return DG::TEX_FORMAT_RG8_UINT;
                case 4:
                    return DG::TEX_FORMAT_RGBA8_UINT;
                default:
                    throw std::runtime_error("Invalid format!");
                }
            }
        case core::ValueType::UINT16:
            if (format.bNormalized) {
                if (format.bLinear) {
                    switch (format.mChannels) {
                    case 1:
                        return DG::TEX_FORMAT_R16_UNORM;
                    case 2:
                        return DG::TEX_FORMAT_RG16_UNORM;
                    case 4:
                        return DG::TEX_FORMAT_RGBA16_UNORM;
                    default:
                        throw std::runtime_error("Invalid format!");
                    }
                } else {
                    throw std::runtime_error("Invalid format!");
                }
            } else {
                switch (format.mChannels) {
                case 1:
                    return DG::TEX_FORMAT_R16_UINT;
                case 2:
                    return DG::TEX_FORMAT_RG16_UINT;
                case 4:
                    return DG::TEX_FORMAT_RGBA16_UINT;
                default:
                    throw std::runtime_error("Invalid format!");
                }
            }
        case core::ValueType::UINT32:
            switch (format.mChannels) {
            case 1:
                return DG::TEX_FORMAT_R32_UINT;
            case 2:
                return DG::TEX_FORMAT_RG32_UINT;
            case 4:
                return DG::TEX_FORMAT_RGBA32_UINT;
            default:
                throw std::runtime_error("Invalid format!");
            }
        case core::ValueType::INT8:
            if (format.bNormalized) {
                switch (format.mChannels) {
                case 1:
                    return DG::TEX_FORMAT_R8_SNORM;
                case 2:
                    return DG::TEX_FORMAT_RG8_SNORM;
                case 4:
                    return DG::TEX_FORMAT_RGBA8_SNORM;
                default:
                    throw std::runtime_error("Invalid format!");
                }
            } else {
                switch (format.mChannels) {
                case 1:
                    return DG::TEX_FORMAT_R8_SINT;
                case 2:
                    return DG::TEX_FORMAT_RG8_SINT;
                case 4:
                    return DG::TEX_FORMAT_RGBA8_SINT;
                default:
                    throw std::runtime_error("Invalid format!");
                }
            }
        case core::ValueType::INT16:
            if (format.bNormalized) {
                switch (format.mChannels) {
                case 1:
                    return DG::TEX_FORMAT_R16_SNORM;
                case 2:
                    return DG::TEX_FORMAT_RG16_SNORM;
                case 4:
                    return DG::TEX_FORMAT_RGBA16_SNORM;
                default:
                    throw std::runtime_error("Invalid format!");
                }
            } else {
                switch (format.mChannels) {
                case 1:
                    return DG::TEX_FORMAT_R16_SINT;
                case 2:
                    return DG::TEX_FORMAT_RG16_SINT;
                case 4:
                    return DG::TEX_FORMAT_RGBA16_SINT;
                default:
                    throw std::runtime_error("Invalid format!");
                }
            }
        case core::ValueType::INT32:
            switch (format.mChannels) {
            case 1:
                return DG::TEX_FORMAT_R32_SINT;
            case 2:
                return DG::TEX_FORMAT_RG32_SINT;
            case 4:
                return DG::TEX_FORMAT_RGBA32_SINT;
            default:
                throw std::runtime_error("Invalid format!");
            }
        default:
            throw std::runtime_error("Invalid format!");
        }
    }
}