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
            // TODO:            
            // red UFO
            // new sounds: fire, explosion, ufo sound, enemy movement sound
            // enemy explode animation on death
            // sprite for tank
            // enemies shoot down projectiles

            static float x1 = 0.0f;
            static float y1 = -0.8f;
            float paddleWidth = 0.1f;
            float paddleHeight = 0.025f;
            float enemyDimScale = 0.18f;
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
            static const int spriteAnimMaxFrames = 64;
            struct sprite_animation
            {
                // int frameRate = 60;
                int numFrames = 0;
                clip_rect frames[spriteAnimMaxFrames];
            };

            float screenEdge = 4 / 3.0f;

            struct entity_info
            {
                float x;
                float y;
                float width;
                float height;
                unsigned int index;
                bool alive = true;
                unsigned int health;
                unsigned char points = 1;

                sprite_animation anim = {};
            };

            static entity_info shields[4][3 * 4] = {};

            static bool shieldInit = false;
            if (!shieldInit)
            {
                float shieldW = (enemyDimScale * 0.5f) * 0.75f;

                sprite_animation mainBlock = {};
                mainBlock.numFrames = 4;
                (mainBlock.frames[0]).point[0].x = 48;
                (mainBlock.frames[0]).point[0].y = 32;
                (mainBlock.frames[0]).point[1].x = 48 + 6;
                (mainBlock.frames[0]).point[1].y = 32 + 6;

                sprite_animation nwFacingSlope = {};
                nwFacingSlope.numFrames = 4;
                nwFacingSlope.frames[0].point[0].x = 48;
                nwFacingSlope.frames[0].point[0].y = 38;
                nwFacingSlope.frames[0].point[1].x = 48 + 6;
                nwFacingSlope.frames[0].point[1].y = 38 + 6;

                sprite_animation seFacingSlope = {};
                seFacingSlope.numFrames = 4;
                seFacingSlope.frames[0].point[0].x = 54;
                seFacingSlope.frames[0].point[0].y = 38;
                seFacingSlope.frames[0].point[1].x = 54 + 6;
                seFacingSlope.frames[0].point[1].y = 38 + 6;
                sprite_animation neFacingSlope = {};
                neFacingSlope.numFrames = 4;
                neFacingSlope.frames[0].point[0].x = 42;
                neFacingSlope.frames[0].point[0].y = 38;
                neFacingSlope.frames[0].point[1].x = 42 + 6;
                neFacingSlope.frames[0].point[1].y = 38 + 6;
                sprite_animation swFacingSlope = {};
                swFacingSlope.numFrames = 4;
                swFacingSlope.frames[0].point[0].x = 60;
                swFacingSlope.frames[0].point[0].y = 38;
                swFacingSlope.frames[0].point[1].x = 60 + 6;
                swFacingSlope.frames[0].point[1].y = 38 + 6;

                for (int i = 1; i < 4; ++i)
                {
                    mainBlock.frames[i] = mainBlock.frames[0];
                    mainBlock.frames[i].point[0].y = mainBlock.frames[0].point[0].y - 6*i;
                    mainBlock.frames[i].point[1].y = mainBlock.frames[0].point[1].y - 6*i;

                    nwFacingSlope.frames[i] = nwFacingSlope.frames[i-1];
                    nwFacingSlope.frames[i].point[0].y = nwFacingSlope.frames[0].point[0].y - 6*i;
                    nwFacingSlope.frames[i].point[1].y = nwFacingSlope.frames[0].point[1].y - 6*i;

                    seFacingSlope.frames[i] = seFacingSlope.frames[i-1];
                    seFacingSlope.frames[i].point[0].y = seFacingSlope.frames[0].point[0].y - 6*i;
                    seFacingSlope.frames[i].point[1].y = seFacingSlope.frames[0].point[1].y - 6*i;

                    neFacingSlope.frames[i] = neFacingSlope.frames[i-1];
                    neFacingSlope.frames[i].point[0].y = neFacingSlope.frames[0].point[0].y - 6*i;
                    neFacingSlope.frames[i].point[1].y = neFacingSlope.frames[0].point[1].y - 6*i;

                    swFacingSlope.frames[i] = swFacingSlope.frames[i-1];
                    swFacingSlope.frames[i].point[0].y = swFacingSlope.frames[0].point[0].y - 6*i;
                    swFacingSlope.frames[i].point[1].y = swFacingSlope.frames[0].point[1].y - 6*i;
                }

                for (int i = 0; i < 4; ++i)
                {
                    float shieldX = i / 2.0f - screenEdge / 2 - shieldW * 3;
                    float shieldY = -0.5f;

                    shields[i][0].x = shieldX;
                    shields[i][0].y = shieldY;
                    shields[i][0].width = shieldW;
                    shields[i][0].height = shieldW;
                    shields[i][0].anim = mainBlock;

                    shields[i][1].x = shieldX + shieldW;
                    shields[i][1].y = shieldY;
                    shields[i][1].width = shieldW;
                    shields[i][1].height = shieldW;
                    shields[i][1].anim = mainBlock;

                    shields[i][2].x = shieldX + shieldW;
                    shields[i][2].y = shieldY + shieldW;
                    shields[i][2].width = shieldW;
                    shields[i][2].height = shieldW;
                    shields[i][2].anim = mainBlock;

                    shields[i][3].x = shieldX;
                    shields[i][3].y = shieldY - shieldW;
                    shields[i][3].width = shieldW;
                    shields[i][3].height = shieldW;
                    shields[i][3].anim = mainBlock;

                    shields[i][4].x = shieldX + 2 * shieldW;
                    shields[i][4].y = shieldY;
                    shields[i][4].width = shieldW;
                    shields[i][4].height = shieldW;
                    shields[i][4].anim = mainBlock;

                    shields[i][5].x = shieldX + 2 * shieldW;
                    shields[i][5].y = shieldY + shieldW;
                    shields[i][5].width = shieldW;
                    shields[i][5].height = shieldW;
                    shields[i][5].anim = mainBlock;

                    shields[i][6].x = shieldX + 3 * shieldW;
                    shields[i][6].y = shieldY - shieldW;
                    shields[i][6].width = shieldW;
                    shields[i][6].height = shieldW;
                    shields[i][6].anim = mainBlock;

                    shields[i][7].x = shieldX + 3 * shieldW;
                    shields[i][7].y = shieldY;
                    shields[i][7].width = shieldW;
                    shields[i][7].height = shieldW;
                    shields[i][7].anim = mainBlock;

                    shields[i][8].x = shieldX;
                    shields[i][8].y = shieldY + shieldW;
                    shields[i][8].width = shieldW;
                    shields[i][8].height = shieldW;
                    shields[i][8].anim = nwFacingSlope;

                    shields[i][9].x = shieldX + shieldW;
                    shields[i][9].y = shieldY - shieldW;
                    shields[i][9].width = shieldW;
                    shields[i][9].height = shieldW;
                    shields[i][9].anim = seFacingSlope;

                    shields[i][10].x = shieldX + 3 * shieldW;
                    shields[i][10].y = shieldY + shieldW;
                    shields[i][10].width = shieldW;
                    shields[i][10].height = shieldW;
                    shields[i][10].anim = neFacingSlope;

                    shields[i][11].x = shieldX + 2 * shieldW;
                    shields[i][11].y = shieldY - shieldW;
                    shields[i][11].width = shieldW;
                    shields[i][11].height = shieldW;
                    shields[i][11].anim = swFacingSlope;

                    for (int j = 0; j < 12; ++j)
                    {
                        shields[i][j].health = 4;
                    }
                }

                shieldInit = true;
            }

            static const int numenemysW = 11;
            static const int numenemysH = 5;
            static entity_info enemies[numenemysW][numenemysH] = {};

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
                        entity_info b;
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

                        b.anim.numFrames = 2;
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
                    }
                }
                enemyInit = true;
            }

            if (GetAsyncKeyState('D') & 0x8000)
            {
                x1 += 1.0f / 60;
                if (x1 > screenEdge)
                    x1 = screenEdge;
            }
            if (GetAsyncKeyState('A') & 0x8000)
            {
                x1 -= 1.0f / 60;
                if (x1 < -screenEdge)
                    x1 = -screenEdge;
            }

            static struct projectile
            {
                float x;
                float y;
                float w = 0.015f;
                float h = w * 3;
                float trajX = 0;
                float trajY = 1.0f / 1.414f;
                bool active = false;
            } ball;

            static int enemyAdvanceRate = 60;
            static float xEnemyShift = enemies[0][0].width / 8.0f;
            static bool shiftX = true;
            static float yEnemyShift = 0.0f;
            bool shiftEnemiesThisFrame = false;
            static int aniFrame = 0;
            if (frameCount % enemyAdvanceRate == 0)
            {
                aniFrame = (aniFrame == 0) ? 1 : 0;
                static unsigned int soundFramePitch = 0;
                xa.Play(1.0f + soundFramePitch / 12.0f);
                if (soundFramePitch >= 3)
                {
                    soundFramePitch = 0;
                }
                else
                {
                    soundFramePitch++;
                }

                // find furthest alive enemy
                //  if that guy reaches end of screen, then shift down and reverse direction
                float leftmostX = 0.0f;
                float rightmostX = 0.0f;
                for (int x = 0; x < numenemysW; ++x)
                {
                    entity_info e = enemies[x][0];
                    if (!e.alive)
                    {
                        for (int y = 0; y < numenemysH; ++y)
                        {
                            entity_info e2 = enemies[x][y];
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
                if ((rightmostX >= screenEdge - enemies[0][0].width / 2 || leftmostX <= -screenEdge + enemies[0][0].width / 2) && yEnemyShift == 0)
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
                             0, 0, 2 * screenEdge, 2)) // test ball against screen
                {
                    for (int x = 0; x < numenemysW; ++x)
                    {
                        for (int y = 0; y < numenemysH; ++y)
                        {
                            entity_info b = enemies[x][y];
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

                    for (int i = 0; i < 4; ++i)
                    {
                        for (int j = 0; j < 3 * 4; ++j)
                        {
                            entity_info s = shields[i][j];
                            bool result = AABBTest(ball.x, ball.y, ball.w, ball.h,
                                                   s.x, s.y, s.width, s.height);
                            if (s.alive && result)
                            {
                                shields[i][j].health--;
                                if (shields[i][j].health <= 0)
                                {
                                    shields[i][j].alive = false;
                                }
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

            float shieldW = (enemyDimScale * 0.5f) * 0.75f;

            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 12; ++j)
                {
                    entity_info shield = shields[i][j];

                    if (shield.alive)
                    {
                        sprite_animation anim = shield.anim;
                        clip_rect currentFrame = anim.frames[4 - shield.health];

                        renderer.DrawGameTextureRect(shield.x, shield.y, shield.width, shield.height, 0,
                                                     currentFrame.point[0].x, currentFrame.point[0].y,
                                                     currentFrame.point[1].x, currentFrame.point[1].y);
                    }
                }
            }

            for (int x = 0; x < numenemysW; ++x)
            {
                for (int y = 0; y < numenemysH; ++y)
                {
                    entity_info enemy = enemies[x][y];
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