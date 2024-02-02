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
    renderer.LoadTextures();

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
            // game code
            static float x1 = 0.0f;
            static float y1 = -0.8f;
            float paddleWidth = 0.1f;
            float paddleHeight = 0.025f;
            float enemyDimScale = 0.2f;
            float enemyWidth = 1 * enemyDimScale;
            float enemyHeight = 1 * 8.0 / 12.0f * enemyDimScale;

            struct enemy_info
            {
                float x;
                float y;
                float width;
                float height;
                unsigned int index;
                bool alive = true;
                unsigned char points = 1;
            };

            static const int numenemysW = 11;
            static const int numenemysH = 5;
            static enemy_info enemies[numenemysW][numenemysH] = {};

            static bool enemyInit = false;
            if (!enemyInit)
            {
                for (int x = 0; x < numenemysW; ++x)
                {
                    for (int y = 0; y < numenemysH; ++y)
                    {
                        static unsigned char points[4] = {2, 4, 8, 16};
                        unsigned int index = (y / 2);

                        float scaleFactor = 0.80f;
                        enemy_info b;
                        b.width = enemyWidth;
                        b.height = enemyHeight;
                        b.x = x * b.width + b.width / 2.0f - 1.0f;
                        b.y = y * b.height;
                        b.points = points[index];
                        b.index = index;

                        b.width *= scaleFactor;
                        b.height *= scaleFactor;
                        enemies[x][y] = b;
                    }
                }
                enemyInit = true;
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

            static struct projectile
            {
                float x;
                float y;
                float w = 0.025f;
                float h = w * 3;
                float trajX = 0;
                float trajY = 1.0f / 1.414f;
                bool active = false;
            } ball;

            static unsigned int score = 0;

            static float speed = 2.5f;

            float soundFreq = speed * 0.4f;
            bool hit = false;

            if (ball.active)
            {
                ball.x += ball.trajX / 60 * speed;
                ball.y += ball.trajY / 60 * speed;

                if (AABBTest(ball.x, ball.y, ball.w, ball.h,
                             0, 0, 2, 2)) // test ball against screen
                {
                    for (int x = 0; x < numenemysW; ++x)
                    {
                        for (int y = 0; y < numenemysH; ++y)
                        {
                            enemy_info b = enemies[x][y];
                            bool result = AABBTest(ball.x, ball.y, ball.w, ball.h,
                                                   b.x, b.y, b.width, b.height);
                            if (b.alive && result)
                            {
                                score += b.points;
                                soundFreq = speed + (b.points / 100.f);
                                enemies[x][y].alive = false;
                                hit = true;
                                ball.active = false;
                            }
                        }
                    }
                }
                else
                {
                    ball.active = false;
                }
            }
            else
            {
                ball.x = x1;
                ball.y = y1;
                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                {
                    xa.Play();
                    ball.x = x1;
                    ball.y = y1;
                    ball.active = true;
                }
            }

            if (hit)
            {
                xa.Play(soundFreq);
                hit = false;
            }

            // draw game
            constantBufferData.view[0] = (float)window.height / (float)window.width;
            texRenderConstantBufferData.view[0] = (float)window.height / (float)window.width;
            renderer.StartDraw(0.255f, 0.255f, 0.255f);
            renderer.DrawRect(x1, y1, paddleWidth, paddleHeight);
            if (ball.active)
                renderer.DrawRect(ball.x, ball.y, ball.w, ball.h);

            static int aniFrame = 0;
            if (frameCount % 60 == 0)
            {
                aniFrame = (aniFrame == 0) ? 1 : 0;
            }
            for (int x = 0; x < numenemysW; ++x)
            {
                for (int y = 0; y < numenemysH; ++y)
                {
                    enemy_info enemy = enemies[x][y];
                    if (enemy.alive)
                    {
                        // todo move this to enemy creation and account for different sizes of clip,
                        //  so change actual width and height of enemy (plus collision)
                        // animation is just a series of rectangles and a framerate
                        int x1, y1, x2, y2;
                        x1 = 2;
                        y1 = 4;
                        x2 = 14;
                        y2 = 12;
                        if (enemy.index == 1)
                        {
                            x1 = 2;
                            y1 = 20;
                            x2 = x1 + 11;
                            y2 = y1 + 8;
                        }
                        else if (enemy.index == 2)
                        {
                            x1 = 4;
                            y1 = 36;
                            x2 = x1 + 8;
                            y2 = y1 + 8;
                        }

                        renderer.DrawGameTextureRect(enemy.x, enemy.y, enemy.width, enemy.height, 0,
                                                     x1 + 16 * aniFrame, y1,
                                                     x2 + 16 * aniFrame, y2);
                    }
                }
            }

            float chScale = 0.1f;
            float chW = chScale * 4.0f / 6.0f;
            float chH = chScale;

            renderer.DrawFontRect(-1.05f, 0.66667f, score % 10, chW, chH);
            renderer.DrawFontRect(-1.05f - chW, 0.66667f, (score / 10) % 10, chW, chH);
            renderer.DrawFontRect(-1.05f - 2 * chW, 0.66667f, (score / 100) % 1000, chW, chH);

            frameCount++;
            renderer.pSwapChain->Present(1, 0);
        }
    }
    return (0);
}