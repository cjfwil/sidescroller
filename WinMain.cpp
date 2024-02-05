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
            // TODO: make enemies move
            // destructible cover
            // red UFO
            // new sounds, fire, explosion, ufo sound, movement sound
            // explode animation
            // sprite for tank

            static float x1 = 0.0f;
            static float y1 = -0.8f;
            float paddleWidth = 0.1f;
            float paddleHeight = 0.025f;
            float enemyDimScale = 0.15f;
            float enemyWidth = 1 * enemyDimScale;
            float enemyHeight = 1 * 8.0 / 12.0f * enemyDimScale;

            struct v2
            {
                float x;
                float y;
            };

            struct clip_rect
            {
                v2 point[2];
            };

            // we define a sprite animation as just a series of rectangles and a framerate
            struct sprite_animation
            {
                int frameRate = 60;
                clip_rect frames[2]; // right now only need 2 frames of animation
            };

            struct enemy_info
            {
                float x;
                float y;
                float width;
                float height;
                unsigned int index;
                bool alive = true;
                unsigned char points = 1;

                sprite_animation anim = {};
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

                        int x1, y1, x2, y2;
                        if (index == 0)
                        {
                            x1 = 2;
                            y1 = 4;
                            x2 = 14;
                            y2 = 12;
                        }
                        if (index == 1)
                        {
                            x1 = 2;
                            y1 = 20;
                            x2 = x1 + 11;
                            y2 = y1 + 8;
                        }
                        if (index == 2)
                        {
                            x1 = 4;
                            y1 = 36;
                            x2 = x1 + 8;
                            y2 = y1 + 8;
                        }

                        b.anim.frames[0].point[0].x = x1;
                        b.anim.frames[0].point[0].y = y1;

                        b.anim.frames[0].point[1].x = x2;
                        b.anim.frames[0].point[1].y = y2;

                        b.anim.frames[1].point[0].x = x1 + 16;
                        b.anim.frames[1].point[0].y = y1;

                        b.anim.frames[1].point[1].x = x2 + 16;
                        b.anim.frames[1].point[1].y = y2;

                        b.width *= (x2 - x1) / 12.0f;
                        // b.height *= (y2-y1)/16.0f;

                        enemies[x][y] = b;

                        // // renderer.DrawGameTextureRect(enemy.x, enemy.y, enemy.width, enemy.height, 0,
                        //                              x1 + 16 * aniFrame, y1,
                        //                              x2 + 16 * aniFrame, y2);
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

            static int enemyAdvanceRate = 60;
            static float xEnemyShift = enemies[0][0].width / 3.0f;
            static bool shiftX = true;
            static float yEnemyShift = 0.0f;
            bool shiftEnemiesThisFrame = false;
            static int aniFrame = 0;
            if (frameCount % enemyAdvanceRate == 0)
            {
                aniFrame = (aniFrame == 0) ? 1 : 0;

                // find furthest alive enemy
                //  if that guy reaches end of screen, then shift down and reverse direction
                float leftmostX = 0.0f;
                float rightmostX = 0.0f;
                for (int x = 0; x < numenemysW; ++x)
                {
                    enemy_info e = enemies[x][0];
                    if (!e.alive)
                    {
                        for (int y = 0; y < numenemysH; ++y)
                        {
                            enemy_info e2 = enemies[x][y];
                            if (e2.alive)
                            {
                                if (e2.x < leftmostX)
                                {
                                    leftmostX = e2.x;
                                }
                                if (e2.x > rightmostX)
                                {
                                    rightmostX = e2.x;
                                }
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (e.x < leftmostX)
                        {
                            leftmostX = e.x;
                        }
                        if (e.x > rightmostX)
                        {
                            rightmostX = e.x;
                        }
                    }
                }
                if ((rightmostX >= 1.0f || leftmostX <= -1.0f) && yEnemyShift == 0)
                {
                    yEnemyShift = -enemies[0][0].height;
                    xEnemyShift *= -1;
                    enemyAdvanceRate = enemyAdvanceRate - 10;
                    if (enemyAdvanceRate <= 0)
                        enemyAdvanceRate = 5;
                }
                else
                {
                    yEnemyShift = 0;
                }
                shiftEnemiesThisFrame = true;
            }
            if (shiftEnemiesThisFrame)
            {
                shiftEnemiesThisFrame = false;

                for (int x = 0; x < numenemysW; ++x)
                {
                    for (int y = 0; y < numenemysH; ++y)
                    {
                        if (yEnemyShift == 0.0f)
                            enemies[x][y].x += xEnemyShift;
                        enemies[x][y].y += yEnemyShift;

                        if (enemies[x][y].y <= y1 && enemies[x][y].alive)
                        {
                            // GAME OVER
                            enemyInit = false;
                            enemyAdvanceRate = 60;
                        }
                    }
                }
            }

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

            for (int x = 0; x < numenemysW; ++x)
            {
                for (int y = 0; y < numenemysH; ++y)
                {
                    enemy_info enemy = enemies[x][y];
                    if (enemy.alive)
                    {
                        int x1, y1, x2, y2;
                        x1 = enemy.anim.frames[aniFrame].point[0].x;
                        y1 = enemy.anim.frames[aniFrame].point[0].y;
                        x2 = enemy.anim.frames[aniFrame].point[1].x;
                        y2 = enemy.anim.frames[aniFrame].point[1].y;

                        renderer.DrawGameTextureRect(enemy.x, enemy.y, enemy.width, enemy.height, 0,
                                                     x1, y1, x2, y2);
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