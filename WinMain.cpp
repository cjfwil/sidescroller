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

bool AABBTest(float x1, float y1, float w1, float h1,
              float x2, float y2, float w2, float h2)
{
    return (fabs(x2 - x1) <= ((w1 + w2) / 2.0f) &&
            fabs(y2 - y1) <= ((h1 + h2) / 2.0f));
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Win32Window &window = Win32Window::GetInstance();

    D3D11Renderer renderer = D3D11Renderer(window.hwnd);
    renderer.LoadFontTexture();

    XAudioRenderer xa = XAudioRenderer();

    ShowWindow(window.hwnd, SW_SHOWDEFAULT);

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

            static unsigned char bricks[14][8] = {};
            for (int x = 0; x < 14; ++x)
            {
                for (int y = 0; y < 8; ++y)
                {
                    unsigned char brick = (y/2)+1;
                    bricks[x][y] = brick;
                }
            }

            ballX += trajX / 60;
            ballY += trajY / 60;

            bool hit = false;
            if (AABBTest(x1, y1, paddleWidth, paddleHeight,
                         ballX, ballY, ballW, ballH) &&
                trajX < 0)
            {
                // hit left paddle
                trajX *= -1;
                hit = true;
            }
            else if (AABBTest(x2, y2, paddleWidth, paddleHeight,
                              ballX, ballY, ballW, ballH) &&
                     trajX > 0)
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

            for (int x = 0; x < 14; ++x)
            {
                for (int y = 0; y < 8; ++y)
                {                    
                    static float (colour[5])[4] = {
                        {1, 0, 1, 1},
                        {1, 1, 0, 1},
                        {0.15f, 0.75f, .15f, 1},
                        {1, 0.5f, 0, 1},
                        {1, 0, 0, 1},
                    };

                    unsigned char brick = bricks[x][y];
                    if (brick != 0) {                        
                        float r = (colour[brick])[0];
                        float g = (colour[brick])[1];
                        float b = (colour[brick])[2];                        
                        // renderer.DrawRect((x*paddleHeight))-1.0f, y/16.0f, paddleWidth, paddleHeight, 3.14159f/2, r, g, b);
                        renderer.DrawRect((2*x/14.0f)-1.0f, y/16.0f, paddleWidth, paddleHeight, 3.14159f/2, r, g, b);
                    }
                }
            }

            float chScale = 0.2f;
            float chW = chScale * 4.0f / 6.0f;
            float chH = chScale;

            renderer.DrawFontRect(-0.5f, 0.66667f, score1 % 10, chW, chH);
            renderer.DrawFontRect(-0.5f - chW, 0.66667f, (score1 / 10) % 10, chW, chH);
            renderer.DrawFontRect(-0.5f - 2 * chW, 0.66667f, (score1 / 100) % 1000, chW, chH);

            renderer.DrawFontRect(0.5f + 2 * chW, 0.66667f, score2 % 10, chW, chH);
            renderer.DrawFontRect(0.5f + chW, 0.66667f, (score2 / 10) % 10, chW, chH);
            renderer.DrawFontRect(0.5f, 0.66667f, (score2 / 100) % 10, chW, chH);

            renderer.pSwapChain->Present(1, 0);

            frameCount++;
        }
    }
    return (0);
}