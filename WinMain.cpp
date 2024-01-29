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

#include <xaudio2.h>

#include "src/Win32Window.hpp"
#include "src/D3D11Renderer.hpp"

#include "src/XAudioRenderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "src/stb_image.h"

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Win32Window &window = Win32Window::GetInstance();
    ShowWindow(window.hwnd, SW_SHOWDEFAULT);

    D3D11Renderer renderer = D3D11Renderer(window.hwnd);

    HRESULT hr = S_OK;

    // load texture (bitmap font)
    int texWidth, texHeight, n;
    int forcedN = 4;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *clrData = stbi_load("assets/bitmap_font.png", &texWidth, &texHeight, &n, forcedN);

    // create texture d3d11

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = texWidth;
    texDesc.Height = texHeight;
    texDesc.MipLevels = 0;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = (void *)clrData;
    sd.SysMemPitch = texWidth * sizeof(unsigned int);

    ID3D11Texture2D *bitmapFontTexture;
    ID3D11SamplerState *samplerState;
    hr = renderer.pDevice->CreateTexture2D(&texDesc, nullptr, &bitmapFontTexture);
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create texture 2d\n");
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = UINT_MAX;

    ID3D11ShaderResourceView *textureShaderResourceView;
    hr = renderer.pDevice->CreateShaderResourceView(bitmapFontTexture, &srvDesc, &textureShaderResourceView);

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = renderer.pDevice->CreateSamplerState(&samplerDesc, &samplerState);

    renderer.pContext->PSSetShader(renderer.m_pPixelShader[1], nullptr, 0);
    renderer.pContext->PSSetShaderResources(0u, 1u, &textureShaderResourceView);
    renderer.pContext->PSSetSamplers(0, 1, &samplerState);

    renderer.pContext->UpdateSubresource(bitmapFontTexture, 0, nullptr, clrData, texWidth * forcedN, 0);
    stbi_image_free(clrData);

    // constant buffers
    //  CD3D11_BUFFER_DESC cbDesc(
    //      sizeof(ConstantBufferStruct),
    //      D3D11_BIND_CONSTANT_BUFFER,
    //      D3D11_USAGE_DYNAMIC,
    //      D3D11_CPU_ACCESS_WRITE);

    CD3D11_BUFFER_DESC cbDesc(
        sizeof(ConstantBufferStruct),
        D3D11_BIND_CONSTANT_BUFFER);

    hr = renderer.pDevice->CreateBuffer(
        &cbDesc,
        nullptr,
        &renderer.m_pConstantBuffer);

    CD3D11_BUFFER_DESC cbFontDesc(
        sizeof(FontConstantBufferStruct),
        D3D11_BIND_CONSTANT_BUFFER);

    hr = renderer.pDevice->CreateBuffer(
        &cbFontDesc,
        nullptr,
        &renderer.m_pFontRenderConstantBuffer);

    // XAUDIO2
    XAudioRenderer xa = XAudioRenderer();

    // start xaudio

    // hr = xa.pSourceVoice->SubmitSourceBuffer(&buffer);

    // hr = xa.pSourceVoice->Start(0);

    unsigned int frameCount = 0;

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
            // play sound test only
            if (GetAsyncKeyState(VK_SPACE) & 0x8000)
            {
                xa.Play();
            }

            // game code
            float x1 = -1.0f;
            float x2 = 1.0f;
            static float y1 = 0.0f;
            static float y2 = 0.0f;
            float paddleWidth = 0.025f;
            float paddleHeight = 0.1f;
            if (GetAsyncKeyState('W') & 0x8000)
            {
                y1 += 1.0f / 60;
                if (y1 > 1.0f)
                    y1 = 1;
            }
            if (GetAsyncKeyState('S') & 0x8000)
            {
                y1 -= 1.0f / 60;
                if (y1 < -1.0f)
                    y1 = -1;
            }

            if (GetAsyncKeyState(VK_UP) & 0x8000)
            {
                y2 += 1.0f / 60;
                if (y2 > 1.0f)
                    y2 = 1;
            }
            if (GetAsyncKeyState(VK_DOWN) & 0x8000)
            {
                y2 -= 1.0f / 60;
                if (y2 < -1.0f)
                    y2 = -1;
            }

            static unsigned char score1 = 0;
            static unsigned char score2 = 0;

            static float ballX = 0;
            static float ballY = 1.0f;
            float ballW = 0.025f;
            float ballH = ballW;

            static float trajX = 1.0f / 1.414f;
            static float trajY = -1.0f / 1.414f;

            ballX += trajX / 60;
            ballY += trajY / 60;

            bool hit = false;
            if (fabs(ballX - x1) <= (paddleWidth / 2 + ballW / 2) &&
                fabs(ballY - y1) <= (paddleHeight / 2 + ballH / 2) && trajX < 0)
            {
                // hit left paddle
                trajX *= -1;
                hit = true;
            }
            else if (fabs(ballX - x2) <= (paddleWidth / 2 + ballW / 2) &&
                     fabs(ballY - y2) <= (paddleHeight / 2 + ballH / 2) && trajX > 0)
            {
                // hit right paddle
                trajX *= -1;
                hit = true;
            }
            else if (ballX < -1.0f || ballX > 1.0f)
            {
                if (ballX > 1.0f)
                    score1++;
                if (ballX < -1.0f)
                    score2++;
                trajX *= -1;
                trajY *= -1;
                ballX = 0.0f;
                ballY = (trajY < 0) ? 1.0f : -1.0f;
            }

            if (ballY > 1.0f || ballY < -1.0f)
            {
                trajY *= -1;
                hit = true;
            }

            if (hit)
            {
                xa.Play();
            }

            // draw game
            constantBufferData.view[0] = (float)window.height / (float)window.width;
            fontConstantBufferData.view[0] = (float)window.height / (float)window.width;
            renderer.StartDraw(0.255f, 0.255f, 0.255f);
            renderer.DrawRect(x1, y1, paddleWidth, paddleHeight);
            renderer.DrawRect(x2, y2, paddleWidth, paddleHeight);
            renderer.DrawRect(ballX, ballY, ballW, ballH);

            for (int i = 0; i <= 30; ++i)
            {
                renderer.DrawRect(0.0f, i / 15.0f - 1.0f, 0.01f, 0.035f);
            }

            float chScale = 0.3f;
            float chW = chScale * 4.0f / 6.0f;
            float chH = chScale;
            
            renderer.DrawFontRect(-0.5f, 0.66667f, score1 % 10, chW, chH);
            renderer.DrawFontRect(-0.5f - chW, 0.66667f, (score1 / 10) % 10, chW, chH);
            renderer.DrawFontRect(-0.5f - 2 * chW, 0.66667f, (score1 / 100) % 1000, chW, chH);

            renderer.DrawFontRect(0.5f + 2 * chW, 0.66667f, score2 % 10, 0.3f * 4.0f / 6.0f, 0.3f);
            renderer.DrawFontRect(0.5f + chW, 0.66667f, (score2 / 10) % 10, 0.3f * 4.0f / 6.0f, 0.3f);
            renderer.DrawFontRect(0.5f, 0.66667f, (score2 / 100) % 10, 0.3f * 4.0f / 6.0f, 0.3f);

            renderer.pSwapChain->Present(1, 0);

            frameCount++;
        }
    }
    return (0);
}