//--------------------------------------------------------------------------------------
// File: PBREffect.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EffectCommon.h"

using namespace DirectX;


// Constant buffer layout. Must match the shader!
struct PBREffectConstants
{
    XMVECTOR eyePosition;
    XMMATRIX world;
    XMVECTOR worldInverseTranspose[3];
    XMMATRIX worldViewProj;
    XMMATRIX prevWorldViewProj; // for velocity generation

    XMVECTOR lightDirection[IEffectLights::MaxDirectionalLights];
    XMVECTOR lightDiffuseColor[IEffectLights::MaxDirectionalLights];

    // PBR Parameters
    XMVECTOR Albedo;
    float    Metallic;
    float    Roughness;
    int      numRadianceMipLevels;

    // Size of render target
    float   targetWidth;
    float   targetHeight;
};

static_assert((sizeof(PBREffectConstants) % 16) == 0, "CB size not padded correctly");


// Traits type describes our characteristics to the EffectBase template.
struct PBREffectTraits
{
    using ConstantBufferType = PBREffectConstants;

    static constexpr int VertexShaderCount = 6;
    static constexpr int PixelShaderCount = 5;
    static constexpr int ShaderPermutationCount = 16;
    static constexpr int RootSignatureCount = 1;
};


// Internal PBREffect implementation class.
class PBREffect::Impl : public EffectBase<PBREffectTraits>
{
public:
    Impl(_In_ ID3D11Device* device);

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedoTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rmaTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissiveTexture;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> radianceTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceTexture;

    bool biasedVertexNormals;
    bool velocityEnabled;
    bool instancing;

    XMVECTOR lightColor[MaxDirectionalLights];

    int GetCurrentShaderPermutation() const noexcept;

    void Apply(_In_ ID3D11DeviceContext* deviceContext);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOnePBREffect_VSConstant.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_VSConstantBn.inc"

    #include "Shaders/Compiled/XboxOnePBREffect_VSConstantInst.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_VSConstantBnInst.inc"

    #include "Shaders/Compiled/XboxOnePBREffect_VSConstantVelocity.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_VSConstantVelocityBn.inc"

    #include "Shaders/Compiled/XboxOnePBREffect_PSConstant.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_PSTextured.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_PSTexturedEmissive.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_PSTexturedVelocity.inc"
    #include "Shaders/Compiled/XboxOnePBREffect_PSTexturedEmissiveVelocity.inc"
#else
    #include "Shaders/Compiled/PBREffect_VSConstant.inc"
    #include "Shaders/Compiled/PBREffect_VSConstantBn.inc"

    #include "Shaders/Compiled/PBREffect_VSConstantInst.inc"
    #include "Shaders/Compiled/PBREffect_VSConstantBnInst.inc"

    #include "Shaders/Compiled/PBREffect_VSConstantVelocity.inc"
    #include "Shaders/Compiled/PBREffect_VSConstantVelocityBn.inc"

    #include "Shaders/Compiled/PBREffect_PSConstant.inc"
    #include "Shaders/Compiled/PBREffect_PSTextured.inc"
    #include "Shaders/Compiled/PBREffect_PSTexturedEmissive.inc"
    #include "Shaders/Compiled/PBREffect_PSTexturedVelocity.inc"
    #include "Shaders/Compiled/PBREffect_PSTexturedEmissiveVelocity.inc"
#endif
}


template<>
const ShaderBytecode EffectBase<PBREffectTraits>::VertexShaderBytecode[] =
{
    { PBREffect_VSConstant,           sizeof(PBREffect_VSConstant)           },
    { PBREffect_VSConstantVelocity,   sizeof(PBREffect_VSConstantVelocity)   },
    { PBREffect_VSConstantBn,         sizeof(PBREffect_VSConstantBn)         },
    { PBREffect_VSConstantVelocityBn, sizeof(PBREffect_VSConstantVelocityBn) },
    { PBREffect_VSConstantInst,       sizeof(PBREffect_VSConstantInst)       },
    { PBREffect_VSConstantBnInst,     sizeof(PBREffect_VSConstantBnInst)     },
};


template<>
const int EffectBase<PBREffectTraits>::VertexShaderIndices[] =
{
    0,      // constant
    0,      // textured
    0,      // textured + emissive
    1,      // textured + velocity
    1,      // textured + emissive + velocity
    4,      // instancing + constant
    4,      // instancing + textured
    4,      // instancing + textured + emissive

    2,      // constant (biased vertex normals)
    2,      // textured (biased vertex normals)
    2,      // textured + emissive (biased vertex normals)
    3,      // textured + velocity (biased vertex normals)
    3,      // textured + emissive + velocity (biasoed vertex normals)
    5,      // instancing + constant (biased vertex normals)
    5,      // instancing + textured (biased vertex normals)
    5,      // instancing + textured + emissive (biased vertex normals)
};


template<>
const ShaderBytecode EffectBase<PBREffectTraits>::PixelShaderBytecode[] =
{
    { PBREffect_PSConstant,                 sizeof(PBREffect_PSConstant)                 },
    { PBREffect_PSTextured,                 sizeof(PBREffect_PSTextured)                 },
    { PBREffect_PSTexturedEmissive,         sizeof(PBREffect_PSTexturedEmissive)         },
    { PBREffect_PSTexturedVelocity,         sizeof(PBREffect_PSTexturedVelocity)         },
    { PBREffect_PSTexturedEmissiveVelocity, sizeof(PBREffect_PSTexturedEmissiveVelocity) },
};


template<>
const int EffectBase<PBREffectTraits>::PixelShaderIndices[] =
{
    0,      // constant
    1,      // textured
    2,      // textured + emissive
    3,      // textured + velocity
    4,      // textured + emissive + velocity
    0,      // instancing + constant
    1,      // instancing + textured
    2,      // instancing + textured + emissive

    0,      // constant (biased vertex normals)
    1,      // textured (biased vertex normals)
    2,      // textured + emissive (biased vertex normals)
    3,      // textured + velocity (biased vertex normals)
    4,      // textured + emissive + velocity (biased vertex normals)
    0,      // instancing + constant (biased vertex normals)
    1,      // instancing + textured (biased vertex normals)
    2,      // instancing + textured + emissive (biased vertex normals)
};

// Global pool of per-device PBREffect resources. Required by EffectBase<>, but not used.
template<>
SharedResourcePool<ID3D11Device*, EffectBase<PBREffectTraits>::DeviceResources> EffectBase<PBREffectTraits>::deviceResourcesPool = {};

// Constructor.
PBREffect::Impl::Impl(_In_ ID3D11Device* device)
    : EffectBase(device),
    biasedVertexNormals(false),
    velocityEnabled(false),
    instancing(false),
    lightColor{}
{
    if (device->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
    {
        throw std::runtime_error("PBREffect requires Feature Level 10.0 or later");
    }

    static_assert(static_cast<int>(std::size(EffectBase<PBREffectTraits>::VertexShaderIndices)) == PBREffectTraits::ShaderPermutationCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<PBREffectTraits>::VertexShaderBytecode)) == PBREffectTraits::VertexShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<PBREffectTraits>::PixelShaderBytecode)) == PBREffectTraits::PixelShaderCount, "array/max mismatch");
    static_assert(static_cast<int>(std::size(EffectBase<PBREffectTraits>::PixelShaderIndices)) == PBREffectTraits::ShaderPermutationCount, "array/max mismatch");

    // Lighting
    static const XMVECTORF32 defaultLightDirection = { { { 0, -1, 0, 0 } } };
    for (int i = 0; i < MaxDirectionalLights; i++)
    {
        lightColor[i] = g_XMOne;
        constants.lightDirection[i] = defaultLightDirection;
        constants.lightDiffuseColor[i] = g_XMZero;
    }

    // Default PBR values
    constants.Albedo = g_XMOne;
    constants.Metallic = 0.5f;
    constants.Roughness = 0.2f;
    constants.numRadianceMipLevels = 1;
}


int PBREffect::Impl::GetCurrentShaderPermutation() const noexcept
{
    int permutation = 0;

    if (instancing)
    {
        // Vertex shader needs to use vertex matrix transform.
        permutation = (albedoTexture) ? 6 : 5;
    }
    else if (velocityEnabled)
    {
        // Optional velocity buffer (implies textured RMA)?
        permutation = 3;
    }
    else if (albedoTexture)
    {
        // Textured RMA vs. constant albedo/roughness/metalness?
        permutation = 1;
    }

    // Using an emissive texture?
    if (emissiveTexture)
    {
        permutation += 1;
    }

    if (biasedVertexNormals)
    {
        // Compressed normals need to be scaled and biased in the vertex shader.
        permutation += 8;
    }

    return permutation;
}


// Sets our state onto the D3D device.
void PBREffect::Impl::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    assert(deviceContext != nullptr);

    // Store old wvp for velocity calculation in shader
    constants.prevWorldViewProj = constants.worldViewProj;

    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);

    // World inverse transpose matrix.
    if (dirtyFlags & EffectDirtyFlags::WorldInverseTranspose)
    {
        constants.world = XMMatrixTranspose(matrices.world);

        XMMATRIX worldInverse = XMMatrixInverse(nullptr, matrices.world);

        constants.worldInverseTranspose[0] = worldInverse.r[0];
        constants.worldInverseTranspose[1] = worldInverse.r[1];
        constants.worldInverseTranspose[2] = worldInverse.r[2];

        dirtyFlags &= ~EffectDirtyFlags::WorldInverseTranspose;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }

    // Eye position vector.
    if (dirtyFlags & EffectDirtyFlags::EyePosition)
    {
        XMMATRIX viewInverse = XMMatrixInverse(nullptr, matrices.view);

        constants.eyePosition = viewInverse.r[3];

        dirtyFlags &= ~EffectDirtyFlags::EyePosition;
        dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
    }

    // Set the textures
    if (albedoTexture)
    {
        ID3D11ShaderResourceView* textures[6] =
        {
            albedoTexture.Get(), normalTexture.Get(), rmaTexture.Get(),
            emissiveTexture.Get(),
            radianceTexture.Get(), irradianceTexture.Get()
        };
        deviceContext->PSSetShaderResources(0, 6, textures);
    }
    else
    {
        ID3D11ShaderResourceView* textures[6] =
        {
            nullptr, nullptr, nullptr,
            nullptr,
            radianceTexture.Get(), irradianceTexture.Get()
        };
        deviceContext->PSSetShaderResources(0, 6, textures);
    }

    // Set shaders and constant buffers.
    ApplyShaders(deviceContext, GetCurrentShaderPermutation());
}

// Public constructor.
PBREffect::PBREffect(_In_ ID3D11Device* device)
    : pImpl(std::make_unique<Impl>(device))
{
}


// Move constructor.
PBREffect::PBREffect(PBREffect&& moveFrom) noexcept
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
PBREffect& PBREffect::operator= (PBREffect&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
PBREffect::~PBREffect()
{
}


// IEffect methods.
void PBREffect::Apply(_In_ ID3D11DeviceContext* deviceContext)
{
    pImpl->Apply(deviceContext);
}


void PBREffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
    pImpl->GetVertexShaderBytecode(pImpl->GetCurrentShaderPermutation(), pShaderByteCode, pByteCodeLength);
}


// Camera settings.
void XM_CALLCONV PBREffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose;
}


void XM_CALLCONV PBREffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition;
}


void XM_CALLCONV PBREffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV PBREffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition;
}


// Light settings
void PBREffect::SetLightingEnabled(bool value)
{
    if (!value)
    {
        throw std::invalid_argument("PBREffect does not support turning off lighting");
    }
}


void PBREffect::SetPerPixelLighting(bool)
{
    // Unsupported interface method.
}


void XM_CALLCONV PBREffect::SetAmbientLightColor(FXMVECTOR)
{
    // Unsupported interface.
}


void PBREffect::SetLightEnabled(int whichLight, bool value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDiffuseColor[whichLight] = (value) ? pImpl->lightColor[whichLight] : g_XMZero;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV PBREffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV PBREffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->lightColor[whichLight] = value;
    pImpl->constants.lightDiffuseColor[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV PBREffect::SetLightSpecularColor(int, FXMVECTOR)
{
    // Unsupported interface.
}


void PBREffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// PBR Settings
void PBREffect::SetAlpha(float value)
{
    // Set w to new value, but preserve existing xyz (constant albedo).
    pImpl->constants.Albedo = XMVectorSetW(pImpl->constants.Albedo, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void PBREffect::SetConstantAlbedo(FXMVECTOR value)
{
    // Set xyz to new value, but preserve existing w (alpha).
    pImpl->constants.Albedo = XMVectorSelect(pImpl->constants.Albedo, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void PBREffect::SetConstantMetallic(float value)
{
    pImpl->constants.Metallic = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void PBREffect::SetConstantRoughness(float value)
{
    pImpl->constants.Roughness = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture settings.
void PBREffect::SetAlbedoTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->albedoTexture = value;
}


void PBREffect::SetNormalTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->normalTexture = value;
}


void PBREffect::SetRMATexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->rmaTexture = value;
}

void PBREffect::SetEmissiveTexture(_In_opt_ ID3D11ShaderResourceView* value)
{
    pImpl->emissiveTexture = value;
}


void PBREffect::SetSurfaceTextures(
    _In_opt_ ID3D11ShaderResourceView* albedo,
    _In_opt_ ID3D11ShaderResourceView* normal,
    _In_opt_ ID3D11ShaderResourceView* roughnessMetallicAmbientOcclusion)
{
    pImpl->albedoTexture = albedo;
    pImpl->normalTexture = normal;
    pImpl->rmaTexture = roughnessMetallicAmbientOcclusion;
}


void PBREffect::SetIBLTextures(
    _In_opt_ ID3D11ShaderResourceView* radiance,
    int numRadianceMips,
    _In_opt_ ID3D11ShaderResourceView* irradiance)
{
    pImpl->radianceTexture = radiance;
    pImpl->irradianceTexture = irradiance;

    pImpl->constants.numRadianceMipLevels = numRadianceMips;
    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Normal compression settings.
void PBREffect::SetBiasedVertexNormals(bool value)
{
    pImpl->biasedVertexNormals = value;
}


// Instancing settings.
void PBREffect::SetInstancingEnabled(bool value)
{
    pImpl->instancing = value;
}


// Additional settings.
void PBREffect::SetVelocityGeneration(bool value)
{
    pImpl->velocityEnabled = value;
}


void PBREffect::SetRenderTargetSizeInPixels(int width, int height)
{
    pImpl->constants.targetWidth = static_cast<float>(width);
    pImpl->constants.targetHeight = static_cast<float>(height);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}
