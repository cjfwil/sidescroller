#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shcore.lib")

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <windows.h>
#include <stdbool.h>

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <stdio.h>

#include "Win32Window.hpp"

class D3D11Renderer
{
public:
    D3D_FEATURE_LEVEL m_featureLevel;
    D3D11_TEXTURE2D_DESC m_bbDesc;
    D3D11_VIEWPORT m_viewport;

    ID3D11Device *pDevice;
    ID3D11DeviceContext *pContext;
    IDXGISwapChain *pSwapChain;
    ID3D11Texture2D *pBackBuffer;
    ID3D11RenderTargetView *pRenderTarget;

    D3D11Renderer(HWND hwnd)
    {
        HRESULT hr = S_OK;
        hr = CreateDeviceResources();
        if (SUCCEEDED(hr))
        {
            hr = CreateShaders();
            if (SUCCEEDED(hr))
            {
                hr = CreateQuad();
                if (SUCCEEDED(hr))
                {
                    hr = CreateWindowResources(hwnd);
                    if (FAILED(hr))
                        OutputDebugStringA("Failed to create D3D11 Window Resources");
                }
                else
                    OutputDebugStringA("Failed to create quad");
            }
            else
                OutputDebugStringA("Failed to create shaders\n");
        }
        else
            OutputDebugStringA("Failed to creat D3D11 Device Resources\n");
    }

    void Draw()
    {
        const float teal[] = {0.098f, 0.439f, 0.439f, 1.000f};
        pContext->ClearRenderTargetView(
            pRenderTarget,
            teal);
        // context->ClearDepthStencilView(
        //     depthStencil,
        //     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        //     1.0f,
        //     0);

        // Set the render target.
        pContext->OMSetRenderTargets(
            1,
            &pRenderTarget,
            nullptr);

        UINT stride = sizeof(DirectX::XMFLOAT2);
        UINT offset = 0;

        pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->IASetInputLayout(m_pInputLayout);
        pContext->VSSetShader(m_pVertexShader, nullptr, 0);
        pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
        pContext->PSSetShader(m_pPixelShader, nullptr, 0);
        pContext->DrawIndexed(m_indexCount, 0, 0);
    }

private:
    HRESULT CreateDeviceResources()
    {
        HRESULT hr = S_OK;

        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
        };

        UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        hr = D3D11CreateDevice(
            nullptr,                  // Specify nullptr to use the default adapter.
            D3D_DRIVER_TYPE_HARDWARE, // Create a device using the hardware graphics driver.
            0,                        // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            deviceFlags,              // Set debug and Direct2D compatibility flags.
            levels,                   // List of feature levels this app can support.
            ARRAYSIZE(levels),        // Size of the list above.
            D3D11_SDK_VERSION,        // Always set this to D3D11_SDK_VERSION for Windows Store apps.
            &pDevice,                 // Returns the Direct3D device created.
            &m_featureLevel,          // Returns feature level of device created.
            &pContext                 // Returns the device immediate context.
        );

        if (FAILED(hr))
        {
            OutputDebugStringA("Failed to create D3D11 Device\n");
        }

        // // Store pointers to the Direct3D 11.1 API device and immediate context.
        // device.As(&m_pd3dDevice);
        // context.As(&m_pd3dDeviceContext);

        return hr;
    }

    HRESULT CreateWindowResources(HWND hWnd)
    {
        HRESULT hr = S_OK;

        DXGI_SWAP_CHAIN_DESC desc;
        ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
        desc.Windowed = TRUE; // Sets the initial state of full-screen mode.
        desc.BufferCount = 2;
        desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SampleDesc.Count = 1;   // multisampling setting
        desc.SampleDesc.Quality = 0; // vendor-specific flag
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.OutputWindow = hWnd;

        IDXGIDevice3 *dxgiDevice;
        pDevice->QueryInterface(__uuidof(IDXGIDevice3), reinterpret_cast<void **>(&dxgiDevice));

        IDXGIAdapter *adapter;
        IDXGIFactory *factory;

        hr = dxgiDevice->GetAdapter(&adapter);

        if (SUCCEEDED(hr))
        {
            adapter->GetParent(IID_PPV_ARGS(&factory));

            hr = factory->CreateSwapChain(
                pDevice,
                &desc,
                &pSwapChain);
        }

        // Configure the back buffer, stencil buffer, and viewport.
        hr = ConfigureBackBuffer();

        return hr;
    }

    HRESULT ConfigureBackBuffer()
    {
        HRESULT hr = S_OK;

        hr = pSwapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            (void **)&pBackBuffer);

        hr = pDevice->CreateRenderTargetView(
            pBackBuffer,
            nullptr,
            &pRenderTarget);

        pBackBuffer->GetDesc(&m_bbDesc);

        // Create a depth-stencil view for use with 3D rendering if needed.
        // CD3D11_TEXTURE2D_DESC depthStencilDesc(
        //     DXGI_FORMAT_D24_UNORM_S8_UINT,
        //     static_cast<UINT>(m_bbDesc.Width),
        //     static_cast<UINT>(m_bbDesc.Height),
        //     1, // This depth stencil view has only one texture.
        //     1, // Use a single mipmap level.
        //     D3D11_BIND_DEPTH_STENCIL);

        // pDevice->CreateTexture2D(
        //     &depthStencilDesc,
        //     nullptr,
        //     &m_pDepthStencil);

        // CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);

        // m_pd3dDevice->CreateDepthStencilView(
        //     m_pDepthStencil.Get(),
        //     &depthStencilViewDesc,
        //     &m_pDepthStencilView);

        ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
        m_viewport.Height = (float)m_bbDesc.Height;
        m_viewport.Width = (float)m_bbDesc.Width;
        m_viewport.MinDepth = 0;
        m_viewport.MaxDepth = 1;

        pContext->RSSetViewports(
            1,
            &m_viewport);

        return hr;
    }

    ID3D11Buffer *m_pVertexBuffer;
    ID3D11Buffer *m_pIndexBuffer;
    unsigned int m_indexCount;
    ID3D11VertexShader *m_pVertexShader;
    ID3D11InputLayout *m_pInputLayout;
    ID3D11InputLayout *m_pInputLayoutExtended;
    ID3D11PixelShader *m_pPixelShader;
    ID3D11Buffer *m_pConstantBuffer;

    HRESULT CreateShaders()
    {
        HRESULT hr = S_OK;

        // Use the Direct3D device to load resources into graphics memory.

        // You'll need to use a file loader to load the shader bytecode. In this
        // example, we just use the standard library.
        FILE *vShader, *pShader;
        BYTE *bytes;

        size_t destSize = 4096;
        size_t bytesRead = 0;
        bytes = new BYTE[destSize];

        fopen_s(&vShader, "shaders/VShader.cso", "rb");
        bytesRead = fread_s(bytes, destSize, 1, 4096, vShader);
        hr = pDevice->CreateVertexShader(
            bytes,
            bytesRead,
            nullptr,
            &m_pVertexShader);

        D3D11_INPUT_ELEMENT_DESC iaDesc[] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},

                {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };

        hr = pDevice->CreateInputLayout(
            iaDesc,
            ARRAYSIZE(iaDesc),
            bytes,
            bytesRead,
            &m_pInputLayout);

        delete bytes;

        bytes = new BYTE[destSize];
        bytesRead = 0;
        fopen_s(&pShader, "shaders/PShader.cso", "rb");
        bytesRead = fread_s(bytes, destSize, 1, 4096, pShader);
        hr = pDevice->CreatePixelShader(
            bytes,
            bytesRead,
            nullptr,
            &m_pPixelShader);

        delete bytes;

        // CD3D11_BUFFER_DESC cbDesc(
        //     sizeof(ConstantBufferStruct),
        //     D3D11_BIND_CONSTANT_BUFFER);

        // hr = device->CreateBuffer(
        //     &cbDesc,
        //     nullptr,
        //     m_pConstantBuffer.GetAddressOf());

        fclose(vShader);
        fclose(pShader);

        return hr;
    }

    HRESULT CreateQuad()
    {
        HRESULT hr = S_OK;

        DirectX::XMFLOAT2 vertices[] = {
            DirectX::XMFLOAT2(-0.5f, 0.5f),
            DirectX::XMFLOAT2(0.5f, 0.5f),
            DirectX::XMFLOAT2(-0.5f, -0.5f),
            DirectX::XMFLOAT2(0.5f, -0.5f),
            DirectX::XMFLOAT2(0.0f, 0.0f), //padding
        };

        // Create vertex buffer:

        CD3D11_BUFFER_DESC vDesc(
            sizeof(vertices),
            D3D11_BIND_VERTEX_BUFFER);

        D3D11_SUBRESOURCE_DATA vData;
        ZeroMemory(&vData, sizeof(D3D11_SUBRESOURCE_DATA));
        vData.pSysMem = vertices;
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        hr = pDevice->CreateBuffer(
            &vDesc,
            &vData,
            &m_pVertexBuffer);

        // Create index buffer:
        unsigned short quadIndices[] = {0, 1, 2, 1, 3, 2};

        m_indexCount = ARRAYSIZE(quadIndices);

        CD3D11_BUFFER_DESC iDesc(
            sizeof(quadIndices),
            D3D11_BIND_INDEX_BUFFER);

        D3D11_SUBRESOURCE_DATA iData;
        ZeroMemory(&iData, sizeof(D3D11_SUBRESOURCE_DATA));
        iData.pSysMem = quadIndices;
        iData.SysMemPitch = 0;
        iData.SysMemSlicePitch = 0;

        hr = pDevice->CreateBuffer(
            &iDesc,
            &iData,
            &m_pIndexBuffer);

        return hr;
    }
};

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Win32Window &window = Win32Window::GetInstance();
    ShowWindow(window.hwnd, SW_SHOWDEFAULT);

    D3D11Renderer renderer = D3D11Renderer(window.hwnd);

    bool quit = false;
    while (!quit)
    {
        MSG msg;
        bool bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);
        if (bGotMsg)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                quit = true;
        }
        else
        {

            renderer.Draw();
            renderer.pSwapChain->Present(1, 0);
        }
    }
    return (0);
}