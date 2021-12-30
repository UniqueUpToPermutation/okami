#include <okami/diligent/GraphicsUtils.hpp>
#include <okami/diligent/Display.hpp>

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

namespace okami::graphics::diligent {
    void CreateDeviceAndSwapChain(
        IDisplay* display,
        INativeWindowProvider* windowProvider,
        IEngineFactory** factory,
        IRenderDevice** device,
        std::vector<IDeviceContext*>& contexts,
        ISwapChain** swapChain,
        const get_engine_initialization_attribs& attribsFunc) {
        SwapChainDesc SCDesc;

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

    InputLayoutDiligent ToDiligent(const core::VertexFormat& layout) {
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

    core::ResourceDimension ToOkami(DG::RESOURCE_DIMENSION dim) {
        switch (dim) {
        case DG::RESOURCE_DIM_TEX_1D:
            return core::ResourceDimension::Texture1D;
        case DG::RESOURCE_DIM_TEX_1D_ARRAY:
            return core::ResourceDimension::Texture1DArray;
        case DG::RESOURCE_DIM_TEX_2D:
            return core::ResourceDimension::Texture2D;
        case DG::RESOURCE_DIM_TEX_2D_ARRAY:
            return core::ResourceDimension::Texture2DArray;
        case DG::RESOURCE_DIM_TEX_3D:
            return core::ResourceDimension::Texture3D;
        case DG::RESOURCE_DIM_TEX_CUBE:
            return core::ResourceDimension::TextureCube;
        case DG::RESOURCE_DIM_TEX_CUBE_ARRAY:
            return core::ResourceDimension::TextureCubeArray;
        default:
            throw std::runtime_error("Unknown dimension.");
        }
    }

    core::ValueType ToOkami(DG::VALUE_TYPE valueType) {
        switch (valueType) {
        case DG::VT_FLOAT16:
            return core::ValueType::FLOAT16;
        case DG::VT_FLOAT32:
            return core::ValueType::FLOAT32;
        case DG::VT_INT16:
            return core::ValueType::INT16;
        case DG::VT_INT32:
            return core::ValueType::INT32;
        case DG::VT_INT8:
            return core::ValueType::INT8;
        case DG::VT_UINT16:
            return core::ValueType::UINT16;
        case DG::VT_UINT32:
            return core::ValueType::UINT32;
        case DG::VT_UINT8:
            return core::ValueType::UINT8;
        default:
            throw std::runtime_error("Unknown type.");
        }
    }

    core::InputElementFrequency ToOkami(DG::INPUT_ELEMENT_FREQUENCY frequency) {
        switch (frequency) {
        case DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE:
            return core::InputElementFrequency::PER_INSTANCE;
        case DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX:
            return core::InputElementFrequency::PER_VERTEX;
        default:
            throw std::runtime_error("Unknown frequency.");
        }
    }

    core::TextureFormat ToOkami(DG::TEXTURE_FORMAT format) {
        switch (format) {
        case DG::TEX_FORMAT_R8_SINT:
            return core::TextureFormat::R8_SINT();
        case DG::TEX_FORMAT_R8_SNORM:
            return core::TextureFormat::R8_SNORM();
        case DG::TEX_FORMAT_R8_UINT:
            return core::TextureFormat::R8_UINT();
        case DG::TEX_FORMAT_R8_UNORM:
            return core::TextureFormat::R8_UNORM();
        case DG::TEX_FORMAT_RG8_SINT:
            return core::TextureFormat::RG8_SINT();
        case DG::TEX_FORMAT_RG8_SNORM:
            return core::TextureFormat::RG8_SNORM();
        case DG::TEX_FORMAT_RG8_UINT:
            return core::TextureFormat::RG8_UINT();
        case DG::TEX_FORMAT_RG8_UNORM:
            return core::TextureFormat::RG8_UNORM();
        case DG::TEX_FORMAT_RGBA8_SINT:
            return core::TextureFormat::RGBA8_SINT();
        case DG::TEX_FORMAT_RGBA8_SNORM:
            return core::TextureFormat::RGBA8_SNORM();
        case DG::TEX_FORMAT_RGBA8_UINT:
            return core::TextureFormat::RGBA8_UINT();
        case DG::TEX_FORMAT_RGBA8_UNORM:
            return core::TextureFormat::RGBA8_UNORM();
        case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
            return core::TextureFormat::SRGBA8_UNORM();
        case DG::TEX_FORMAT_R16_UINT:
            return core::TextureFormat::R16_UINT();
        case DG::TEX_FORMAT_R16_SINT:
            return core::TextureFormat::R16_SINT();
        case DG::TEX_FORMAT_RG16_UINT:
            return core::TextureFormat::RG16_UINT();
        case DG::TEX_FORMAT_RG16_SINT:
            return core::TextureFormat::RG16_SINT();
        case DG::TEX_FORMAT_RGBA16_UINT:
            return core::TextureFormat::RGBA16_UINT();
        case DG::TEX_FORMAT_RGBA16_SINT:
            return core::TextureFormat::RGBA16_SINT();
        case DG::TEX_FORMAT_R16_UNORM:
            return core::TextureFormat::R16_UNORM();
        case DG::TEX_FORMAT_R16_SNORM:
            return core::TextureFormat::R16_SNORM();
        case DG::TEX_FORMAT_RG16_UNORM:
            return core::TextureFormat::RG16_UNORM();
        case DG::TEX_FORMAT_RG16_SNORM:
            return core::TextureFormat::RG16_SNORM();
        case DG::TEX_FORMAT_RGBA16_UNORM:
            return core::TextureFormat::RGBA16_UNORM();
        case DG::TEX_FORMAT_RGBA16_SNORM:
            return core::TextureFormat::RGBA16_SNORM();
        case DG::TEX_FORMAT_R32_SINT:
            return core::TextureFormat::R32_SINT();
        case DG::TEX_FORMAT_R32_UINT:
            return core::TextureFormat::R32_UINT();
        case DG::TEX_FORMAT_R32_FLOAT:
            return core::TextureFormat::R32_FLOAT();
        case DG::TEX_FORMAT_RG32_SINT:
            return core::TextureFormat::RG32_SINT();
        case DG::TEX_FORMAT_RG32_UINT:
            return core::TextureFormat::RG32_UINT();
        case DG::TEX_FORMAT_RG32_FLOAT:
            return core::TextureFormat::RG32_FLOAT();
        case DG::TEX_FORMAT_RGB32_SINT:
            return core::TextureFormat::RGB32_SINT();
        case DG::TEX_FORMAT_RGB32_UINT:
            return core::TextureFormat::RGB32_UINT();
        case DG::TEX_FORMAT_RGB32_FLOAT:
            return core::TextureFormat::RGB32_FLOAT();
        case DG::TEX_FORMAT_RGBA32_SINT:
            return core::TextureFormat::RGBA32_SINT();
        case DG::TEX_FORMAT_RGBA32_UINT:
            return core::TextureFormat::RGBA32_UINT();
        case DG::TEX_FORMAT_RGBA32_FLOAT:
            return core::TextureFormat::RGBA32_FLOAT();
        default:
            throw std::runtime_error("Not supported!");
        }
    }

	DG::float4x4 GetSurfacePretransformMatrix(DG::ISwapChain* swapChain, 
		const DG::float3& f3CameraViewAxis)
	{
		const auto& SCDesc = swapChain->GetDesc();
		switch (SCDesc.PreTransform)
		{
			case  DG::SURFACE_TRANSFORM_ROTATE_90:
				// The image content is rotated 90 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F / 2.f);

			case  DG::SURFACE_TRANSFORM_ROTATE_180:
				// The image content is rotated 180 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F);

			case  DG::SURFACE_TRANSFORM_ROTATE_270:
				// The image content is rotated 270 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F * 3.f / 2.f);

			case  DG::SURFACE_TRANSFORM_OPTIMAL:
				UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
				return DG::float4x4::Identity();

			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
				UNEXPECTED("Mirror transforms are not supported");
				return DG::float4x4::Identity();

			default:
				return DG::float4x4::Identity();
		}
	}

	DG::float4x4 GetAdjustedProjectionMatrix(DG::ISwapChain* swapChain, 
		bool bIsGL, 
		float FOV, 
		float NearPlane, 
		float FarPlane)
	{
		const auto& SCDesc = swapChain->GetDesc();

		float AspectRatio = static_cast<float>(SCDesc.Width) / static_cast<float>(SCDesc.Height);
		float XScale, YScale;
		if (SCDesc.PreTransform == DG::SURFACE_TRANSFORM_ROTATE_90 ||
			SCDesc.PreTransform == DG::SURFACE_TRANSFORM_ROTATE_270 ||
			SCDesc.PreTransform == DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
			SCDesc.PreTransform == DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270)
		{
			// When the screen is rotated, vertical FOV becomes horizontal FOV
			XScale = 1.f / std::tan(FOV / 2.f);
			// Aspect ratio is inversed
			YScale = XScale * AspectRatio;
		}
		else
		{
			YScale = 1.f / std::tan(FOV / 2.f);
			XScale = YScale / AspectRatio;
		}

		DG::float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;
		Proj.SetNearFarClipPlanes(NearPlane, FarPlane, bIsGL);
		return Proj;
	}

	DG::float4x4 GetAdjustedOrthoMatrix(
		bool bIsGL,
		const DG::float2& fCameraSize,
		float zNear, float zFar) {
		float XScale = (float)(2.0 / fCameraSize.x);
		float YScale = (float)(2.0 / fCameraSize.y);

		DG::float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;

		if (bIsGL)
        {
            Proj._33 = -(-(zFar + zNear) / (zFar - zNear));
            Proj._43 = -2 * zNear * zFar / (zFar - zNear);
        }
        else
        {
            Proj._33 = zFar / (zFar - zNear);
            Proj._43 = -zNear * zFar / (zFar - zNear);
        }

		Proj._44 = 1;

		return Proj;
	}

    DG::float4x4 GetProjection(
        const core::Camera& camera,
        DG::ISwapChain* swapChain, 
        bool bIsGL) {
        if (camera.mType == core::Camera::Type::PERSPECTIVE) {
			// Get pretransform matrix that rotates the scene according the surface orientation
			auto srfPreTransform = GetSurfacePretransformMatrix(swapChain, DG::float3{0, 0, 1});

			// Get projection matrix adjusted to the current screen orientation
			auto proj = GetAdjustedProjectionMatrix(swapChain, bIsGL, 
                camera.mFieldOfView, camera.mNearPlane, camera.mFarPlane);
			return srfPreTransform * proj;
		} else if (camera.mType == core::Camera::Type::ORTHOGRAPHIC) {
			// Get pretransform matrix that rotates the scene according the surface orientation
			auto srfPreTransform = GetSurfacePretransformMatrix(swapChain, DG::float3{0, 0, 1});

			// Get projection matrix adjusted to the current screen orientation
			auto proj = GetAdjustedOrthoMatrix(bIsGL, ToDiligent(camera.mOrthoSize), 
                camera.mNearPlane, camera.mFarPlane);
			return srfPreTransform * proj;
		} else {
			throw std::runtime_error("Invalid Camera Type!");
		}
    }

    uint GetTypeSize(DG::VALUE_TYPE type) {
		switch (type) {
			case DG::VT_FLOAT16:
				return sizeof(DG::Uint16);
			case DG::VT_FLOAT32:
				return sizeof(DG::Float32);
			case DG::VT_INT32:
				return sizeof(DG::Int32);
			case DG::VT_INT16:
				return sizeof(DG::Int16);
			case DG::VT_INT8:
				return sizeof(DG::Int8);
			case DG::VT_UINT32:
				return sizeof(DG::Uint32);
			case DG::VT_UINT16:
				return sizeof(DG::Uint16);
			case DG::VT_UINT8:
				return sizeof(DG::Uint8);
			default:
				return 0;
		}
	}

	DG::VALUE_TYPE GetComponentType(DG::TEXTURE_FORMAT texFormat) {
		switch (texFormat) {
			case DG::TEX_FORMAT_RGBA32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_RGBA32_UINT:
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_RGBA32_SINT:    								
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RGB32_FLOAT:  
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_RGB32_UINT:  
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_RGB32_SINT:  
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RGBA16_FLOAT: 
				return DG::VT_FLOAT16;
			case DG::TEX_FORMAT_RGBA16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RGBA16_UINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RGBA16_SNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RGBA16_SINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RG32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_RG32_UINT: 
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_RG32_SINT: 
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RGBA8_UINT: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RGBA8_SNORM: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_RGBA8_SINT: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_RG16_FLOAT: 
				return DG::VT_FLOAT16;
			case DG::TEX_FORMAT_RG16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RG16_UINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RG16_SNORM: 
				return DG::VT_INT16;
			case DG::TEX_FORMAT_RG16_SINT:
				return DG::VT_INT16;
			case DG::TEX_FORMAT_D32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_R32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_R32_UINT: 
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_R32_SINT: 
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RG8_UNORM: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RG8_UINT: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RG8_SNORM: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_RG8_SINT: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_R16_FLOAT: 
				return DG::VT_FLOAT16;
			case DG::TEX_FORMAT_D16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_R16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_R16_UINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_R16_SNORM: 
				return DG::VT_INT16;
			case DG::TEX_FORMAT_R16_SINT: 
				return DG::VT_INT16;
			case DG::TEX_FORMAT_R8_UNORM: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_R8_UINT: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_R8_SNORM: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_R8_SINT: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_A8_UNORM:	
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RG8_B8G8_UNORM: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_G8R8_G8B8_UNORM:
				return DG::VT_UINT8;
			default:
				return DG::VT_NUM_TYPES;
		}
	}

	int GetComponentCount(DG::TEXTURE_FORMAT texFormat) {
		switch (texFormat) {
			case DG::TEX_FORMAT_RGBA32_FLOAT: 
				return 4;
			case DG::TEX_FORMAT_RGBA32_UINT:
				return 4;
			case DG::TEX_FORMAT_RGBA32_SINT:    								
				return 4;
			case DG::TEX_FORMAT_RGB32_FLOAT:  
				return 3;
			case DG::TEX_FORMAT_RGB32_UINT:  
				return 3;
			case DG::TEX_FORMAT_RGB32_SINT:  
				return 3;
			case DG::TEX_FORMAT_RGBA16_FLOAT: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_UNORM: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_UINT: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_SNORM: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_SINT: 
				return 4;
			case DG::TEX_FORMAT_RG32_FLOAT: 
				return 2;
			case DG::TEX_FORMAT_RG32_UINT: 
				return 2;
			case DG::TEX_FORMAT_RG32_SINT: 
				return 2;
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return 4;
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return 4;
			case DG::TEX_FORMAT_RGBA8_UINT: 
				return 4;
			case DG::TEX_FORMAT_RGBA8_SNORM: 
				return 4;
			case DG::TEX_FORMAT_RGBA8_SINT: 
				return 4;
			case DG::TEX_FORMAT_RG16_FLOAT: 
				return 2;
			case DG::TEX_FORMAT_RG16_UNORM: 
				return 2;
			case DG::TEX_FORMAT_RG16_UINT: 
				return 2;
			case DG::TEX_FORMAT_RG16_SNORM: 
				return 2;
			case DG::TEX_FORMAT_RG16_SINT:
				return 2;
			case DG::TEX_FORMAT_D32_FLOAT: 
				return 1;
			case DG::TEX_FORMAT_R32_FLOAT: 
				return 1;
			case DG::TEX_FORMAT_R32_UINT: 
				return 1;
			case DG::TEX_FORMAT_R32_SINT: 
				return 1;
			case DG::TEX_FORMAT_RG8_UNORM: 
				return 2;
			case DG::TEX_FORMAT_RG8_UINT: 
				return 2;
			case DG::TEX_FORMAT_RG8_SNORM: 
				return 2;
			case DG::TEX_FORMAT_RG8_SINT: 
				return 2;
			case DG::TEX_FORMAT_R16_FLOAT: 
				return 1;
			case DG::TEX_FORMAT_D16_UNORM: 
				return 1;
			case DG::TEX_FORMAT_R16_UNORM: 
				return 1;
			case DG::TEX_FORMAT_R16_UINT: 
				return 1;
			case DG::TEX_FORMAT_R16_SNORM: 
				return 1;
			case DG::TEX_FORMAT_R16_SINT: 
				return 1;
			case DG::TEX_FORMAT_R8_UNORM: 
				return 1;
			case DG::TEX_FORMAT_R8_UINT: 
				return 1;
			case DG::TEX_FORMAT_R8_SNORM: 
				return 1;
			case DG::TEX_FORMAT_R8_SINT: 
				return 1;
			case DG::TEX_FORMAT_A8_UNORM:	
				return 1;
			default:
				return -1;
		}
	}

	int GetSize(DG::VALUE_TYPE v) {
		switch (v) {
			case DG::VT_FLOAT32:
				return 4;
			case DG::VT_FLOAT16:
				return 2;
			case DG::VT_INT8:
				return 1;
			case DG::VT_INT16:
				return 2;
			case DG::VT_INT32:
				return 4;
			case DG::VT_UINT8:
				return 1;
			case DG::VT_UINT16:
				return 2;
			case DG::VT_UINT32:
				return 4;
			default:
				return -1;
		}
	}

	int GetPixelByteSize(DG::TEXTURE_FORMAT format) {
		auto count = GetComponentCount(format);
		if (count < 0)
			return -1;

		return GetTypeSize(GetComponentType(format)) * count;
	}

	bool GetIsNormalized(DG::TEXTURE_FORMAT format) {
		switch (format) {
			case DG::TEX_FORMAT_RGBA16_UNORM: 
				return true;
			case DG::TEX_FORMAT_RGBA16_SNORM: 
				return true;
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return true;
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return true;
			case DG::TEX_FORMAT_RGBA8_SNORM: 
				return true;
			case DG::TEX_FORMAT_RG16_UNORM: 
				return true;
			case DG::TEX_FORMAT_RG16_SNORM: 
				return true;
			case DG::TEX_FORMAT_RG8_UNORM: 
				return true;
			case DG::TEX_FORMAT_RG8_SNORM: 
				return true;
			case DG::TEX_FORMAT_D16_UNORM: 
				return true;
			case DG::TEX_FORMAT_R16_UNORM: 
				return true;
			case DG::TEX_FORMAT_R16_SNORM: 
				return true;
			case DG::TEX_FORMAT_R8_UNORM: 
				return true;
			case DG::TEX_FORMAT_R8_SNORM: 
				return true;
			case DG::TEX_FORMAT_A8_UNORM:	
				return true;
			default:
				return false;
		}
	}

	bool GetIsSRGB(DG::TEXTURE_FORMAT format) {
		switch (format) {
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}
}