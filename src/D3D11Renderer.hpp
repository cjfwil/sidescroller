#ifndef D3D11RENDERER_HPP
#define D3D11RENDERER_HPP

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "src/stb_image.h"

struct d3d_texture_info
{
    unsigned int width;
    unsigned int height;
    ID3D11Texture2D *texture;
    ID3D11ShaderResourceView *shaderResourceView;
};

struct d3d_tileset_texture
{
    int tileWidth = 32;
    d3d_texture_info textureInfo;
};

__declspec(align(16)) struct ConstantBufferStruct
{
    float view[16] = {1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, 0, 1, 0,
                      0, 0, 0, 1};
    float colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float offset[2] = {0, 0};
    float scale[2] = {1, 1};
    float rot;
} constantBufferData;
static_assert((sizeof(ConstantBufferStruct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

__declspec(align(16)) struct TexRenderConstantBufferStruct
{
    float view[16] = {1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, 0, 1, 0,
                      0, 0, 0, 1};
    float offset[2] = {0, 0};
    float scale[2] = {1, 1};
    float uvOffset[2] = {0, 0};
    float uvScale[2] = {1, 1};
    float rot;
    float texSize;
} texRenderConstantBufferData;
static_assert((sizeof(TexRenderConstantBufferStruct) % 16) == 0, "TexRenderConstantBufferStruct size must be 16-byte aligned");

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
            hr = CreateShaders("shaders/VShader.cso", "shaders/PShader.cso");
            if (SUCCEEDED(hr))
            {
                hr = CreateShaders("shaders/VShaderFont.cso", "shaders/PShaderFont.cso");
                hr = CreateConstantBuffers();
                hr = CreateQuad();
                if (SUCCEEDED(hr))
                {
                    hr = CreateWindowResources(hwnd);
                    if (FAILED(hr))
                        OutputDebugStringA("Failed to create D3D11 Window Resources\n");
                }
                else
                    OutputDebugStringA("Failed to create quad\n");
            }
            else
                OutputDebugStringA("Failed to create shaders\n");
        }
        else
            OutputDebugStringA("Failed to create D3D11 Device Resources\n");
    }

    void DrawRect(float x, float y, float w = 1.0f, float h = 1.0f, float theta = 0.0f,
                  float r = 1.0f, float g = 1.0f, float b = 1.0f)
    {
        constantBufferData.offset[0] = x;
        constantBufferData.offset[1] = y;
        constantBufferData.scale[0] = w;
        constantBufferData.scale[1] = h;
        constantBufferData.colour[0] = r;
        constantBufferData.colour[1] = g;
        constantBufferData.colour[2] = b;
        constantBufferData.rot = theta;

        pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &constantBufferData, 0, 0);

        UINT stride = sizeof(local_vertex);
        UINT offset = 0;

        pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->IASetInputLayout(m_pInputLayout);
        pContext->VSSetShader(m_pVertexShader[0], nullptr, 0);
        pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
        pContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);
        pContext->PSSetShader(m_pPixelShader[0], nullptr, 0);
        pContext->DrawIndexed(m_indexCount, 0, 0);

        constantBufferData.offset[0] = 0.0f;
        constantBufferData.offset[1] = 0.0f;
        constantBufferData.scale[0] = 1.0f;
        constantBufferData.scale[1] = 1.0f;
    }

    void DrawFontRect(float x, float y, unsigned char number, float w = 1.0f, float h = 1.0f, float theta = 0.0f)
    {
        // TODO: New constant buffer type for rendering fonts, dont need view space matrix, only screen
        texRenderConstantBufferData.offset[0] = x;
        texRenderConstantBufferData.offset[1] = y;
        texRenderConstantBufferData.uvScale[0] = 1 / 10.0f;
        texRenderConstantBufferData.uvScale[1] = 1 / 4.0f;
        texRenderConstantBufferData.uvOffset[0] = number / 10.0f;
        texRenderConstantBufferData.uvOffset[1] = 3 / 4.0f;
        texRenderConstantBufferData.scale[0] = w;
        texRenderConstantBufferData.scale[1] = h;
        texRenderConstantBufferData.rot = theta;
        texRenderConstantBufferData.texSize = 50.0f;

        pContext->UpdateSubresource(m_pTexRenderConstantBuffer, 0, nullptr, &texRenderConstantBufferData, 0, 0);

        UINT stride = sizeof(local_vertex);
        UINT offset = 0;

        pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->IASetInputLayout(m_pInputLayout);
        pContext->VSSetShader(m_pVertexShader[1], nullptr, 0);
        pContext->VSSetConstantBuffers(0, 1, &m_pTexRenderConstantBuffer);
        pContext->PSSetConstantBuffers(0, 1, &m_pTexRenderConstantBuffer);
        pContext->PSSetShader(m_pPixelShader[1], nullptr, 0);
        pContext->PSSetShaderResources(0u, 1u, &fontTexture.shaderResourceView);
        pContext->PSSetSamplers(0, 1, &samplerState);
        pContext->DrawIndexed(m_indexCount, 0, 0);

        texRenderConstantBufferData.offset[0] = 0.0f;
        texRenderConstantBufferData.offset[1] = 0.0f;
        texRenderConstantBufferData.scale[0] = 1.0f;
        texRenderConstantBufferData.scale[1] = 1.0f;
    }

    // TODO: Arguments for this: pass in two points to crop the image
    //  store knowledge of the game texture and just allow pixel positions to be passed
    void DrawGameTextureRect(float x = 0, float y = 0, float w = 1, float h = 1, float theta = 0,
                             int uvX1 = 0, int uvY1 = 0, int uvX2 = 0, int uvY2 = 0, d3d_texture_info *txt = NULL)
    {
        d3d_texture_info *t = txt;
        if (txt == NULL)
        {
            t = &gameTexture;
        }
        // TODO: New constant buffer type for rendering fonts, dont need view space matrix, only screen
        float xScale = (uvX2 - uvX1) / (float)t->width;
        float yScale = (uvY2 - uvY1) / (float)t->height;
        if (uvX1 == uvX2)
            xScale = 1.0f;
        if (uvY1 == uvY2)
            yScale = 1.0f;

        texRenderConstantBufferData.offset[0] = x;
        texRenderConstantBufferData.offset[1] = y;
        texRenderConstantBufferData.uvScale[0] = xScale;
        texRenderConstantBufferData.uvScale[1] = yScale;
        texRenderConstantBufferData.uvOffset[0] = uvX1 / (float)t->width;
        texRenderConstantBufferData.uvOffset[1] = uvY1 / (float)t->height;
        texRenderConstantBufferData.scale[0] = w;
        texRenderConstantBufferData.scale[1] = h;
        texRenderConstantBufferData.rot = theta;
        texRenderConstantBufferData.texSize = (float)t->width;

        pContext->UpdateSubresource(m_pTexRenderConstantBuffer, 0, nullptr, &texRenderConstantBufferData, 0, 0);

        UINT stride = sizeof(local_vertex);
        UINT offset = 0;

        pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->IASetInputLayout(m_pInputLayout);
        pContext->VSSetShader(m_pVertexShader[1], nullptr, 0);
        pContext->VSSetConstantBuffers(0, 1, &m_pTexRenderConstantBuffer);
        pContext->PSSetConstantBuffers(0, 1, &m_pTexRenderConstantBuffer);
        pContext->PSSetShader(m_pPixelShader[1], nullptr, 0);
        pContext->PSSetShaderResources(0u, 1u, &t->shaderResourceView);
        pContext->PSSetSamplers(0, 1, &samplerState);
        pContext->DrawIndexed(m_indexCount, 0, 0);

        texRenderConstantBufferData.offset[0] = 0.0f;
        texRenderConstantBufferData.offset[1] = 0.0f;
        texRenderConstantBufferData.scale[0] = 1.0f;
        texRenderConstantBufferData.scale[1] = 1.0f;
    }

    void DrawTile(float x, float y, float w, float h, uint8_t index)
    {
        int numTilesInX = tilesetTexture.textureInfo.width / tilesetTexture.tileWidth;
        int x_index = (index % numTilesInX)*tilesetTexture.tileWidth;
        int y_index = (index / numTilesInX)*tilesetTexture.tileWidth;
        int x_index_end = x_index + tilesetTexture.tileWidth;
        int y_index_end = y_index + tilesetTexture.tileWidth;
        DrawGameTextureRect(x, y, w, h, 0, x_index, y_index, x_index_end, y_index_end, &(tilesetTexture.textureInfo));
    }

    void StartDraw(float r = 0.098f, float g = 0.439f, float b = 0.439f)
    {
        const float backColour[] = {r, g, b, 1.000f};
        pContext->ClearRenderTargetView(
            pRenderTarget,
            backColour);
        // context->ClearDepthStencilView(
        //     depthStencil,
        //     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        //     1.0f,
        //     0);

        pContext->OMSetRenderTargets(1, &pRenderTarget, nullptr);
        pContext->OMSetBlendState(blendState, NULL, 0xFFFFFFFFu);
    }

    d3d_texture_info LoadTexture(char *path, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        d3d_texture_info result;
        int texWidth, texHeight, n;
        int forcedN = 4;
        stbi_set_flip_vertically_on_load(1);
        unsigned char *clrData = stbi_load(path, &texWidth, &texHeight, &n, forcedN);

        result.width = texWidth;
        result.height = texHeight;

        // create texture d3d11

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = texWidth;
        texDesc.Height = texHeight;
        texDesc.MipLevels = 0;
        texDesc.ArraySize = 1;
        texDesc.Format = format;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = (void *)clrData;
        sd.SysMemPitch = texWidth * sizeof(unsigned int);

        ID3D11Texture2D *texture;

        HRESULT hr = pDevice->CreateTexture2D(&texDesc, nullptr, &texture);
        if (FAILED(hr))
        {
            OutputDebugStringA("Failed to create texture 2d\n");
        }
        pContext->UpdateSubresource(texture, 0, nullptr, clrData, texWidth * forcedN, 0);
        stbi_image_free(clrData);

        result.texture = texture;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = UINT_MAX;
        hr = pDevice->CreateShaderResourceView(result.texture, &srvDesc, &result.shaderResourceView);

        return (result);
    }

    HRESULT LoadTextures(BOOL enableAlphaBlend = FALSE, BOOL enableLinearFilter = FALSE)
    {
        fontTexture = LoadTexture("assets/bitmap_font.png");
        gameTexture = LoadTexture("assets/space_invaders.png");
        tilesetTexture.textureInfo = LoadTexture("assets/tileset_image.png");

        D3D11_SAMPLER_DESC samplerDesc = {};
        if (enableLinearFilter)
        {
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // linear is for allowing sub pixel sampling (to come, need alpha blending first)
        }
        else
        {
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        }
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = enableAlphaBlend;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        HRESULT hr = pDevice->CreateBlendState(&blendDesc, &blendState);

        hr = pDevice->CreateSamplerState(&samplerDesc, &samplerState);
        return (hr);
    }

private:
    d3d_texture_info fontTexture;
    d3d_texture_info gameTexture;
    d3d_tileset_texture tilesetTexture;
    ID3D11SamplerState *samplerState;
    ID3D11BlendState *blendState;

    HRESULT CreateConstantBuffers()
    {
        HRESULT hr = S_OK;
        CD3D11_BUFFER_DESC cbDesc(
            sizeof(ConstantBufferStruct),
            D3D11_BIND_CONSTANT_BUFFER);

        hr = pDevice->CreateBuffer(
            &cbDesc,
            nullptr,
            &m_pConstantBuffer);

        CD3D11_BUFFER_DESC cbFontDesc(
            sizeof(TexRenderConstantBufferStruct),
            D3D11_BIND_CONSTANT_BUFFER);

        hr = pDevice->CreateBuffer(
            &cbFontDesc,
            nullptr,
            &m_pTexRenderConstantBuffer);
        return hr;
    }

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

            factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
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
    ID3D11VertexShader *m_pVertexShader[2] = {};
    ID3D11InputLayout *m_pInputLayout;
    ID3D11InputLayout *m_pInputLayoutExtended;

private:
    ID3D11PixelShader *m_pPixelShader[2] = {};
    ID3D11Buffer *m_pConstantBuffer;
    ID3D11Buffer *m_pTexRenderConstantBuffer;

    HRESULT CreateShaders(char *vs_path, char *ps_path)
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

        int vs_index = 0;
        if (m_pVertexShader[vs_index] != nullptr)
            vs_index++;

        fopen_s(&vShader, vs_path, "rb");
        bytesRead = fread_s(bytes, destSize, 1, 4096, vShader);
        hr = pDevice->CreateVertexShader(
            bytes,
            bytesRead,
            nullptr,
            &(m_pVertexShader[vs_index]));

        D3D11_INPUT_ELEMENT_DESC iaDesc[] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,
                 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},

                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
                 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };

        hr = pDevice->CreateInputLayout(
            iaDesc,
            ARRAYSIZE(iaDesc),
            bytes,
            bytesRead,
            &m_pInputLayout);

        delete bytes;

        int ps_index = 0;
        if (m_pPixelShader[ps_index])
            ps_index++;

        bytes = new BYTE[destSize];
        bytesRead = 0;
        fopen_s(&pShader, ps_path, "rb");
        bytesRead = fread_s(bytes, destSize, 1, 4096, pShader);
        hr = pDevice->CreatePixelShader(
            bytes,
            bytesRead,
            nullptr,
            &(m_pPixelShader[ps_index]));

        delete bytes;

        fclose(vShader);
        fclose(pShader);

        return hr;
    }

    struct local_vertex
    {
        DirectX::XMFLOAT2 pos;
        DirectX::XMFLOAT2 uv;
    };

    HRESULT CreateQuad()
    {
        HRESULT hr = S_OK;

        local_vertex vertices[] = {
            {DirectX::XMFLOAT2(-0.5f, 0.5f), DirectX::XMFLOAT2(0, 1)},
            {DirectX::XMFLOAT2(0.5f, 0.5f), DirectX::XMFLOAT2(1, 1)},
            {DirectX::XMFLOAT2(-0.5f, -0.5f), DirectX::XMFLOAT2(0, 0)},
            {DirectX::XMFLOAT2(0.5f, -0.5f), DirectX::XMFLOAT2(1, 0)},
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

#endif