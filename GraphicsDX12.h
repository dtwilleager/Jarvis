#pragma once
#pragma once
#include "Graphics.h"
#include "Mesh.h"
#include "Material.h"

#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include <sstream>
#include <set>
#include <array>
#include <stdio.h>
#include <fstream>
#include <cassert>
#include <map>

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d12SDKLayers.h>

#define _USE_MATH_DEFINES
#include <math.h>


using std::string;
using std::shared_ptr;
using std::vector;
using std::queue;
using std::set;
using std::array;
using std::ifstream;
using std::map;
using Microsoft::WRL::ComPtr;

namespace Jarvis
{
  class GraphicsDX12 :
    public Graphics
  {
  public:
    GraphicsDX12(string name, HINSTANCE hinstance, HWND window);
    ~GraphicsDX12();

    void                createDevice(uint32_t numFrames);

    void                createView(shared_ptr<View> view);
    void                resize(uint32_t width, uint32_t height);

    void                buildBuffers(vector<shared_ptr<Renderable>>& renderables);
    void                buildMaterial(shared_ptr<Material>);

    void                beginGBufferPass(shared_ptr<View> view);
    void                drawMesh(shared_ptr<View> view, shared_ptr<Mesh>, shared_ptr<Material>);
    void                compute(shared_ptr<View> view);
    void                trace(shared_ptr<View> view);
    void                endGBufferPass(shared_ptr<View> view);
    void                beginLightingPass(shared_ptr<View> view);
    void                endLightingPass(shared_ptr<View> view);
    void                beginFrame(shared_ptr<View> view);
    void                endFrame(shared_ptr<View> view); 
    void                executeCommands(shared_ptr<View> view);
    void                present(shared_ptr<View> view);

    void                updateFrameData(shared_ptr<View> view, vector<shared_ptr<Light>>& lights);
    void                updateMaterialData(shared_ptr<View> view);
    void                updateObjectData(shared_ptr<View> view, vector<shared_ptr<Renderable>>& renderables);

  private:
    HRESULT             createAdapter();
    void                enableDebugLayer();
    void                enableDeveloperMode();
    HRESULT             createCommandQueue();
    void                flushCommandQueue();
    HRESULT             createSwapchain(uint32_t numFrames);
    void                createSamplers();
    void                createRootSignatures();
    void                createInputLayouts();
    void                createTextureDescriptorHeaps();
    void                createPipelines();
    void                createShaders();


    ComPtr<ID3D12Resource>  createAndUploadBuffer(uint32_t size, void* data, D3D12_HEAP_TYPE heapFlags, ComPtr<ID3D12Resource>& uploadBuffer);
    void createConstantBuffers();
    ComPtr<ID3DBlob>    loadShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint,
      const std::string& target);

    void                printLog(string s);
    void                update(shared_ptr<View> view, shared_ptr<Material>);
    UINT                CalcConstantBufferByteSize(UINT byteSize);
    void                CopyData(void* dst, void* src, uint32_t size);
    DirectX::XMFLOAT4X4 ConvertTransform(mat4 matrix);

    static DirectX::XMFLOAT4X4 Identity4x4()
    {
      static DirectX::XMFLOAT4X4 I(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

      return I;
    }

    // Vertex Structures
    struct Vertex4
    {
      DirectX::XMFLOAT3 position;
      DirectX::XMFLOAT3 normal;
      DirectX::XMFLOAT2 texCoord;
      DirectX::XMFLOAT3 tangent;
    };

    struct Vertex3
    {
      DirectX::XMFLOAT3 position;
      DirectX::XMFLOAT3 normal;
      DirectX::XMFLOAT2 texCoord;
    };

    struct Vertex2
    {
      DirectX::XMFLOAT3 position;
      DirectX::XMFLOAT3 normal;
    };

    // Per mesh graphics data
    struct Dx12MeshData
    {
      size_t m_vertexStart;
      size_t m_numVertices;
      size_t m_indexStart;
      size_t m_numIndeces;
      uint32_t m_materialIndex;
      uint32_t m_meshIndex;

    };

    // Per texture graphics data
    struct Dx12TextureData
    {

    };

    struct Dx12Texture
    {
      ComPtr<ID3D12Resource>  m_texture = nullptr;
      ComPtr<ID3D12Resource>  m_textureUploadHeap = nullptr;
    };

    // Per material graphics data
    struct Dx12MaterialData
    {
      Dx12Texture             m_albedoTexture;
      Dx12Texture             m_normalTexture;
      Dx12Texture             m_roughnessTexture;
      int                     m_heapStartIndex = -1;
      int                     m_materialIndex = -1;
      ComPtr<ID3D12RootSignature> m_rootSignature;
      vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
      uint32_t                m_shaderIndex;
      ComPtr<ID3D12PipelineState> m_pso;
    };

    // Shader Structure
    struct Dx12ShaderData
    {
      ComPtr<ID3DBlob>        m_vsByteCode = nullptr;
      ComPtr<ID3DBlob>        m_psByteCode = nullptr;
    };

    // Per buffer graphics data
    struct Dx12ConstantBufferData
    {
    };

    // Per View graphics data
    struct Dx12ViewData
    {
      uint32_t m_currentWidth;
      uint32_t m_currentHeight;
      uint32_t m_frameIndex;
    };

    struct Dx12SwapchainBufferData
    {
      D3D12_RESOURCE_STATES   m_usageState;
      uint32_t                m_width;
      uint32_t                m_height;
      uint32_t                m_arraySize;
      DXGI_FORMAT             m_format;
      ComPtr<ID3D12Resource>  m_resource;
      D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
      ComPtr<ID3D12Resource>  m_depthResource;
      D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;

      ComPtr<ID3D12Resource>  m_albedoGbuffer;
      ComPtr<ID3D12Resource>  m_normalGbuffer;
      ComPtr<ID3D12Resource>  m_positionGbuffer;
      bool                    m_firstFrame = true;

    };

    struct MaterialConstants
    {
      DirectX::XMFLOAT4 m_diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
      DirectX::XMFLOAT3 m_fresnelR0 = { 0.01f, 0.01f, 0.01f };
      float             m_roughness = 64.0f;

      DirectX::XMFLOAT4X4 m_textureTransform = Identity4x4();
    };

    struct LightData
    {
      DirectX::XMFLOAT3 m_color = { 0.5f, 0.5f, 0.5f };
      float pad1;
      DirectX::XMFLOAT3 m_direction = { 0.0f, -1.0f, 0.0f };
      float pad2;
    };

#define MaxLights 64

    struct FrameConstants
    {
      DirectX::XMFLOAT4X4 m_view = Identity4x4();
      DirectX::XMFLOAT4X4 m_invView = Identity4x4();
      DirectX::XMFLOAT4X4 m_projection = Identity4x4();
      DirectX::XMFLOAT4X4 m_invProjection = Identity4x4();
      DirectX::XMFLOAT4X4 m_viewProjection = Identity4x4();
      DirectX::XMFLOAT4X4 m_invViewProjection = Identity4x4();
      DirectX::XMFLOAT4 m_ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
      DirectX::XMFLOAT3 m_eyePosition = { 0.0f, 0.0f, 0.0f };
      UINT m_numLights;
      LightData Lights[MaxLights];
      DirectX::XMFLOAT2 m_renderTargetSize = { 0.0f, 0.0f };
      DirectX::XMFLOAT2 m_invRenderTargetSize = { 0.0f, 0.0f };
      float m_near = 0.0f;
      float m_far = 0.0f;
    };

    struct ObjectConstants
    {
      DirectX::XMFLOAT4X4 m_worldTransform = Identity4x4();
    };

    struct ConstantBuffer
    {
      ComPtr<ID3D12Resource> m_buffer;
      BYTE* m_data = nullptr;
      UINT m_size = 0;
    };

    struct FrameData
    {
      ComPtr<ID3D12CommandAllocator>      m_graphicsAllocator;
      ConstantBuffer*                     m_materialData;
      ConstantBuffer*                     m_frameData;
      ConstantBuffer*                     m_objectData;
      uint64_t                            m_currentFence;
    };

    struct ScreenQuadVertex
    {
      DirectX::XMFLOAT4 position;
      DirectX::XMFLOAT2 texcoord;
      DirectX::XMFLOAT2 pad;
    };

    void loadTexture(shared_ptr<Texture> texture, Dx12Texture* textureData);

    uint32_t  m_frameIndex;
    HINSTANCE m_hinstance;
    HWND      m_window;
    uint32_t  m_numFrames;

    DXGI_FORMAT m_swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    ComPtr<IDXGIFactory4>   m_dxgiFactory;
    ComPtr<IDXGIAdapter1>   m_adapter;
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID3D12Device>    m_device;

    uint32_t                m_rtvDescriptorSize = 0;
    uint32_t                m_dsvDescriptorSize = 0;
    uint32_t                m_cbvSrvUavDescriptorSize;

    vector<Dx12SwapchainBufferData*>    m_swapchainBufferResources;
    vector<FrameData*>                  m_frameData;
    ComPtr<ID3D12DescriptorHeap>        m_swapchainBufferRTVHeap;
    ComPtr<ID3D12DescriptorHeap>        m_swapchainBufferDSVHeap;

    ComPtr<ID3D12CommandAllocator>      m_graphicsCommandAllocator;
    ComPtr<ID3D12CommandQueue>          m_graphicsCommandQueue;
    ComPtr<ID3D12GraphicsCommandList>   m_graphicsCommandList;
    ComPtr<ID3D12Fence>                 m_graphicsFence;
    uint64_t                            m_currentGraphicsFence;
    HANDLE                              m_graphicsFenceEventHandle;

    D3D12_VIEWPORT                      m_screenViewport;
    D3D12_RECT                          m_scissorRect;

    uint32_t                            m_vertexComps[3];

    ComPtr<ID3D12CommandAllocator>      m_resourceCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_resourceCommandList;

    ComPtr<ID3D12Resource>              m_vertexBuffer;
    ComPtr<ID3D12Resource>              m_indexBuffer;
    ComPtr<ID3D12Resource>              m_vertexBufferFSQ;

    map<std::wstring, ComPtr<ID3DBlob>>  m_vsMap;
    map<std::wstring, ComPtr<ID3DBlob>>  m_psMap;

    vector<shared_ptr<Material>>         m_materials;
    size_t                               m_numMeshes = 0;
    size_t                               m_numVerts = 0;
    size_t                               m_numIndeces = 0;
    uint32_t                             m_numTextures = 0;
    uint32_t                             m_meshIndex = 0;
    uint32_t                             m_materialIndex = 0;
    uint32_t                             m_heapIndex = 0;

    std::array<D3D12_STATIC_SAMPLER_DESC, 4>   m_samplers;
    std::array<ComPtr<ID3D12RootSignature>, 4> m_rootSignatures;
    std::array<vector<D3D12_INPUT_ELEMENT_DESC>, 4> m_inputLayouts;
    std::array<Dx12ShaderData, 4>        m_shaders;
    Dx12ShaderData                       m_lightPassShader;
    Dx12ShaderData                       m_lineShader;
    ComPtr<ID3D12RootSignature>          m_lightPassRootSignature;
    vector<D3D12_INPUT_ELEMENT_DESC>     m_inputLayoutLightPass;
    ComPtr<ID3D12DescriptorHeap>         m_srvHeapLightPass;
    ComPtr<ID3D12DescriptorHeap>         m_srvHeap;
    uint32_t                             m_cbvSrvDescriptorSize;

    std::array < ComPtr<ID3D12PipelineState>, 4> m_psos;
    ComPtr <ID3D12PipelineState>         m_lightPassPso;
    ComPtr <ID3D12PipelineState>         m_linePso;
  };
}


