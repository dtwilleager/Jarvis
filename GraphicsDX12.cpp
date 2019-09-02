#include "stdafx.h"
#include "GraphicsDX12.h"
#include "Light.h"
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <atlstr.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Jarvis
{
  GraphicsDX12::GraphicsDX12(string name, HINSTANCE hinstance, HWND window) : Graphics(name, hinstance, window),
    m_frameIndex(0),
    m_hinstance(hinstance),
    m_window(window),
    m_currentGraphicsFence(0)
  {
    m_vertexComps[0] = 0;
    m_vertexComps[1] = 0;
    m_vertexComps[2] = 0;
  }


  GraphicsDX12::~GraphicsDX12()
  {
  }

  void GraphicsDX12::createDevice(uint32_t numFrames)
  {
    enableDebugLayer();
    if (createAdapter() != S_OK)
    {
      return;
    }
    enableDeveloperMode();
    if (createCommandQueue() != S_OK)
    {
      return;
    }
    if (createSwapchain(numFrames) != S_OK)
    {
      return;
    }
    resize(m_width, m_height);
  }

  HRESULT GraphicsDX12::createAdapter()
  {
    HRESULT hr = E_FAIL;

    // Obtain the DXGI factory
    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory));
    if (!SUCCEEDED(hr))
    {
      printLog("Error: Unable to create DXGI Factory");
      return hr;
    }

    SIZE_T MaxSize = 0;
    Microsoft::WRL::ComPtr<ID3D12Device> device;

    for (uint32_t index = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(index, &m_adapter); ++index)
    {
      DXGI_ADAPTER_DESC1 desc;
      m_adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      {
        continue;
      }

      if (desc.DedicatedVideoMemory > MaxSize &&
        SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device))))
      {
        m_adapter->GetDesc1(&desc);
        printLog("D3D12 capable hardware found: " + std::string((char*)desc.Description) + "(" + std::to_string(desc.DedicatedVideoMemory) + "MB)");
        MaxSize = desc.DedicatedVideoMemory;
      }

      if (MaxSize > 0)
      {
        m_device = device.Detach();

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        hr = S_OK;
      }
      else
      {
        printLog("No D3D12 capable hardware found");
        hr = E_FAIL;
      }
    }
    return hr;
  }

  void GraphicsDX12::enableDebugLayer()
  {
#if _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
    {
      debugInterface->EnableDebugLayer();
    }
    else
    {
      printLog("WARNING: Unable to enable D3D12 debug validation layer");
    }
#endif
  }

  void GraphicsDX12::enableDeveloperMode()
  {
#ifndef RELEASE
    bool DeveloperModeEnabled = false;

    // Look in the Windows Registry to determine if Developer Mode is enabled
    HKEY hKey;
    LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
      DWORD keyValue, keySize = sizeof(DWORD);
      result = RegQueryValueEx(hKey, "AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
      if (result == ERROR_SUCCESS && keyValue == 1)
      {
        DeveloperModeEnabled = true;
      }
      RegCloseKey(hKey);
    }


    // Prevent the GPU from overclocking or underclocking to get consistent timings
    if (DeveloperModeEnabled)
    {
      m_device->SetStablePowerState(TRUE);
    }
#endif
  }

  HRESULT GraphicsDX12::createCommandQueue()
  {
    HRESULT hr = S_OK;
    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    QueueDesc.NodeMask = 1;
    hr = m_device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_graphicsCommandQueue));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create graphics command queue");
      return hr;
    }
    m_graphicsCommandQueue->SetName(L"Direct Graphics Command Queue");

    hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_graphicsFence));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create graphics fence");
      return hr;
    }
    m_graphicsFence->SetName(L"Direct Graphics Fence");
    m_graphicsFence->Signal((uint64_t)D3D12_COMMAND_LIST_TYPE_DIRECT << 56);

    m_graphicsFenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    if (m_graphicsFenceEventHandle == INVALID_HANDLE_VALUE)
    {
      printLog("ERROR: Could not create graphics fence");
      return E_FAIL;
    }

    hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_graphicsCommandAllocator));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create graphics allocator");
      return hr;
    }
    m_graphicsCommandAllocator->SetName(L"Direct Graphics Allocator");

    hr = m_device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,
      m_graphicsCommandAllocator.Get(),
      nullptr,
      IID_PPV_ARGS(m_graphicsCommandList.GetAddressOf()));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create graphics command list");
      return hr;
    }

    m_graphicsCommandList->Close();

    // Create a command list for resource uploads
    hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_resourceCommandAllocator));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create resource allocator");
      return hr;
    }
    m_resourceCommandAllocator->SetName(L"Direct Resource Allocator");

    hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
      m_resourceCommandAllocator.Get(),
      nullptr,
      IID_PPV_ARGS(m_resourceCommandList.GetAddressOf()));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create resource command list");
      return hr;
    }

    m_resourceCommandList->Close();

    return hr;
  }


  void GraphicsDX12::flushCommandQueue()
  {
    HRESULT hr = S_OK; 
    
    m_currentGraphicsFence++;
    hr = m_graphicsCommandQueue->Signal(m_graphicsFence.Get(), m_currentGraphicsFence);

    if (hr != S_OK)
    {
      printLog("WARNING: graphics command queue signal failed in flushCommandQueue");
      return;
    }

    // Wait until the GPU has completed commands up to this fence point.
    if (m_graphicsFence->GetCompletedValue() < m_currentGraphicsFence)
    {
      HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

      // Fire event when GPU hits current fence.  
      hr = m_graphicsFence->SetEventOnCompletion(m_currentGraphicsFence, eventHandle);
      if (hr != S_OK)
      {
        printLog("WARNING: graphics command queue set event failed in flushCommandQueue");
        CloseHandle(eventHandle);
        return;
      }

      // Wait until the GPU hits current fence event is fired.
      WaitForSingleObject(eventHandle, INFINITE);
      CloseHandle(eventHandle);
    }
  }

  HRESULT GraphicsDX12::createSwapchain(uint32_t numFrames)
  {
    HRESULT hr = S_OK;

    RECT rect;
    m_width = 800;
    m_height = 600;
    if (GetClientRect(m_window, &rect))
    {
      m_width = rect.right - rect.left;
      m_height = rect.bottom - rect.top;
    }

    m_numFrames = numFrames;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = m_swapChainFormat;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_numFrames;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    hr = m_dxgiFactory->CreateSwapChainForHwnd(m_graphicsCommandQueue.Get(), m_window, &swapChainDesc, nullptr, nullptr, &m_swapChain);
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create Swap Chain");
      return hr;
    }

    for (uint32_t i = 0; i < m_numFrames; ++i)
    {
      ComPtr<ID3D12Resource> displayPlane;
      hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&displayPlane));
      if (!SUCCEEDED(hr))
      {
        printLog("ERROR: Could not get Swap Chain Buffer");
        break;
      }

      Dx12SwapchainBufferData* bufferData = new Dx12SwapchainBufferData();
      D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();

      bufferData->m_resource.Attach(displayPlane.Get());
      bufferData->m_usageState = D3D12_RESOURCE_STATE_PRESENT;

      bufferData->m_width = (uint32_t)resourceDesc.Width;
      bufferData->m_height = resourceDesc.Height;
      bufferData->m_arraySize = resourceDesc.DepthOrArraySize;
      bufferData->m_format = resourceDesc.Format;
#ifndef RELEASE
      bufferData->m_resource->SetName(L"Primary SwapChain Buffer");
#endif

      displayPlane.Detach();
      m_swapchainBufferResources.push_back(bufferData);

      // Create FrameData
      FrameData* frameData = new FrameData();

      m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(frameData->m_graphicsAllocator.GetAddressOf()));
      frameData->m_currentFence = 0;
      m_frameData.push_back(frameData);
    }


    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = m_numFrames * (4); // 3 gbuffers and a color buffer
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_swapchainBufferRTVHeap));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create swap chain buffer render target view descriptor heap.");
      return hr;
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = m_numFrames;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_swapchainBufferDSVHeap));
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: Could not create swap chain depth stencil descriptor heap.");
      return hr;
    }

    return hr;
  }

  void GraphicsDX12::beginFrame(shared_ptr<View> view)
  {
    HRESULT hr = S_OK;

    //uint32_t currentFrame = frameIndex % m_numFrames;

    ComPtr<ID3D12CommandAllocator> graphicsAllocator = m_frameData[m_frameIndex]->m_graphicsAllocator;
    Dx12SwapchainBufferData* bufferData = m_swapchainBufferResources[m_frameIndex];

    if (m_frameData[m_frameIndex]->m_currentFence != 0 && m_graphicsFence->GetCompletedValue() < m_frameData[m_frameIndex]->m_currentFence)
    {
      HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
      hr = m_graphicsFence->SetEventOnCompletion(m_frameData[m_frameIndex]->m_currentFence, eventHandle);
      if (hr != S_OK)
      {
        printLog("WARNING: graphics fence set event failed in beginCommands");
        CloseHandle(eventHandle);
        return;
      }
      WaitForSingleObject(eventHandle, INFINITE);
      CloseHandle(eventHandle);
    }

    hr = graphicsAllocator->Reset();
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: command allocator reset failed in beginCommands.");
      return;
    }

    hr = m_graphicsCommandList->Reset(graphicsAllocator.Get(), nullptr);
  }

  void GraphicsDX12::beginGBufferPass(shared_ptr<View> view)
  {
    Dx12SwapchainBufferData* bufferData = m_swapchainBufferResources[m_frameIndex];

    m_graphicsCommandList->RSSetViewports(1, &m_screenViewport);
    m_graphicsCommandList->RSSetScissorRects(1, &m_scissorRect);

    if (bufferData->m_firstFrame)
    {
      m_graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_resource.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
      bufferData->m_firstFrame = false;
    }
    else
    {
      CD3DX12_RESOURCE_BARRIER renderTargetBarriers[4];
      renderTargetBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_albedoGbuffer.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
      renderTargetBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_normalGbuffer.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
      renderTargetBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_positionGbuffer.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
      renderTargetBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_resource.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

      // Indicate a state transition on the resource usage.
      m_graphicsCommandList->ResourceBarrier(4, renderTargetBarriers);
    }


    // Clear the back buffer and depth buffer.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
      m_swapchainBufferRTVHeap->GetCPUDescriptorHandleForHeapStart(),
      (m_frameIndex * 4)+1,
      m_rtvDescriptorSize);
    //m_graphicsCommandList->ClearRenderTargetView(rtvHeapHandle, DirectX::Colors::DarkSlateBlue, 0, nullptr);

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(
      m_swapchainBufferDSVHeap->GetCPUDescriptorHandleForHeapStart(),
      m_frameIndex,
      m_dsvDescriptorSize);
    m_graphicsCommandList->ClearDepthStencilView(dsvHeapHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    m_graphicsCommandList->OMSetRenderTargets(3, &rtvHeapHandle, true, &dsvHeapHandle);
  }


  void GraphicsDX12::drawMesh(shared_ptr<View> view, shared_ptr<Mesh> mesh, shared_ptr<Material> material)
  {
    Dx12MeshData* meshData = (Dx12MeshData*)mesh->getGraphicsData();
    Dx12MaterialData* materialData = (Dx12MaterialData*)material->getGraphicsData();

    if (mesh->getPrimitive() == Mesh::LINES)
    {
      m_graphicsCommandList->SetGraphicsRootSignature(m_rootSignatures[0].Get());
      m_graphicsCommandList->SetPipelineState(m_linePso.Get());
    }
    else
    {
      m_graphicsCommandList->SetGraphicsRootSignature(materialData->m_rootSignature.Get());
      m_graphicsCommandList->SetPipelineState(m_psos[materialData->m_shaderIndex].Get());
    }

    uint32_t objCBByteSize = m_frameData[m_frameIndex]->m_objectData->m_size;
    uint32_t matCBByteSize = m_frameData[m_frameIndex]->m_materialData->m_size;

    ComPtr<ID3D12Resource> objectCB = m_frameData[m_frameIndex]->m_objectData->m_buffer;
    ComPtr<ID3D12Resource> matCB = m_frameData[m_frameIndex]->m_materialData->m_buffer;
    ComPtr<ID3D12Resource> frameCB = m_frameData[m_frameIndex]->m_frameData->m_buffer;

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(Vertex4);
    vbv.SizeInBytes = (uint32_t)(m_numVerts * sizeof(Vertex4));
    m_graphicsCommandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R16_UINT;
    ibv.SizeInBytes = (uint32_t)(m_numIndeces * sizeof(uint16_t));
    m_graphicsCommandList->IASetIndexBuffer(&ibv);

    if (mesh->getPrimitive() == Mesh::LINES)
    {
      m_graphicsCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

      ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
      m_graphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

      D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + meshData->m_meshIndex*objCBByteSize;
      D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress();

      m_graphicsCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);
      m_graphicsCommandList->SetGraphicsRootConstantBufferView(1, matCBAddress);
      m_graphicsCommandList->SetGraphicsRootConstantBufferView(2, frameCB->GetGPUVirtualAddress());

    }
    else
    {
      m_graphicsCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
      m_graphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

      CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
      tex.Offset(materialData->m_heapStartIndex, m_cbvSrvDescriptorSize);

      D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + meshData->m_meshIndex*objCBByteSize;
      D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialData->m_materialIndex*matCBByteSize;

      if (materialData->m_shaderIndex == 0)
      {
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(1, matCBAddress);
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(2, frameCB->GetGPUVirtualAddress());
      }
      else
      {
        m_graphicsCommandList->SetGraphicsRootDescriptorTable(0, tex);
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(2, matCBAddress);
        m_graphicsCommandList->SetGraphicsRootConstantBufferView(3, frameCB->GetGPUVirtualAddress());
      }
    }




    m_graphicsCommandList->DrawIndexedInstanced((uint32_t)meshData->m_numIndeces, 1, (uint32_t)meshData->m_indexStart, (int)meshData->m_vertexStart, 0);
  }

  void GraphicsDX12::compute(shared_ptr<View> view)
  {
  }

  void GraphicsDX12::trace(shared_ptr<View> view)
  {
  }

  void GraphicsDX12::endGBufferPass(shared_ptr<View> view)
  {
    HRESULT hr = S_OK;
    //uint32_t currentFrame = frameIndex % m_numFrames;
    Dx12SwapchainBufferData* bufferData = m_swapchainBufferResources[m_frameIndex];

    CD3DX12_RESOURCE_BARRIER renderTargetBarriers[3];
    renderTargetBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_albedoGbuffer.Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    renderTargetBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_normalGbuffer.Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    renderTargetBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_positionGbuffer.Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Indicate a state transition on the resource usage.
    m_graphicsCommandList->ResourceBarrier(3, renderTargetBarriers);
  }

  void GraphicsDX12::beginLightingPass(shared_ptr<View> view)
  {
    // Clear the back buffer and depth buffer.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
      m_swapchainBufferRTVHeap->GetCPUDescriptorHandleForHeapStart(),
      m_frameIndex * 4,
      m_rtvDescriptorSize);
    m_graphicsCommandList->ClearRenderTargetView(rtvHeapHandle, DirectX::Colors::DarkSlateBlue, 0, nullptr);

    m_graphicsCommandList->OMSetRenderTargets(1, &rtvHeapHandle, true, nullptr);
  }

  void GraphicsDX12::endLightingPass(shared_ptr<View> view)
  {
    m_graphicsCommandList->SetGraphicsRootSignature(m_lightPassRootSignature.Get());
    m_graphicsCommandList->SetPipelineState(m_lightPassPso.Get());

    uint32_t objCBByteSize = m_frameData[m_frameIndex]->m_objectData->m_size;
    uint32_t matCBByteSize = m_frameData[m_frameIndex]->m_materialData->m_size;

    ComPtr<ID3D12Resource> objectCB = m_frameData[m_frameIndex]->m_objectData->m_buffer;
    ComPtr<ID3D12Resource> matCB = m_frameData[m_frameIndex]->m_materialData->m_buffer;
    ComPtr<ID3D12Resource> frameCB = m_frameData[m_frameIndex]->m_frameData->m_buffer;

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = m_vertexBufferFSQ->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(ScreenQuadVertex);
    vbv.SizeInBytes = (uint32_t)(4 * sizeof(ScreenQuadVertex));
    m_graphicsCommandList->IASetVertexBuffers(0, 1, &vbv);

    m_graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeapLightPass.Get() };
    m_graphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_srvHeapLightPass->GetGPUDescriptorHandleForHeapStart());
    tex.Offset(m_frameIndex*3, m_cbvSrvDescriptorSize);

    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress();

    m_graphicsCommandList->SetGraphicsRootDescriptorTable(0, tex);
    m_graphicsCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);
    m_graphicsCommandList->SetGraphicsRootConstantBufferView(2, matCBAddress);
    m_graphicsCommandList->SetGraphicsRootConstantBufferView(3, frameCB->GetGPUVirtualAddress());

    m_graphicsCommandList->DrawInstanced(4, 1, 0, 0);
  }

  void GraphicsDX12::endFrame(shared_ptr<View> view)
  {
    HRESULT hr = S_OK;
    //uint32_t currentFrame = frameIndex % m_numFrames;
    Dx12SwapchainBufferData* bufferData = m_swapchainBufferResources[m_frameIndex];

    // Indicate a state transition on the resource usage.
    m_graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferData->m_resource.Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    hr = m_graphicsCommandList->Close();
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: command list close failed in endCommands.");
      return;
    }
  }

  void GraphicsDX12::executeCommands(shared_ptr<View> view)
  {
    ID3D12CommandList* cmdsLists[] = { m_graphicsCommandList.Get() };
    m_graphicsCommandQueue->ExecuteCommandLists(1, cmdsLists);
  }

  void GraphicsDX12::update(shared_ptr<View> view, shared_ptr<Material> material)
  {
    Dx12MaterialData* materialData = (Dx12MaterialData*)material->getGraphicsData();
  }

  void GraphicsDX12::present(shared_ptr<View> view)
  {
    HRESULT hr = S_OK;
    //uint32_t currentFrame = frameIndex % m_numFrames;

    // Swap the back and front buffers
    hr = m_swapChain->Present(0, 0);
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: present failed.");
      return;
    }
    m_frameData[m_frameIndex]->m_currentFence = ++m_currentGraphicsFence;
    m_graphicsCommandQueue->Signal(m_graphicsFence.Get(), m_currentGraphicsFence);

    m_frameIndex++;
    if (m_frameIndex == m_numFrames)
    {
      m_frameIndex = 0;
    }
  }

  void GraphicsDX12::buildBuffers(vector<shared_ptr<Renderable>>& renderables)
  {
    HRESULT hr = S_OK;
    m_numMeshes = 0;
    m_numVerts = 0;
    m_numIndeces = 0;

    for (size_t i = 0; i < renderables.size(); ++i)
    {
      for (size_t j = 0; j < renderables[i]->numGeometries(); ++j)
      {
        shared_ptr<Geometry> geometry = renderables[i]->getGeometry(j);
        if (geometry->getGeometryType() == Geometry::GEOMETRY_MESH)
        {
          shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(geometry);
          m_numMeshes++;
          m_numVerts += mesh->getNumVerts();
          m_numIndeces += mesh->getIndexBufferSize();

          shared_ptr<Material> material = mesh->getMaterial();
          int materialIndex = 0;
          for (; materialIndex < m_materials.size(); materialIndex++)
          {
            if (material == m_materials[materialIndex])
            {
              break;
            }
          }
          if (materialIndex == m_materials.size())
          {
            m_materials.emplace_back(material);
            if (material->getAlbedoTexture() != nullptr)
            {
              m_numTextures++;
            }

            shared_ptr<Texture> normal = material->getNormalTexture();
            if (material->getNormalTexture() != nullptr)
            {
              m_numTextures++;
            }

            shared_ptr<Texture> roughness = material->getMetallicRoughnessTexture();
            if (material->getMetallicRoughnessTexture() != nullptr)
            {
              m_numTextures++;
            }
          }
        }
      }
    }


    void* vertexData = malloc(m_numVerts * sizeof(Vertex4));
    uint16_t* indexData = (uint16_t*)malloc(m_numIndeces * sizeof(uint16_t));

    float* meshVertexData[5];
    uint32_t* meshIndexData;

    Vertex4* v4ptr = (Vertex4*)vertexData;
    uint16_t* iptr = indexData;
    size_t vertexStart = 0;
    size_t indexStart = 0;

    for (size_t i = 0; i < renderables.size(); ++i)
    {
      for (size_t j = 0; j < renderables[i]->numGeometries(); ++j)
      {
        shared_ptr<Geometry> geometry = renderables[i]->getGeometry(j);
        if (geometry->getGeometryType() == Geometry::GEOMETRY_MESH)
        {
          shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(geometry);
          Dx12MeshData* meshData = new Dx12MeshData();

          size_t numBuffers = mesh->getNumBuffers();
          for (size_t k = 0; k < numBuffers; ++k)
          {
            meshVertexData[k] = mesh->getVertexBufferData(k);
          }
          meshIndexData = mesh->getIndexBuffer();

          size_t numVerts = mesh->getNumVerts();
          for (size_t k = 0; k < numVerts; ++k)
          {
            v4ptr->position.x = meshVertexData[0][k * 3 + 0];
            v4ptr->position.y = meshVertexData[0][k * 3 + 1];
            v4ptr->position.z = meshVertexData[0][k * 3 + 2];
            v4ptr->normal.x = meshVertexData[1][k * 3 + 0];
            v4ptr->normal.y = meshVertexData[1][k * 3 + 1];
            v4ptr->normal.z = meshVertexData[1][k * 3 + 2];
            if (numBuffers > 2)
            {
              v4ptr->texCoord.x = meshVertexData[2][k * 2 + 0];
              v4ptr->texCoord.y = meshVertexData[2][k * 2 + 1];
              v4ptr->tangent.x = meshVertexData[3][k * 3 + 0];
              v4ptr->tangent.y = meshVertexData[3][k * 3 + 1];
              v4ptr->tangent.z = meshVertexData[3][k * 3 + 2];
            }
            v4ptr++;
          }

          size_t numIndeces = mesh->getIndexBufferSize();
          for (size_t k = 0; k < numIndeces; ++k)
          {
            *iptr = (uint16_t)meshIndexData[k];
            iptr++;
          }

          meshData->m_numIndeces = numIndeces;
          meshData->m_numVertices = numVerts;
          meshData->m_vertexStart = vertexStart;
          meshData->m_indexStart = indexStart;
          meshData->m_meshIndex = m_meshIndex++;
          mesh->setGraphicsData(meshData);
          mesh->setDirty(false);

          vertexStart += numVerts;
          indexStart += numIndeces;
        }
      }
    }

    createSamplers();
    createRootSignatures();
    createInputLayouts();
    createTextureDescriptorHeaps();
    createShaders();
    createPipelines();

    for (int i = 0; i < m_materials.size(); ++i)
    {
      buildMaterial(m_materials[i]);
    }


    hr = m_resourceCommandList->Reset(m_resourceCommandAllocator.Get(), nullptr);
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: unable to reset resource command list.");
      return;
    }

    ComPtr<ID3D12Resource> vUploadBuffer;
    ComPtr<ID3D12Resource> iUploadBuffer;

    m_vertexBuffer = createAndUploadBuffer((uint32_t)(m_numVerts * sizeof(Vertex4)), vertexData, D3D12_HEAP_TYPE_DEFAULT, vUploadBuffer);
    m_indexBuffer = createAndUploadBuffer((uint32_t)(m_numIndeces * sizeof(uint16_t)), indexData, D3D12_HEAP_TYPE_DEFAULT, iUploadBuffer);

    // Create the full screen quad
    ScreenQuadVertex QuadVerts[] =
    {
      { { -1.0f,1.0f, 0.0f,1.0f },{ 0.0f,0.0f } },
      { { 1.0f, 1.0f, 0.0f,1.0f }, {1.0f,0.0f } },
      { { -1.0f, -1.0f, 0.0f,1.0f },{ 0.0f,1.0f } },
    { { 1.0f, -1.0f, 0.0f,1.0f },{ 1.0f,1.0f } }
    };

    ComPtr<ID3D12Resource> vfsqUploadBuffer;
    m_vertexBufferFSQ = createAndUploadBuffer((uint32_t)(4 * sizeof(ScreenQuadVertex)), QuadVerts, D3D12_HEAP_TYPE_DEFAULT, vfsqUploadBuffer);

    m_resourceCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { m_resourceCommandList.Get() };
    m_graphicsCommandQueue->ExecuteCommandLists(1, cmdsLists);
    flushCommandQueue();

    createConstantBuffers();
  }

  void GraphicsDX12::createSamplers()
  {
    D3D12_STATIC_SAMPLER_DESC pointClamp = {};
    pointClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pointClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClamp.MipLODBias = 0;
    pointClamp.MaxAnisotropy = 0;
    pointClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    pointClamp.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    pointClamp.MinLOD = 0.0f;
    pointClamp.MaxLOD = D3D12_FLOAT32_MAX;
    pointClamp.ShaderRegister = 0;
    pointClamp.RegisterSpace = 0;
    pointClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC pointWrap = {};
    pointWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pointWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrap.MipLODBias = 0;
    pointWrap.MaxAnisotropy = 0;
    pointWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    pointWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    pointWrap.MinLOD = 0.0f;
    pointWrap.MaxLOD = D3D12_FLOAT32_MAX;
    pointWrap.ShaderRegister = 1;
    pointWrap.RegisterSpace = 0;
    pointWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC linearClamp = {};
    linearClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClamp.MipLODBias = 0;
    linearClamp.MaxAnisotropy = 0;
    linearClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    linearClamp.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearClamp.MinLOD = 0.0f;
    linearClamp.MaxLOD = D3D12_FLOAT32_MAX;
    linearClamp.ShaderRegister = 2;
    linearClamp.RegisterSpace = 0;
    linearClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC linearWrap = {};
    linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.MipLODBias = 0;
    linearWrap.MaxAnisotropy = 0;
    linearWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    linearWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearWrap.MinLOD = 0.0f;
    linearWrap.MaxLOD = D3D12_FLOAT32_MAX;
    linearWrap.ShaderRegister = 3;
    linearWrap.RegisterSpace = 0;
    linearWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    m_samplers[0] = pointClamp;
    m_samplers[1] = pointWrap;
    m_samplers[2] = linearClamp;
    m_samplers[3] = linearWrap;
  }

  void GraphicsDX12::createRootSignatures()
  {
    HRESULT hr = S_OK;
    // Root parameter for no texture signature
    CD3DX12_ROOT_PARAMETER slotRootParameterNoTex[3];

    // Create root CBV.
    slotRootParameterNoTex[0].InitAsConstantBufferView(0);
    slotRootParameterNoTex[1].InitAsConstantBufferView(1);
    slotRootParameterNoTex[2].InitAsConstantBufferView(2);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDescNoTex(3, slotRootParameterNoTex, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSigNoTex = nullptr;
    ComPtr<ID3DBlob> errorBlobNoTex = nullptr;
    hr = D3D12SerializeRootSignature(&rootSigDescNoTex, D3D_ROOT_SIGNATURE_VERSION_1,
      serializedRootSigNoTex.GetAddressOf(), errorBlobNoTex.GetAddressOf());

    if (errorBlobNoTex != nullptr)
    {
      ::OutputDebugStringA((char*)errorBlobNoTex->GetBufferPointer());
    }

    hr = m_device->CreateRootSignature(
      0,
      serializedRootSigNoTex->GetBufferPointer(),
      serializedRootSigNoTex->GetBufferSize(),
      IID_PPV_ARGS(m_rootSignatures[0].GetAddressOf()));

    for (uint32_t i = 1; i < 4; ++i)
    {
      CD3DX12_DESCRIPTOR_RANGE texTableLit;
      texTableLit.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        i,  // number of descriptors
        0); // register t0

      // Root parameter can be a table, root descriptor or root constants.
      CD3DX12_ROOT_PARAMETER slotRootParameter[4];

      // Create root CBV.
      slotRootParameter[0].InitAsDescriptorTable(1, &texTableLit, D3D12_SHADER_VISIBILITY_PIXEL);
      slotRootParameter[1].InitAsConstantBufferView(0);
      slotRootParameter[2].InitAsConstantBufferView(1);
      slotRootParameter[3].InitAsConstantBufferView(2);

      // A root signature is an array of root parameters.
      CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, 4, m_samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

      // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
      ComPtr<ID3DBlob> serializedRootSig = nullptr;
      ComPtr<ID3DBlob> errorBlob = nullptr;
      HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

      if (errorBlob != nullptr)
      {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
      }

      hr = m_device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignatures[i].GetAddressOf()));
    }

    // Create the Light Pass Root Signature
    CD3DX12_DESCRIPTOR_RANGE texTableLightPass;
    texTableLightPass.Init(
      D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
      3,  // number of descriptors
      0); // register t0

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    // Create root CBV.
    slotRootParameter[0].InitAsDescriptorTable(1, &texTableLightPass, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, 4, m_samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
      serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
      ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }

    hr = m_device->CreateRootSignature(
      0,
      serializedRootSig->GetBufferPointer(),
      serializedRootSig->GetBufferSize(),
      IID_PPV_ARGS(m_lightPassRootSignature.GetAddressOf()));
  }

  void GraphicsDX12::createInputLayouts()
  {
    m_inputLayouts[0] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    m_inputLayouts[1] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    m_inputLayouts[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    m_inputLayouts[3] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    m_inputLayoutLightPass =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

  }

  void GraphicsDX12::createTextureDescriptorHeaps()
  {
    HRESULT hr = S_OK;
    //
    // Create the SRV heap.
    //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = m_numTextures;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));


  }

  void GraphicsDX12::createShaders()
  {
    std::wstring vsName = L"Shaders\\GBufferNoTexture.hlsl";
    std::wstring psName = L"Shaders\\GBufferNoTexture.hlsl";
    m_shaders[0].m_vsByteCode = loadShader(vsName, nullptr, "VS", "vs_5_0");
    m_shaders[0].m_psByteCode = loadShader(psName, nullptr, "PS", "ps_5_0");

    vsName = L"Shaders\\GBufferAlbedoMap.hlsl";
    psName = L"Shaders\\GBufferAlbedoMap.hlsl";
    m_shaders[1].m_vsByteCode = loadShader(vsName, nullptr, "VS", "vs_5_0");
    m_shaders[1].m_psByteCode = loadShader(psName, nullptr, "PS", "ps_5_0");

    vsName = L"Shaders\\GBufferAlbedoNormalMap.hlsl";
    psName = L"Shaders\\GBufferAlbedoNormalMap.hlsl";
    m_shaders[2].m_vsByteCode = loadShader(vsName, nullptr, "VS", "vs_5_0");
    m_shaders[2].m_psByteCode = loadShader(psName, nullptr, "PS", "ps_5_0");

    vsName = L"Shaders\\GBufferAlbedoNormalRoughnessMap.hlsl";
    psName = L"Shaders\\GBufferAlbedoNormalRoughnessMap.hlsl";
    m_shaders[3].m_vsByteCode = loadShader(vsName, nullptr, "VS", "vs_5_0");
    m_shaders[3].m_psByteCode = loadShader(psName, nullptr, "PS", "ps_5_0");

    vsName = L"Shaders\\LightPass.hlsl";
    psName = L"Shaders\\LightPass.hlsl";
    m_lightPassShader.m_vsByteCode = loadShader(vsName, nullptr, "VS", "vs_5_0");
    m_lightPassShader.m_psByteCode = loadShader(psName, nullptr, "PS", "ps_5_0");

    vsName = L"Shaders\\Line.hlsl";
    psName = L"Shaders\\Line.hlsl";
    m_lineShader.m_vsByteCode = loadShader(vsName, nullptr, "VS", "vs_5_0");
    m_lineShader.m_psByteCode = loadShader(psName, nullptr, "PS", "ps_5_0");
  }

  void GraphicsDX12::createPipelines()
  {
    HRESULT hr = S_OK;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;

    //
    // PSO for each type of Material
    //
    for (uint32_t i = 0; i < 4; i++)
    {
      ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
      psoDesc.InputLayout = { m_inputLayouts[i].data(), (UINT)m_inputLayouts[i].size() };
      psoDesc.pRootSignature = m_rootSignatures[i].Get();
      psoDesc.VS =
      {
        reinterpret_cast<BYTE*>(m_shaders[i].m_vsByteCode->GetBufferPointer()),
        m_shaders[i].m_vsByteCode->GetBufferSize()
      };
      psoDesc.PS =
      {
        reinterpret_cast<BYTE*>(m_shaders[i].m_psByteCode->GetBufferPointer()),
        m_shaders[i].m_psByteCode->GetBufferSize()
      };

      CD3DX12_RASTERIZER_DESC rdesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
      rdesc.CullMode = D3D12_CULL_MODE_NONE;
      psoDesc.RasterizerState = rdesc;
      psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
      psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
      psoDesc.SampleMask = UINT_MAX;
      psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      psoDesc.NumRenderTargets = 3;
      psoDesc.RTVFormats[0] = DXGI_FORMAT_R11G11B10_FLOAT;
      psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
      psoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
      psoDesc.SampleDesc.Count = 1;
      psoDesc.SampleDesc.Quality = 0;
      psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
      hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_psos[i]));
    }

    // Create the light pass pso
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { m_inputLayouts[0].data(), (UINT)m_inputLayouts[0].size() };
    psoDesc.pRootSignature = m_rootSignatures[0].Get();
    psoDesc.VS =
    {
      reinterpret_cast<BYTE*>(m_lineShader.m_vsByteCode->GetBufferPointer()),
      m_lineShader.m_vsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
      reinterpret_cast<BYTE*>(m_lineShader.m_psByteCode->GetBufferPointer()),
      m_lineShader.m_psByteCode->GetBufferSize()
    };

    CD3DX12_RASTERIZER_DESC rdesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rdesc.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState = rdesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    psoDesc.NumRenderTargets = 3;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R11G11B10_FLOAT;
    psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_linePso));

    // Create the light pass pso
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { m_inputLayoutLightPass.data(), (UINT)m_inputLayoutLightPass.size() };
    psoDesc.pRootSignature = m_lightPassRootSignature.Get();
    psoDesc.VS =
    {
      reinterpret_cast<BYTE*>(m_lightPassShader.m_vsByteCode->GetBufferPointer()),
      m_lightPassShader.m_vsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
      reinterpret_cast<BYTE*>(m_lightPassShader.m_psByteCode->GetBufferPointer()),
      m_lightPassShader.m_psByteCode->GetBufferSize()
    };

    rdesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rdesc.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState = rdesc;
    psoDesc.RasterizerState.DepthClipEnable = false;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_swapChainFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_lightPassPso));
  }

  void GraphicsDX12::createConstantBuffers()
  {
    for (uint32_t i = 0; i < m_numFrames; ++i)
    {
      // Material Data
      m_frameData[i]->m_materialData = new ConstantBuffer();
      m_frameData[i]->m_materialData->m_size = CalcConstantBufferByteSize(sizeof(MaterialConstants));

      m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(m_frameData[i]->m_materialData->m_size*m_materials.size()),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_frameData[i]->m_materialData->m_buffer));

      m_frameData[i]->m_materialData->m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_frameData[i]->m_materialData->m_data));

      // Frame Data
      m_frameData[i]->m_frameData = new ConstantBuffer();
      m_frameData[i]->m_frameData->m_size = CalcConstantBufferByteSize(sizeof(FrameConstants));

      m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(m_frameData[i]->m_frameData->m_size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_frameData[i]->m_frameData->m_buffer));

      m_frameData[i]->m_frameData->m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_frameData[i]->m_frameData->m_data));

      // Object Data
      m_frameData[i]->m_objectData = new ConstantBuffer();
      m_frameData[i]->m_objectData->m_size = CalcConstantBufferByteSize(sizeof(ObjectConstants));

      m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(m_frameData[i]->m_objectData->m_size*m_numMeshes),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_frameData[i]->m_objectData->m_buffer));

      m_frameData[i]->m_objectData->m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_frameData[i]->m_objectData->m_data));
    }
  }

  ComPtr<ID3D12Resource> GraphicsDX12::createAndUploadBuffer(uint32_t size, void* data, D3D12_HEAP_TYPE heapFlags, ComPtr<ID3D12Resource>& uploadBuffer)
  {
    HRESULT hr = S_OK;
    ComPtr<ID3D12Resource> defaultBuffer;

    // Create the actual default buffer resource.
    hr = m_device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(heapFlags),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(size),
      D3D12_RESOURCE_STATE_COMMON,
      nullptr,
      IID_PPV_ARGS(defaultBuffer.GetAddressOf()));

    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: unable to create committed resource.");
      return nullptr;
    }

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    hr = m_device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(size),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: unable to create committed resource for upload.");
      return nullptr;
    }

    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = data;
    subResourceData.RowPitch = size;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    m_resourceCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

    UpdateSubresources(m_resourceCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    m_resourceCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
      D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    return defaultBuffer;
  }

  void GraphicsDX12::updateFrameData(shared_ptr<View> view, vector<shared_ptr<Light>>& lights)
  {
    FrameConstants currentFrameConstants;

    vec3 eye = view->getEye();
    vec3 target = view->getTarget();
    vec3 up = view->getUp();

    DirectX::XMVECTOR pos = DirectX::XMVectorSet(eye.x, eye.y, eye.z, 1.0f);
    DirectX::XMVECTOR targetp = DirectX::XMVectorSet(target.x, target.y, target.z, 1.0f);
    DirectX::XMVECTOR upv = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);

    DirectX::XMMATRIX viewm = DirectX::XMMatrixLookAtLH(pos, targetp, upv);
    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH((float)M_PI / 2.0f, 1.0f, 0.1f, 10000.0f);
    DirectX::XMFLOAT4X4 projection;

    DirectX::XMStoreFloat4x4(&projection, P);
    DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&projection);

    DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(viewm, proj);
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&XMMatrixDeterminant(viewm), viewm);
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&currentFrameConstants.m_view, XMMatrixTranspose(viewm));
    XMStoreFloat4x4(&currentFrameConstants.m_invView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&currentFrameConstants.m_projection, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&currentFrameConstants.m_invProjection, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&currentFrameConstants.m_viewProjection, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&currentFrameConstants.m_invViewProjection, XMMatrixTranspose(invViewProj));

    currentFrameConstants.m_eyePosition = { viewm.r->m128_f32[12], viewm.r->m128_f32[13], viewm.r->m128_f32[14] };
    currentFrameConstants.m_renderTargetSize = { (float)1200, (float)800 };
    currentFrameConstants.m_invRenderTargetSize = { 1.0f / (float)1200, 1.0f / (float)800 };
    currentFrameConstants.m_near = 0.01f;
    currentFrameConstants.m_far = 10000.0f;
    currentFrameConstants.m_ambientLight = { 0.1f, 0.1f, 0.1f, 1.0f };

    currentFrameConstants.m_numLights = (uint32_t)(lights.size() > 16 ? 16: lights.size());
    for (uint32_t i = 0; i < lights.size(); ++i)
    {
      if (i == 64)
      {
        break;
      }
      vec3 diffuse;
      vec3 position;
      lights[i]->getDiffuse(diffuse);
      lights[i]->getPosition(position);
      currentFrameConstants.Lights[i].m_color = { diffuse.r, diffuse.g, diffuse.b};
      currentFrameConstants.Lights[i].m_direction = { position.x, position.y, position.z };
    }

    void* dst = m_frameData[m_frameIndex]->m_frameData->m_data;
    CopyData(dst, &currentFrameConstants, sizeof(currentFrameConstants));
  }

  void GraphicsDX12::updateMaterialData(shared_ptr<View> view)
  {

    //frameIndex = frameIndex % m_numFrames;

    for (int i=0; i< m_materials.size(); ++i)
    {
      uint32_t dirty = m_materials[i]->getDirty();
      if (dirty > 0)
      {
        vec4 color;
        shared_ptr<Material> material = m_materials[i];
        MaterialConstants matConstants;
        material->getAlbedoColor(color);
        matConstants.m_diffuseAlbedo.x = color.r;
        matConstants.m_diffuseAlbedo.y = color.g;
        matConstants.m_diffuseAlbedo.z = color.b;
        matConstants.m_diffuseAlbedo.w = color.a;

        matConstants.m_fresnelR0.x = 0.01f;
        matConstants.m_fresnelR0.y = 0.01f;
        matConstants.m_fresnelR0.z = 0.01f;
        matConstants.m_roughness = material->getRoughness();
        matConstants.m_textureTransform = Identity4x4();

        void* dst = m_frameData[m_frameIndex]->m_materialData->m_data + i * m_frameData[m_frameIndex]->m_materialData->m_size;
        CopyData(dst, &matConstants, sizeof(matConstants));

        // Next FrameResource need to be updated too.
        material->setDirty(dirty--);
      }
    }
  }

  void GraphicsDX12::updateObjectData(shared_ptr<View> view, vector<shared_ptr<Renderable>>& renderables)
  {
    //frameIndex = frameIndex % m_numFrames;
    uint32_t meshIndex = 0;

    for (uint32_t i = 0; i < renderables.size(); ++i)
    {
      shared_ptr<Entity> entity = renderables[i]->getEntity(0);
      glm::mat4 transform;
      entity->getCompositeTransform(transform);
      ObjectConstants objConstants;
      objConstants.m_worldTransform = ConvertTransform(transform);

      for (uint32_t j = 0; j < renderables[i]->numGeometries(); ++j)
      {
        shared_ptr<Geometry> geometry = renderables[i]->getGeometry(j);
        if (geometry->getGeometryType() == Geometry::GEOMETRY_MESH)
        {
          void* dst = m_frameData[m_frameIndex]->m_objectData->m_data + meshIndex * m_frameData[m_frameIndex]->m_objectData->m_size;
          CopyData(dst, &objConstants, sizeof(objConstants));
          meshIndex++;
        }
      }
    }
  }

  void GraphicsDX12::buildMaterial(shared_ptr<Material> material)
  {
    Dx12MaterialData* materialData = (Dx12MaterialData*)material->getGraphicsData();
    uint32_t numTextures = 0;
    if (materialData == nullptr || material->getDirty())
    {
      if (materialData != nullptr)
      {
        // TODO: Free old resources
      }

      materialData = new Dx12MaterialData();
      CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
      hDescriptor.Offset(m_heapIndex, m_cbvSrvDescriptorSize);
      ComPtr<ID3D12Resource> texture = nullptr;
      D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

      shared_ptr<Texture> albedo = material->getAlbedoTexture();
      if (albedo != nullptr)
      {
        loadTexture(albedo, &materialData->m_albedoTexture);

        materialData->m_heapStartIndex = m_heapIndex;

        texture = materialData->m_albedoTexture.m_texture;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        m_device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
        numTextures++;
      }

      shared_ptr<Texture> normal = material->getNormalTexture();
      if (normal != nullptr)
      {
        loadTexture(normal, &materialData->m_normalTexture);

        texture = materialData->m_normalTexture.m_texture;
        hDescriptor.Offset(1, m_cbvSrvDescriptorSize);
        m_heapIndex++;

        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        m_device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
        numTextures++;
      }

      shared_ptr<Texture> roughness = material->getMetallicRoughnessTexture();
      if (roughness != nullptr)
      {
        loadTexture(roughness, &materialData->m_roughnessTexture);

        texture = materialData->m_roughnessTexture.m_texture;
        hDescriptor.Offset(1, m_cbvSrvDescriptorSize);
        m_heapIndex++;

        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        m_device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
        numTextures++;
      }

      materialData->m_inputLayout = m_inputLayouts[numTextures];
      materialData->m_rootSignature = m_rootSignatures[numTextures];
      materialData->m_shaderIndex = numTextures;
      materialData->m_materialIndex = m_materialIndex++;
      material->setGraphicsData(materialData);
    }
  }

  void GraphicsDX12::loadTexture(shared_ptr<Texture> texture, Dx12Texture* textureData)
  {
    HRESULT hr = S_OK;

    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    ZeroMemory(&textureDesc, sizeof(D3D12_RESOURCE_DESC));
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = texture->getWidth();
    textureDesc.Height = (uint32_t)texture->getHeight();
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    m_device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &textureDesc,
      D3D12_RESOURCE_STATE_COMMON,
      nullptr,
      IID_PPV_ARGS(&textureData->m_texture));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureData->m_texture.Get(), 0, 1);

    // Create the GPU upload buffer.
    m_device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&textureData->m_textureUploadHeap));

    D3D12_SUBRESOURCE_DATA tData = {};
    tData.pData = texture->getData();
    tData.RowPitch = texture->getWidth() * 4;
    tData.SlicePitch = tData.RowPitch * texture->getHeight();

    hr = m_graphicsCommandList->Reset(m_graphicsCommandAllocator.Get(), nullptr);
    if (!SUCCEEDED(hr))
    {
      //printLog("ERROR: graphics command list rest failed in resize.");
      return;
    }

    m_graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureData->m_texture.Get(),
      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

    UpdateSubresources(m_graphicsCommandList.Get(), textureData->m_texture.Get(), textureData->m_textureUploadHeap.Get(), 0, 0, 1, &tData);

    m_graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureData->m_texture.Get(),
      D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    // Close the command list and execute it to begin the initial GPU setup.
    m_graphicsCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_graphicsCommandList.Get() };
    m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    flushCommandQueue();
  }

  ComPtr<ID3DBlob> GraphicsDX12::loadShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint,
    const std::string& target)
  {
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
      entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
    {
      OutputDebugStringA((char*)errors->GetBufferPointer());
    }

    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: unable to compile shader.");
      return nullptr;
    }
    return byteCode;
  }

  void GraphicsDX12::createView(shared_ptr<View> view)
  {
    Dx12ViewData* viewData = new Dx12ViewData();
    view->setGraphicsData(viewData);

    vec2 size;
    view->getViewportSize(size);
    viewData->m_currentWidth = (uint32_t)size.x;
    viewData->m_currentHeight = (uint32_t)size.y;
    viewData->m_frameIndex = 0;
  }

  void GraphicsDX12::resize(uint32_t width, uint32_t height)
  {
    HRESULT hr = S_OK;

    m_width = width;
    m_height = height;

    flushCommandQueue();

    hr = m_graphicsCommandList->Reset(m_graphicsCommandAllocator.Get(), nullptr);
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: graphics command list rest failed in resize.");
      return;
    }

    // Release the previous resources we will be recreating.
    for (uint32_t i = 0; i < m_numFrames; ++i)
    {
      m_swapchainBufferResources[i]->m_resource.Reset();
      m_swapchainBufferResources[i]->m_depthResource.Reset();
      m_swapchainBufferResources[i]->m_albedoGbuffer.Reset();
      m_swapchainBufferResources[i]->m_normalGbuffer.Reset();
      m_swapchainBufferResources[i]->m_positionGbuffer.Reset();
      m_swapchainBufferResources[i]->m_firstFrame = true;
    }

    hr = m_swapChain->ResizeBuffers(m_numFrames, m_width, m_height, m_swapChainFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: swapchain resize buffers failed.");
      return;
    }

    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_swapchainBufferRTVHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(m_swapchainBufferDSVHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < m_numFrames; ++i)
    {
      hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapchainBufferResources[i]->m_resource));
      if (!SUCCEEDED(hr))
      {
        printLog("ERROR: swapchain resize get buffer failed.");
        return;
      }

      m_device->CreateRenderTargetView(m_swapchainBufferResources[i]->m_resource.Get(), nullptr, rtvHeapHandle);
      rtvHeapHandle.Offset(1, m_rtvDescriptorSize);

      // Create the Albedo Gbuffer
      D3D12_RESOURCE_DESC gbufferDesc;
      gbufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      gbufferDesc.Alignment = 0;
      gbufferDesc.Width = m_width;
      gbufferDesc.Height = m_height;
      gbufferDesc.DepthOrArraySize = 1;
      gbufferDesc.MipLevels = 1;
      gbufferDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
      gbufferDesc.SampleDesc.Count = 1;
      gbufferDesc.SampleDesc.Quality = 0;
      gbufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      gbufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

      D3D12_CLEAR_VALUE gbufferClear;
      gbufferClear.Format = DXGI_FORMAT_R11G11B10_FLOAT;
      gbufferClear.Color[0] = 0.0f;
      gbufferClear.Color[1] = 0.0f;
      gbufferClear.Color[2] = 0.0f;
      gbufferClear.Color[3] = 1.0f;
      hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &gbufferDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &gbufferClear,
        IID_PPV_ARGS(m_swapchainBufferResources[i]->m_albedoGbuffer.GetAddressOf()));

      D3D12_RENDER_TARGET_VIEW_DESC gbufferViewDesc = {};
      gbufferViewDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
      gbufferViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
      gbufferViewDesc.Texture2D.MipSlice = 0;
      m_device->CreateRenderTargetView(m_swapchainBufferResources[i]->m_albedoGbuffer.Get(), &gbufferViewDesc, rtvHeapHandle);
      rtvHeapHandle.Offset(1, m_rtvDescriptorSize);

      // Create the Normal Gbuffer
      gbufferDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      gbufferClear.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &gbufferDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &gbufferClear,
        IID_PPV_ARGS(m_swapchainBufferResources[i]->m_normalGbuffer.GetAddressOf()));

      gbufferViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      m_device->CreateRenderTargetView(m_swapchainBufferResources[i]->m_normalGbuffer.Get(), &gbufferViewDesc, rtvHeapHandle);
      rtvHeapHandle.Offset(1, m_rtvDescriptorSize);

      // Create the Position Gbuffer
      gbufferDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      gbufferClear.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &gbufferDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &gbufferClear,
        IID_PPV_ARGS(m_swapchainBufferResources[i]->m_positionGbuffer.GetAddressOf()));

      gbufferViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      m_device->CreateRenderTargetView(m_swapchainBufferResources[i]->m_positionGbuffer.Get(), &gbufferViewDesc, rtvHeapHandle);
      rtvHeapHandle.Offset(1, m_rtvDescriptorSize);

      // Create the Depth buffer
      D3D12_RESOURCE_DESC depthStencilDesc;
      depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      depthStencilDesc.Alignment = 0;
      depthStencilDesc.Width = m_width;
      depthStencilDesc.Height = m_height;
      depthStencilDesc.DepthOrArraySize = 1;
      depthStencilDesc.MipLevels = 1;
      depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;

      depthStencilDesc.SampleDesc.Count = 1;
      depthStencilDesc.SampleDesc.Quality = 0;
      depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

      D3D12_CLEAR_VALUE optClear;
      optClear.Format = DXGI_FORMAT_D32_FLOAT;
      optClear.DepthStencil.Depth = 1.0f;
      optClear.DepthStencil.Stencil = 0;
      hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(m_swapchainBufferResources[i]->m_depthResource.GetAddressOf()));
      if (!SUCCEEDED(hr))
      {
        printLog("ERROR: create depth buffer resource failed");
        return;
      }

      // Create descriptor to mip level 0 of entire resource using the format of the resource.
      D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
      dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
      dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
      dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
      dsvDesc.Texture2D.MipSlice = 0;
      m_device->CreateDepthStencilView(m_swapchainBufferResources[i]->m_depthResource.Get(), &dsvDesc, dsvHeapHandle);
      dsvHeapHandle.Offset(1, m_dsvDescriptorSize);
    }

    //
    // Create the SRV heap and views for the light pass.
    //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = m_numFrames * 3;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeapLightPass));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_srvHeapLightPass->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < m_numFrames; ++i)
    {
      D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      Dx12SwapchainBufferData* bufferData = m_swapchainBufferResources[i];

      srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srvDesc.Format = bufferData->m_albedoGbuffer->GetDesc().Format;
      srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MostDetailedMip = 0;
      srvDesc.Texture2D.MipLevels = bufferData->m_albedoGbuffer->GetDesc().MipLevels;
      srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
      m_device->CreateShaderResourceView(bufferData->m_albedoGbuffer.Get(), &srvDesc, hDescriptor);
      hDescriptor.Offset(1, m_cbvSrvDescriptorSize);

      srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srvDesc.Format = bufferData->m_normalGbuffer->GetDesc().Format;
      srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MostDetailedMip = 0;
      srvDesc.Texture2D.MipLevels = bufferData->m_normalGbuffer->GetDesc().MipLevels;
      srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
      m_device->CreateShaderResourceView(bufferData->m_normalGbuffer.Get(), &srvDesc, hDescriptor);
      hDescriptor.Offset(1, m_cbvSrvDescriptorSize);

      srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srvDesc.Format = bufferData->m_positionGbuffer->GetDesc().Format;
      srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MostDetailedMip = 0;
      srvDesc.Texture2D.MipLevels = bufferData->m_positionGbuffer->GetDesc().MipLevels;
      srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
      m_device->CreateShaderResourceView(bufferData->m_positionGbuffer.Get(), &srvDesc, hDescriptor);

      if (i < m_numFrames - 1)
      {
        hDescriptor.Offset(1, m_cbvSrvDescriptorSize);
      }
    }

    CD3DX12_RESOURCE_BARRIER depthBarriers[3];
    depthBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_swapchainBufferResources[0]->m_depthResource.Get(),
      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    depthBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_swapchainBufferResources[1]->m_depthResource.Get(),
      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    depthBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_swapchainBufferResources[2]->m_depthResource.Get(),
      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Transition the resource from its initial state to be used as a depth buffer.
    m_graphicsCommandList->ResourceBarrier(3, depthBarriers);

    // Execute the resize commands.
    hr = m_graphicsCommandList->Close();
    if (!SUCCEEDED(hr))
    {
      printLog("ERROR: close of resize command list failed");
      return;
    }
    ID3D12CommandList* cmdsLists[] = { m_graphicsCommandList.Get() };
    m_graphicsCommandQueue->ExecuteCommandLists(1, cmdsLists);

    // Wait until resize is complete.
    flushCommandQueue();

    // Update the viewport transform to cover the client area.
    m_screenViewport.TopLeftX = 0;
    m_screenViewport.TopLeftY = 0;
    m_screenViewport.Width = static_cast<float>(m_width);
    m_screenViewport.Height = static_cast<float>(m_height);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;

    m_scissorRect = { 0, 0, (LONG)m_width, (LONG)m_height };

    m_frameIndex = 0;
  }

  void GraphicsDX12::printLog(string s)
  {
    string st = s + "\n";
    TCHAR name[256];
    _tcscpy_s(name, CA2T(st.c_str()));
    OutputDebugString(name);
  }

  UINT GraphicsDX12::CalcConstantBufferByteSize(UINT byteSize)
  {
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (byteSize + 255) & ~255;
  }

  void GraphicsDX12::CopyData(void* dst, void* src, uint32_t size)
  {
    memcpy(dst, src, size);
  }

  DirectX::XMFLOAT4X4 GraphicsDX12::ConvertTransform(mat4 matrix)
  {
    mat4 transpose = glm::transpose(matrix);
    DirectX::XMFLOAT4X4 outMat;
    outMat.m[0][0] = transpose[0][0];
    outMat.m[0][1] = transpose[0][1];
    outMat.m[0][2] = transpose[0][2];
    outMat.m[0][3] = transpose[0][3];

    outMat.m[1][0] = transpose[1][0];
    outMat.m[1][1] = transpose[1][1];
    outMat.m[1][2] = transpose[1][2];
    outMat.m[1][3] = transpose[1][3];

    outMat.m[2][0] = transpose[2][0];
    outMat.m[2][1] = transpose[2][1];
    outMat.m[2][2] = transpose[2][2];
    outMat.m[2][3] = transpose[2][3];

    outMat.m[3][0] = transpose[3][0];
    outMat.m[3][1] = transpose[3][1];
    outMat.m[3][2] = -transpose[3][2];
    outMat.m[3][3] = transpose[3][3];

    return outMat;
  }
}
