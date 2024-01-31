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
            static float x1 = -1.0f;
            static float y1 = -0.8f;
            float paddleWidth = 0.1f;
            float paddleHeight = 0.025f;
            float brickWidth = 0.13f; //1/7.0f
            float brickHeight = 0.05f; // 1/16

            struct brick_info
            {
                float x;
                float y;
                float width;
                float height;
                float colour[4];
                bool alive = true;
                unsigned char points = 1;
            };

            static brick_info bricks[14][8] = {};
            
            static bool brickInit = false;
            if (!brickInit)
            {
                for (int x = 0; x < 14; ++x)
                {
                    for (int y = 0; y < 8; ++y)
                    {
                        static unsigned char points[4] = {
                            2, 4, 8, 16
                        };
                        static float(colour[4])[4] = {                            
                            {0.15f, 0.75f, .15f, 1},
                            {1, 1, 0, 1},
                            {1, 0.41f, 0, 1},
                            {1, 0, 0, 1},
                        };
                        unsigned char index = (y / 2);

                        brick_info b;
                        b.width = brickWidth;
                        b.height = brickHeight;
                        b.x = x / 7.0f + b.width/2.0f - 1.0f;
                        b.y = y / 16.0f;
                        b.colour[0] = (colour[index])[0];
                        b.colour[1] = (colour[index])[1];
                        b.colour[2] = (colour[index])[2];
                        b.colour[3] = (colour[index])[3];
                        b.points = points[index];
                        bricks[x][y] = b;
                    }
                }
                brickInit = true;
            }

            if (GetAsyncKeyState('D') & 0x8000)
            {
                x1 += 1.0f / 60;
                if (x1 > 1.0f)
                    x1 = 1;
            }
            if (GetAsyncKeyState('A') & 0x8000)
            {
                x1 -= 1.0f / 60;
                if (x1 < -1.0f)
                    x1 = -1;
            }

            static unsigned int score = 0;

            static float ballX = 0;
            static float ballY = -1.0f;
            float ballW = 0.025f;
            float ballH = ballW;
            static float speed = 1.0f;

            static float trajX = 1.0f / 1.414f;
            static float trajY = 1.0f / 1.414f;

            ballX += trajX / 60 * speed;
            ballY += trajY / 60 * speed;

            float soundFreq = speed;
            bool hit = false;
            if (AABBTest(x1, y1, paddleWidth, paddleHeight,
                         ballX, ballY, ballW, ballH) &&
                trajY < 0)
            {
                // hit left paddle
                trajY *= -1;
                hit = true;
            }
            if (ballX < -1.0f || ballX > 1.0f)
            {
                trajX *= -1;
                // trajY *= -1;
                // ballX = 0.0f;
                ballX = (trajX < 0) ? 1.0f : -1.0f;
                hit = true;
                soundFreq = 0.9f * speed;
            }

            if (ballY > 1.0f || ballY < -1.0f)
            {
                trajY *= -1;
                hit = true;
            }

            for (int x = 0; x < 14; ++x)
            {
                for (int y = 0; y < 8; ++y)
                {
                    brick_info b = bricks[x][y];                    
                    bool result = AABBTest(ballX, ballY, ballW, ballH,
                                           b.x, b.y, b.width, b.height);
                    if (b.alive && result)
                    {
                        if (trajY > 0 || trajY < 0)
                            trajY *= -1;   
                        score += b.points;
                        // speed *= 1.0f + (1.0f / 100.0f);
                        soundFreq = speed + (b.points/100.f);
                        bricks[x][y].alive = false;
                        hit = true;
                    }                    
                }
            }

            if (hit)
            {
                xa.Play(soundFreq);
            }

            // draw game
            constantBufferData.view[0] = (float)window.height / (float)window.width;
            fontConstantBufferData.view[0] = (float)window.height / (float)window.width;
            renderer.StartDraw(0.255f, 0.255f, 0.255f);
            renderer.DrawRect(x1, y1, paddleWidth, paddleHeight);
            renderer.DrawRect(ballX, ballY, ballW, ballH);

            renderer.DrawRect(1.0f, 0.0f, 0.01f, 2);
            renderer.DrawRect(-1.0f, 0.0f, 0.01f, 2);

            for (int x = 0; x < 14; ++x)
            {
                for (int y = 0; y < 8; ++y)
                {
                    brick_info brick = bricks[x][y];

                    if (brick.alive)
                        renderer.DrawRect(brick.x, brick.y,
                                          brick.width, brick.height, 0,
                                          brick.colour[0], brick.colour[1], brick.colour[2]);
                }
            }

            float chScale = 0.2f;
            float chW = chScale * 4.0f / 6.0f;
            float chH = chScale;

            renderer.DrawFontRect(-0.5f, 0.66667f, score % 10, chW, chH);
            renderer.DrawFontRect(-0.5f - chW, 0.66667f, (score / 10) % 10, chW, chH);
            renderer.DrawFontRect(-0.5f - 2 * chW, 0.66667f, (score / 100) % 1000, chW, chH);            

            renderer.pSwapChain->Present(1, 0);

            frameCount++;
        }
    }
    return (0);
}