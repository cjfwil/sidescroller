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

#include "src/Win32FileIO.cpp"

#include "src/XAudioRenderer.hpp"

struct v2
{
    float x = 0.0f;
    float y = 0.0f;

    v2() = default;

    v2(float v_x, float v_y)
    {
        x = v_x;
        y = v_y;
    }

    inline v2 operator+(v2 v)
    {
        v2 u = v2(x + v.x, y + v.y);
        return u;
    }

    inline v2 operator*(float t)
    {
        v2 u = v2(x * t, y * t);
        return u;
    }

    inline v2 operator*(v2 v)
    {
        v2 u = v2(x * v.x, y * v.y);
        return u;
    }

    bool equals(v2 v)
    {
        return ((x == v.x) && (y == v.y));
    }

    bool within_radius(float r, v2 v)
    {
        float x_ = x - v.x;
        float y_ = y - v.y;
        return (x_ * x_ + y_ * y_ <= r * r);
    }

    void normalise(void)
    {
        float mag = sqrtf(x * x + y * y);
        if (mag)
        {
            x = x / mag;
            y = y / mag;
        }
    }

    v2 lerp_to(v2 v, float t)
    {
        v2 u = v2(x, y) + v2(v.x - x, v.y - y) * t;
        return u;
    }

    float dot(v2 v)
    {
        return (x * v.x + y * v.y);
    }
};

// TODO: allocate tile memory properly so we can get 512 chunks
// static const float globalTileWidth = 32.0f / 768.0f * 3.0f; // 32 pixels for 768 height window
static const float globalTileWidth = 1 / 9.0f;
static const int unsigned globalChunkHeightInTiles = 18;                // in tiles
static const int unsigned globalChunkWidthInTiles = 18 * (4.0f / 3.0f); // in tiles
static float globalChunkWidthInUnits = globalChunkWidthInTiles * globalTileWidth;
static float globalChunkHeightInUnits = globalChunkHeightInTiles * globalTileWidth;

static inline v2 GetChunkCentreWorldPos(int i, int j)
{
    float xp = (i * globalChunkWidthInUnits) + globalTileWidth / 2.0f;
    float yp = (j * globalChunkHeightInUnits) + globalTileWidth / 2.0f;
    return v2(xp, yp);
}

// Gets bottom left hand corner of chunk in world coordinates
static inline v2 GetChunkCornerWorldPos(int i, int j)
{
    float xp = -globalChunkWidthInUnits / 2.0f;
    float yp = -globalChunkHeightInUnits / 2.0f;
    return (v2(xp, yp) + GetChunkCentreWorldPos(i, j));
}

static inline bool AABBTest(float x1, float y1, float w1, float h1,
                            float x2, float y2, float w2, float h2)
{
    return (fabs(x2 - x1) <= ((w1 + w2) / 2.0f) &&
            fabs(y2 - y1) <= ((h1 + h2) / 2.0f));
}

bool AABBTest(v2 v_1, float w1, float h1,
              v2 v_2, float w2, float h2)
{
    return (AABBTest(v_1.x, v_1.y, w1, h1,
                     v_2.x, v_2.y, w2, h2));
}

struct aabb
{
    v2 pos;
    float w;
    float h;

    bool overlaps(aabb box)
    {
        return (AABBTest(pos, w, h, box.pos, box.w, box.h));
    }
};

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Win32Window &window = Win32Window::GetInstance();

    D3D11Renderer renderer = D3D11Renderer(window.hwnd);
    renderer.LoadTextures(TRUE, TRUE);

    XAudioRenderer xa = XAudioRenderer();

    ShowWindow(window.hwnd, SW_SHOWDEFAULT);

    unsigned int frameCount = 0;
    static float deltaTime = 0;

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

            // make clicking better, get keyboard messages
            // text rendering improve
            // more data in tilemap for game collision???
            // maybe second map which brings up translucent image of green squares where collisions are, overlayed on tile editing mode????

            // maybe mario game???

            // switch to array of 2d textures

            // background music

            // metroid style game:
            // static enemy
            // walking back and forth enemy between 2 points
            // enemy that falls off ledge
            // enemy that crawls around walls
            // enemy that walks between N points back and forth
            // enemy that chases player

            // render player as texture

            // assets req
            //  bullets ?
            //  explode
            // character texture and animations

            static float x1_ = 0.0f;
            static float y1_ = 0.0f;
            float paddleWidth_ = 0.09f;
            float paddleHeight_ = 0.16f;
            float enemyDimScale = 0.18f;
            float enemyWidth = 1 * enemyDimScale;
            float enemyHeight = 1 * 8.0 / 12.0f * enemyDimScale;

            static struct
            {
                v2 pos = v2(globalChunkWidthInUnits * 4, globalChunkHeightInUnits * 4); // centre chunk
                v2 velocity;
                float camH = 2.0f + 0.1f; // plus globalTileWidth/2
                float camW = camH;
            } main_camera;
            main_camera.camW = main_camera.camH * (float)window.width / (float)window.height;

            struct clip_rect
            {
                v2 point[2];
            };

            // we define a sprite animation as just a series of rectangles and a framerate
            static const int spriteAnimMaxFrames = 8;
            struct sprite_animation
            {
                // int frameRate = 60;
                int numFrames = 0;
                clip_rect frames[spriteAnimMaxFrames];
            };

            float screenEdge = 4 / 3.0f;

            struct entity_info
            {
                v2 pos = {};
                v2 velocity = {};
                float width;
                float height;
                unsigned int index;
                bool alive = true;
                unsigned int health;
                unsigned char points = 1;

                sprite_animation anim = {};
            };

            // TODO: Get rid of these as constants and make them adjustable
            static const int chunkNumX = 9;
            static const int chunkNumY = 9;
            static entity_info player = {GetChunkCentreWorldPos(chunkNumX / 2, chunkNumY / 2), v2(0, 0), paddleWidth_, paddleHeight_};

            static entity_info shields[4][3 * 4] = {};

            static entity_info ufo = {};
            ufo.height = 8 / 17.0f * enemyDimScale;
            ufo.width = 1 * enemyDimScale;

            static bool shieldInit = false;
            if (!shieldInit)
            {
                float shieldW = (enemyDimScale * 0.5f) * 0.75f;

                // todo, helper functions for creating this data
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
                    mainBlock.frames[i].point[0].y = mainBlock.frames[0].point[0].y - 6 * i;
                    mainBlock.frames[i].point[1].y = mainBlock.frames[0].point[1].y - 6 * i;

                    nwFacingSlope.frames[i] = nwFacingSlope.frames[i - 1];
                    nwFacingSlope.frames[i].point[0].y = nwFacingSlope.frames[0].point[0].y - 6 * i;
                    nwFacingSlope.frames[i].point[1].y = nwFacingSlope.frames[0].point[1].y - 6 * i;

                    seFacingSlope.frames[i] = seFacingSlope.frames[i - 1];
                    seFacingSlope.frames[i].point[0].y = seFacingSlope.frames[0].point[0].y - 6 * i;
                    seFacingSlope.frames[i].point[1].y = seFacingSlope.frames[0].point[1].y - 6 * i;

                    neFacingSlope.frames[i] = neFacingSlope.frames[i - 1];
                    neFacingSlope.frames[i].point[0].y = neFacingSlope.frames[0].point[0].y - 6 * i;
                    neFacingSlope.frames[i].point[1].y = neFacingSlope.frames[0].point[1].y - 6 * i;

                    swFacingSlope.frames[i] = swFacingSlope.frames[i - 1];
                    swFacingSlope.frames[i].point[0].y = swFacingSlope.frames[0].point[0].y - 6 * i;
                    swFacingSlope.frames[i].point[1].y = swFacingSlope.frames[0].point[1].y - 6 * i;
                }

                for (int i = 0; i < 4; ++i)
                {
                    float shieldX = i / 2.0f - screenEdge / 2 - shieldW * 3;
                    float shieldY = -0.5f;

                    shields[i][0].pos.x = shieldX;
                    shields[i][0].pos.y = shieldY;
                    shields[i][0].width = shieldW;
                    shields[i][0].height = shieldW;
                    shields[i][0].anim = mainBlock;

                    shields[i][1].pos.x = shieldX + shieldW;
                    shields[i][1].pos.y = shieldY;
                    shields[i][1].width = shieldW;
                    shields[i][1].height = shieldW;
                    shields[i][1].anim = mainBlock;

                    shields[i][2].pos.x = shieldX + shieldW;
                    shields[i][2].pos.y = shieldY + shieldW;
                    shields[i][2].width = shieldW;
                    shields[i][2].height = shieldW;
                    shields[i][2].anim = mainBlock;

                    shields[i][3].pos.x = shieldX;
                    shields[i][3].pos.y = shieldY - shieldW;
                    shields[i][3].width = shieldW;
                    shields[i][3].height = shieldW;
                    shields[i][3].anim = mainBlock;

                    shields[i][4].pos.x = shieldX + 2 * shieldW;
                    shields[i][4].pos.y = shieldY;
                    shields[i][4].width = shieldW;
                    shields[i][4].height = shieldW;
                    shields[i][4].anim = mainBlock;

                    shields[i][5].pos.x = shieldX + 2 * shieldW;
                    shields[i][5].pos.y = shieldY + shieldW;
                    shields[i][5].width = shieldW;
                    shields[i][5].height = shieldW;
                    shields[i][5].anim = mainBlock;

                    shields[i][6].pos.x = shieldX + 3 * shieldW;
                    shields[i][6].pos.y = shieldY - shieldW;
                    shields[i][6].width = shieldW;
                    shields[i][6].height = shieldW;
                    shields[i][6].anim = mainBlock;

                    shields[i][7].pos.x = shieldX + 3 * shieldW;
                    shields[i][7].pos.y = shieldY;
                    shields[i][7].width = shieldW;
                    shields[i][7].height = shieldW;
                    shields[i][7].anim = mainBlock;

                    shields[i][8].pos.x = shieldX;
                    shields[i][8].pos.y = shieldY + shieldW;
                    shields[i][8].width = shieldW;
                    shields[i][8].height = shieldW;
                    shields[i][8].anim = nwFacingSlope;

                    shields[i][9].pos.x = shieldX + shieldW;
                    shields[i][9].pos.y = shieldY - shieldW;
                    shields[i][9].width = shieldW;
                    shields[i][9].height = shieldW;
                    shields[i][9].anim = seFacingSlope;

                    shields[i][10].pos.x = shieldX + 3 * shieldW;
                    shields[i][10].pos.y = shieldY + shieldW;
                    shields[i][10].width = shieldW;
                    shields[i][10].height = shieldW;
                    shields[i][10].anim = neFacingSlope;

                    shields[i][11].pos.x = shieldX + 2 * shieldW;
                    shields[i][11].pos.y = shieldY - shieldW;
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
            //

            // TODO: separating tileset
            // - write new shader (allow different shaders to be loaded)?
            // - 2d texture array for tileset???
            // - reduce tile rendering draw calls

            // - tilemap info for collidable or not (maybe last bit of tile index is if collision?)
            // - renable collision with character and rework it

            // ****tilemaps begin****
            // setup tilemap data
            struct tile_data
            {
                // v2 pos; // fill out on creation read only
                // u_char r, g, b;
                // uint8_t visible = 0b1;
                uint8_t tileIndex;

                bool isCollidable()
                {
                    return (tileIndex == 0x1A);
                }
            };

            // TODO: Pull out tilemap stuff into own thing

            struct chunk_info
            {
                // bool init = false;
                // float x = 0.0f, y = 0.0f; // TODO: make this refer to centre of tilemap? currently refers to centre of bottom left tile
                // float tileWidth = 1.0f;
                // bool tileDataAllocated = false;
                tile_data data[globalChunkWidthInTiles][globalChunkHeightInTiles];
            };

            struct tilemap
            {
                // bool initialised = false;
                chunk_info map[chunkNumX][chunkNumY]; // TODO: allocate on heap Virtual Alloc, build chunkNumX, etc.. into struct so it loads it from disk rather than hardcoded

                // TODO: load from some kind of on disk data? CSV file?
                // void setup()
                // {
                //     if (!initialised)
                //     {

                //         tilemap* tm = (tilemap*)(mapFile.data);

                //         for (int j = 0; j < chunkNumY; ++j)
                //         {
                //             for (int i = 0; i < chunkNumX; ++i)
                //             {

                //             }
                //         }

                //         // generate random map
                //         // for (int j = 0; j < chunkNumY; ++j)
                //         // {
                //         //     for (int i = 0; i < chunkNumX; ++i)
                //         //     {
                //         //         if (!map[i][j].init)
                //         //         {
                //         //             map[i][j].x = -(globalChunkWidthInTiles * globalTileWidth) / 2.0f + (i * globalTileWidth * globalChunkWidthInTiles);
                //         //             map[i][j].y = -(globalChunkHeightInTiles * globalTileWidth) / 2.0f + (j * globalTileWidth * globalChunkHeightInTiles);
                //         //             for (int x = 0; x < globalChunkWidthInTiles; ++x)
                //         //             {
                //         //                 for (int y = 0; y < globalChunkHeightInTiles; ++y)
                //         //                 {
                //         //                     tile_data tile = {};
                //         //                     tile.tileIndex = rand() % 4;
                //         //                     map[i][j].data[x][y] = tile;
                //         //                 }
                //         //             }
                //         //         }
                //         //     }
                //         // }
                //         initialised = true;
                //     }
                // }
            };

            static win32_file mapFile = Win32FileRead("game/map.bin");
            static tilemap mainTilemap = *((tilemap *)mapFile.data);
            // mainTilemap.setup();
            // static tilemap mainTilemap = {};

            POINT p;
            static float mouseX = 0;
            static float mouseY = 0;
            if (GetCursorPos(&p))
            {
                if (ScreenToClient(window.hwnd, &p))
                {
                    // get mouse into world coords
                    mouseX = (p.x / (float)window.width - 0.5f) * 2.0f * 4.0f / 3.0f;
                    mouseY = ((window.height - p.y) / (float)window.height - 0.5f) * 2.0f;
                }
            }

            static bool writeGameMapToDisk = false;
            if (writeGameMapToDisk)
            {
                Win32FileWrite("game/map.bin", &mainTilemap, sizeof(mainTilemap));
                writeGameMapToDisk = false;
            }
            // **** tilemaps setup end ****

            static const int numenemysW = 9;
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
                        b.pos.x = x * b.width + b.width / 2.0f - 1.0f;
                        b.pos.y = y * b.height;
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

                        enemies[x][y] = b;
                    }
                }
                enemyInit = true;
            }

            if (GetAsyncKeyState('D') & 0x8000)
            {
                player.velocity.x = 1.0f / 60;
            }
            else if (GetAsyncKeyState('A') & 0x8000)
            {
                player.velocity.x = -1.0f / 60;
            }
            else
            {
                player.velocity.x = 0;
            }

            // simulate player motion
            // do collision detection here

            static bool playerIsGrounded = false;
            float fallDistance = 0.1f / 60.0f;
            static bool checkForFall = false;
            if (!playerIsGrounded || checkForFall)
            {
                float terminalVelocity = 0.04f;
                player.velocity.y -= fallDistance;
                if (player.velocity.y < -terminalVelocity)
                {
                    player.velocity.y = -terminalVelocity;
                }
                checkForFall = false;
            }
            else
            {
                player.velocity.y = 0;
            }

// jump
#if 0
            if (playerIsGrounded && GetAsyncKeyState('W') & 0x8000)
            {
                player.velocity.y = 2.0f / 60;
                playerIsGrounded = false;
            }

// controls like a jetpack
#else
            if (GetAsyncKeyState('W') & 0x8000)
            {
                player.velocity.y += fallDistance * 1.5f;
                playerIsGrounded = false;
            }
            else if (!playerIsGrounded && GetAsyncKeyState('S') & 0x8000)
            {
                player.velocity.y = -1.0f / 60;
            }
#endif

            v2 nextPlayerPos = player.pos + player.velocity;
            bool noCollisionsX = true;
            bool noCollisionsY = true;

            // test against entire tilemap rectangle first
            // player collision against world map
#if 1
            for (int j = 0; j < chunkNumY; ++j)
            {
                for (int i = 0; i < chunkNumX; ++i)
                {
                    bool interactingWithMap = false;
                    chunk_info m = mainTilemap.map[i][j];
                    v2 chunkPos = GetChunkCornerWorldPos(i, j);
                    if (AABBTest(nextPlayerPos, player.width, player.height,
                                 chunkPos + v2(globalChunkWidthInUnits / 2.0f, globalChunkHeightInUnits / 2.0f), globalChunkWidthInUnits + globalTileWidth, globalChunkHeightInUnits + globalTileWidth))
                    {
                        interactingWithMap = true;
                    }

                    if (interactingWithMap)
                    {
                        // separate axis calculate
                        for (int x = 0; x < globalChunkWidthInTiles; ++x)
                        {
                            for (int y = 0; y < globalChunkHeightInTiles; ++y)
                            {
                                tile_data tile = m.data[x][y];
                                if (tile.isCollidable())
                                {
                                    v2 tilePos = v2(x * globalTileWidth + chunkPos.x, y * globalTileWidth + chunkPos.y);
                                    v2 nextPositionXOnly = v2(nextPlayerPos.x, player.pos.y);
                                    if (AABBTest(nextPositionXOnly, player.width, player.height,
                                                 tilePos, globalTileWidth, globalTileWidth))
                                    {
                                        noCollisionsX = false;
                                        checkForFall = true;
                                    }

                                    v2 nextPositionYOnly = v2(player.pos.x, nextPlayerPos.y);
                                    bool test = AABBTest(nextPositionYOnly, player.width, player.height,
                                                         tilePos, globalTileWidth, globalTileWidth);
                                    if (test)
                                    {
                                        noCollisionsY = false;
                                        // playerIsGrounded = true; // if this then can walk on ceiling ??

                                        if (tilePos.y < nextPositionYOnly.y)
                                        {
                                            playerIsGrounded = true;
                                        }
                                        else
                                        {
                                            player.velocity.y = 0; // stop y velocity when head hits ceiling
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (noCollisionsX)
            {
                player.pos.x += player.velocity.x;
                checkForFall = true;
            }
            if (noCollisionsY)
            {
                player.pos.y += player.velocity.y;
            }
            else
            {
                player.velocity.y = 0;
            }
#endif

#if 1
            // simulate camera motion
            // follow player
            static float t = 0;
            if (main_camera.pos.within_radius(0.01f, player.pos))
            {
                t = 0;
            }
            else
            {
                t += deltaTime / 10000.0f;
                // t += ((deltaTime> 0) ? deltaTime : 0) / 10000.0f;
                if (t > 1 || t < 0)
                {
                    t = 1;
                }
            }
            main_camera.pos = main_camera.pos.lerp_to(player.pos, t);
#else

            // control camera with keys
            if (GetAsyncKeyState(VK_UP) & 0x8000)
            {
                main_camera.pos.y += 1.0f / 60.0f;
            }
            else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
            {
                main_camera.pos.y -= 1.0f / 60.0f;
            }

            if (GetAsyncKeyState(VK_LEFT) & 0x8000)
            {
                main_camera.pos.x -= 1.0f / 60.0f;
            }
            else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
            {
                main_camera.pos.x += 1.0f / 60.0f;
            }
#endif

            static int editingTileX = -1;
            static int editingTileY = -1;
            static int editModeChooseTile = 0x1A;

            if (GetAsyncKeyState(0x30 + 0) & 0x8000)
            {
                editModeChooseTile = 0x11;
            }
            if (GetAsyncKeyState(0x30 + 1) & 0x8000)
            {
                editModeChooseTile = 0x14;
            }

            if (GetAsyncKeyState(0x30 + 3) & 0x8000)
            {
                editModeChooseTile = 0x17;
            }
            if (GetAsyncKeyState(0x30 + 4) & 0x8000)
            {
                editModeChooseTile = 0x1A;
            }

            if (GetAsyncKeyState(0x30 + 5) & 0x8000)
            {
                editModeChooseTile = 0x40;
            }

            // tilemap edit code
            if ((GetKeyState(VK_LBUTTON) & 0x100) != 0)
            {
                v2 mouseInWorld = v2(mouseX, mouseY) + main_camera.pos;
                for (int i = 0; i < chunkNumX; ++i)
                {
                    for (int j = 0; j < chunkNumY; ++j)
                    {
                        v2 chunkPos = GetChunkCornerWorldPos(i, j);
                        v2 chunkCentre = chunkPos + v2(globalChunkWidthInUnits / 2.0f, globalChunkHeightInUnits / 2.0f);

                        if (AABBTest(chunkCentre, globalChunkWidthInUnits, globalChunkHeightInUnits,
                                     mouseInWorld, 0, 0))
                        {
                            chunk_info chunk = mainTilemap.map[i][j];
                            for (int x = 0; x < globalChunkWidthInTiles; ++x)
                            {
                                for (int y = 0; y < globalChunkHeightInTiles; ++y)
                                {

                                    if (AABBTest(chunkPos + v2(x, y) * globalTileWidth, globalTileWidth, globalTileWidth,
                                                 mouseInWorld, 0, 0))
                                    {
                                        chunk.data[x][y].tileIndex = editModeChooseTile;
                                        writeGameMapToDisk = true;
                                    }
                                }
                            }
                            mainTilemap.map[i][j] = chunk;
                        }
                    }
                }
            }

            struct projectile
            {
                float x;
                float y;
                float w = 32.0f / 768.0f * 2.0f;
                float h = 32.0f / 768.0f * 2.0f;
                float trajX = 0;
                float trajY = 1.0f / 1.414f;
                bool active = false;
            };

            static projectile playerProjectile;
            static projectile enemyProjectile;

            static bool gameOver = false;

            static int enemyAdvanceRate = 60;
            static float xEnemyShift = enemies[0][0].width / 8.0f;
            static bool shiftX = true;
            static float yEnemyShift = 0.0f;
            bool shiftEnemiesThisFrame = false;
            static int aniFrame = 0;

            static bool facingRight = false;

            if (frameCount % enemyAdvanceRate == 0)
            {
                aniFrame = (aniFrame == 0) ? 1 : 0;
                static unsigned int soundFramePitch = 0;
                // xa.Play(2, 1.0f + soundFramePitch / 12.0f); // enemy move
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
                                if (e2.pos.x < leftmostX)
                                {
                                    leftmostX = e2.pos.x;
                                }
                                if (e2.pos.x > rightmostX)
                                {
                                    rightmostX = e2.pos.x;
                                }
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (e.pos.x < leftmostX)
                        {
                            leftmostX = e.pos.x;
                        }
                        if (e.pos.x > rightmostX)
                        {
                            rightmostX = e.pos.x;
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
                        {
                            enemies[x][y].pos.x += xEnemyShift;
                        }
                        enemies[x][y].pos.y += yEnemyShift;

                        if (enemies[x][y].pos.y <= player.pos.y && enemies[x][y].alive)
                        {
                            gameOver = true;
                        }
                    }
                }
            }

            if (gameOver)
            {
                // GAME OVER
                enemyInit = false;
                enemyAdvanceRate = 60;
                gameOver = false;
            }

            int enemyFireRate = 1; // every enemyFireRate seconds, fire a bolt from a random enemy
            if (frameCount % (enemyFireRate * 60) == 0 && frameCount != 0)
            {
                entity_info e = enemies[rand() % numenemysW][rand() % numenemysH];
                if (e.alive)
                {
                    enemyProjectile.x = e.pos.x;
                    enemyProjectile.y = e.pos.y;
                    enemyProjectile.active = true;
                    enemyProjectile.trajX = 0;
                    enemyProjectile.trajY = -1;
                    enemyProjectile.w = playerProjectile.w;
                    enemyProjectile.h = playerProjectile.h;
                }
            }

            int ufoAppearRate = 30; // every 30 seconds
            // TODO instead of 60 fps allow arbitrary framerates taking in deltatime
            if (frameCount % (ufoAppearRate * 60) == 0 && frameCount != 0)
            {
                ufo.alive = true;
                ufo.pos.x = screenEdge + ufo.width / 2;
                ufo.pos.y = 1 - ufo.height;
            }
            else if (frameCount == 0)
            {
                ufo.alive = false;
            }

            // simulate ufo
            if (ufo.alive)
            {
                ufo.pos.x += -1.0f / 60 * (1 / 5.0f);

                if (ufo.pos.x < -screenEdge)
                {
                    ufo.alive = false;
                }
            }

            static int score = 0;

            static float speed = 2.5f;

            float soundFreq = speed * 0.4f;
            bool hit = false;

            if (playerProjectile.active)
            {
                playerProjectile.x += playerProjectile.trajX / 60 * speed;
                playerProjectile.y += playerProjectile.trajY / 60 * speed;

                if (AABBTest(v2(playerProjectile.x, playerProjectile.y), playerProjectile.w, playerProjectile.h,
                             main_camera.pos, 2 * screenEdge, 2)) // test playerProjectile against screen
                {
                    int xi = (int)((playerProjectile.x+globalChunkWidthInUnits/2.0f)/globalChunkWidthInUnits);
                    int yi = (int)((playerProjectile.y+globalChunkHeightInUnits/2.0f)/globalChunkHeightInUnits);
                    chunk_info m = mainTilemap.map[xi][yi];
                    for (int x = 0; x < globalChunkWidthInTiles; ++x)
                    {
                        for (int y = 0; y < globalChunkHeightInTiles; ++y)
                        {
                            v2 mp = GetChunkCornerWorldPos(xi,yi) + v2(x*globalTileWidth, y*globalTileWidth);
                            if (m.data[x][y].isCollidable() && AABBTest(v2(playerProjectile.x, playerProjectile.y), playerProjectile.w, playerProjectile.h,
                                                                    mp, globalTileWidth, globalTileWidth)) {
                                playerProjectile.active = false;
                            }
                        }
                    }

                    for (int x = 0; x < numenemysW; ++x)
                    {
                        for (int y = 0; y < numenemysH; ++y)
                        {
                            entity_info b = enemies[x][y];
                            bool result = AABBTest(playerProjectile.x, playerProjectile.y, playerProjectile.w, playerProjectile.h,
                                                   b.pos.x, b.pos.y, b.width, b.height);
                            if (b.alive && result)
                            {
                                score += b.points;
                                soundFreq = speed + (b.points / 100.f);
                                enemies[x][y].alive = false;
                                hit = true;
                                playerProjectile.active = false;
                            }
                        }
                    }

                    for (int i = 0; i < 4; ++i)
                    {
                        for (int j = 0; j < 3 * 4; ++j)
                        {
                            entity_info s = shields[i][j];
                            bool result = AABBTest(playerProjectile.x, playerProjectile.y, playerProjectile.w, playerProjectile.h,
                                                   s.pos.x, s.pos.y, s.width, s.height);
                            if (s.alive && result)
                            {
                                shields[i][j].health--;
                                if (shields[i][j].health <= 0)
                                {
                                    shields[i][j].alive = false;
                                }
                                hit = true;
                                playerProjectile.active = false;
                            }
                        }
                    }

                    if (ufo.alive && AABBTest(playerProjectile.x, playerProjectile.y, playerProjectile.w, playerProjectile.h,
                                              ufo.pos.x, ufo.pos.y, ufo.width, ufo.height))
                    {
                        score += 200;
                        ufo.alive = false;
                        hit = true;
                    }
                }
                else
                {
                    playerProjectile.active = false;
                }
            }
            else
            {
                playerProjectile.x = player.pos.x;
                playerProjectile.y = player.pos.y;
                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                {
                    // xa.Play(1, 1.0f);
                    if (facingRight)
                    {
                        playerProjectile.trajY = 0;
                        playerProjectile.trajX = 1.0f;
                    }
                    else
                    {
                        playerProjectile.trajY = 0;
                        playerProjectile.trajX = -1.0f;
                    }
                    playerProjectile.x = player.pos.x;
                    playerProjectile.y = player.pos.y;
                    playerProjectile.active = true;
                }
            }

            if (enemyProjectile.active)
            {
                enemyProjectile.x += enemyProjectile.trajX / 60 * speed;
                enemyProjectile.y += enemyProjectile.trajY / 60 * speed;
                bool result = AABBTest(enemyProjectile.x, enemyProjectile.y, enemyProjectile.w, enemyProjectile.h,
                                       player.pos.x, player.pos.y, player.width, player.height);
                if (result)
                {
                    hit = true;
                    score -= 50;
                    enemyProjectile.active = false;
                    if (score <= 0)
                    {
                        score = 0;
                        gameOver = true;
                    }
                }
                else if (!AABBTest(enemyProjectile.x, enemyProjectile.y, enemyProjectile.w, enemyProjectile.h,
                                   0, 0, 2 * screenEdge, 2))
                { // test enemyProjectile against screen
                    enemyProjectile.active = false;
                    enemyProjectile.x = 0;
                    enemyProjectile.y = 0;
                }
                else
                {
                    for (int i = 0; i < 4; ++i)
                    {
                        for (int j = 0; j < 12; ++j)
                        {
                            entity_info shield = shields[i][j];

                            if (shield.alive && AABBTest(enemyProjectile.x, enemyProjectile.y, enemyProjectile.w, enemyProjectile.h,
                                                         shield.pos.x, shield.pos.y, shield.width, shield.height))
                            {
                                shields[i][j].health--;
                                // TODO this is repeating so pull this out into "simulate shields"
                                if (shields[i][j].health <= 0)
                                {
                                    shields[i][j].alive = false;
                                }
                                hit = true;
                                enemyProjectile.active = false;
                                break;
                            }
                        }
                    }
                }
            }

            if (hit)
            {
                // xa.Play(1, soundFreq);
                hit = false;
            }

            score = fabs(player.velocity.y * 10000);

            // draw game
            constantBufferData.view[0] = (float)window.height / (float)window.width;
            texRenderConstantBufferData.view[0] = (float)window.height / (float)window.width;
            renderer.StartDraw(0, 0, 0);

            // draw tilemap

            // camera AABB test against chunks

            // start tilemap drawing
            // TODO: devise method for zooming in and out? baking chunks? greedy meshing?
            int startDrawX = max((main_camera.pos.x - main_camera.camW / 2.0f) / (globalChunkWidthInUnits), 0);
            int startDrawY = max((main_camera.pos.y - main_camera.camH / 2.0f) / (globalChunkHeightInUnits), 0);

            int endDrawX = max((main_camera.pos.x + main_camera.camW / 2.0f) / (globalChunkWidthInUnits) + 2, 0);
            int endDrawY = max((main_camera.pos.y + main_camera.camH / 2.0f) / (globalChunkHeightInUnits) + 2, 0);

            static bool forceDrawEntireMap = true; // draw full map, less bug prone, use to check for issues
            if (forceDrawEntireMap)
            {
                startDrawX = 0;
                startDrawY = 0;
                endDrawX = chunkNumX;
                endDrawY = chunkNumY;
            }

            for (int j = startDrawY; j < endDrawY; ++j)
            {
                for (int i = startDrawX; i < endDrawX; ++i)
                {
                    float w = globalChunkWidthInUnits;
                    float h = globalChunkHeightInUnits;
                    v2 chunkPos = GetChunkCornerWorldPos(i, j);
                    chunk_info m = mainTilemap.map[i][j];

                    bool test_success = AABBTest(chunkPos + v2(w / 2.0f, h / 2.0f), w, h,
                                                 main_camera.pos, main_camera.camW, main_camera.camH);
                    if (test_success)
                    {
                        // render map m
                        for (int x = 0; x < globalChunkWidthInTiles; ++x)
                        {
                            for (int y = 0; y < globalChunkHeightInTiles; ++y)
                            {
                                tile_data tile = m.data[x][y];

                                v2 tilePos = chunkPos + (v2(x, y) * globalTileWidth);
                                test_success = AABBTest(tilePos, globalTileWidth, globalTileWidth,
                                                        main_camera.pos, main_camera.camW, main_camera.camH);

                                // TODO: if edit mode
                                if (AABBTest(mouseX + main_camera.pos.x, mouseY + main_camera.pos.y, 0, 0, tilePos.x, tilePos.y, globalTileWidth, globalTileWidth))
                                {
                                    renderer.DrawTile(tilePos.x - main_camera.pos.x, tilePos.y - main_camera.pos.y, globalTileWidth, globalTileWidth, editModeChooseTile);
                                    test_success = false;
                                }
                                if (test_success /* && tile.isCollidable()*/)
                                {
                                    // TODO: draw without alpha
                                    renderer.DrawTile(tilePos.x - main_camera.pos.x, tilePos.y - main_camera.pos.y, globalTileWidth, globalTileWidth, tile.tileIndex);
                                }
                            }
                        }
                    }
                }
            }
            // end of tilemap drawing

            // renderer.DrawRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y, player.width, player.height);

            if (player.velocity.x != 0)
                facingRight = (player.velocity.x > 0);
            if (player.velocity.y != 0)
            {
                if (facingRight)
                {
                    renderer.DrawGameTextureRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y + 0.04f, 48.0f / 768.0f * 4.0f, 48.0f / 768.0f * 4.0f, 0, 0, 432 - 383, 48, 432 - 336);
                }
                else
                {
                    renderer.DrawGameTextureRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y + 0.04f, 48.0f / 768.0f * 4.0f, 48.0f / 768.0f * 4.0f, 0, 96, 432 - 383, 96 + 48, 432 - 336);
                }
            }
            else if (player.velocity.x != 0)
            {
                if (facingRight)
                {
                    renderer.DrawGameTextureRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y + 0.04f, 48.0f / 768.0f * 4.0f, 48.0f / 768.0f * 4.0f, 0, 0, 432 - 192, 48, 432 - 144, (int)(frameCount / 4.83f) % 10);
                }
                else
                {
                    renderer.DrawGameTextureRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y + 0.04f, 48.0f / 768.0f * 4.0f, 48.0f / 768.0f * 4.0f, 0, 0, 0, 48, 48, (int)(frameCount / 4.83f) % 10);
                }
            }
            else if (player.velocity.y == 0 && player.velocity.x == 0) // idle
            {
                if (facingRight)
                {
                    renderer.DrawGameTextureRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y + 0.04f, 48.0f / 768.0f * 4.0f, 48.0f / 768.0f * 4.0f, 0, 0, 432 - 143, 48, 432 - 99, (int)(frameCount / 13.95f) % 4);
                }
                else
                {
                    renderer.DrawGameTextureRect(player.pos.x - main_camera.pos.x, player.pos.y - main_camera.pos.y + 0.04f, 48.0f / 768.0f * 4.0f, 48.0f / 768.0f * 4.0f, 0, 288, 432 - 143, 288 + 48, 432 - 99, (int)(frameCount / 13.95f) % 4);
                }
            }

            if (playerProjectile.active)
            {
                // renderer.DrawRect(playerProjectile.x - main_camera.pos.x, playerProjectile.y - main_camera.pos.y, playerProjectile.w, playerProjectile.h);
                renderer.DrawGameTextureRect(playerProjectile.x - main_camera.pos.x, playerProjectile.y - main_camera.pos.y, playerProjectile.w, playerProjectile.h, 0, 0, 15, 15, 31, 0, 1, NULL);
            }
            // if (enemyProjectile.active)
            //     renderer.DrawRect(enemyProjectile.x - main_camera.pos.x, enemyProjectile.y - main_camera.pos.y, enemyProjectile.w, enemyProjectile.h);

            if (ufo.alive)
                renderer.DrawGameTextureRect(ufo.pos.x - main_camera.pos.x, ufo.pos.y - main_camera.pos.y, ufo.width, ufo.height, 0, 0, 127 - 8, 17, 127);

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

                        renderer.DrawGameTextureRect(shield.pos.x - main_camera.pos.x, shield.pos.y - main_camera.pos.y, shield.width, shield.height, 0,
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

                        // renderer.DrawGameTextureRect(enemy.pos.x - main_camera.pos.x, enemy.pos.y - main_camera.pos.y, enemy.width, enemy.height, 0,
                        //  x1, y1, x2, y2);
                        renderer.DrawRect(enemy.pos.x - main_camera.pos.x, enemy.pos.y - main_camera.pos.y, enemy.width, enemy.height);
                    }
                }
            }

            float chScale = 0.1f;
            float chW = chScale * 4.0f / 6.0f;
            float chH = chScale;

            renderer.DrawFontRect(-1.05f, 0.66667f, score % 10, chW, chH);
            renderer.DrawFontRect(-1.05f - chW, 0.66667f, (score / 10) % 10, chW, chH);
            renderer.DrawFontRect(-1.05f - 2 * chW, 0.66667f, (score / 100) % 1000, chW, chH);

            // framerate render
            static LARGE_INTEGER freq;
            if (!freq.QuadPart)
            {
                QueryPerformanceFrequency(&freq); // TODO: don't do this every frame
            }
            static LARGE_INTEGER perfCount, perfCountDifference = {};
            LARGE_INTEGER lastPerfCount = perfCount;
            QueryPerformanceCounter(&perfCount);
            perfCountDifference.QuadPart = perfCount.QuadPart - lastPerfCount.QuadPart;
            perfCountDifference.QuadPart *= 1000000LL;
            perfCountDifference.QuadPart /= freq.QuadPart;

            deltaTime = (perfCountDifference.QuadPart / 1000.0f);

            static unsigned int frameRate = 0;
            static unsigned int lastFrameCount = 0;
            if (frameCount - lastFrameCount > 20)
            { // only update framerate counter every 20 frames
                frameRate = 1000000.0f / (float)(perfCountDifference.QuadPart);
                lastFrameCount = frameCount;
            }

            // TODO: render number as text helper function
            renderer.DrawFontRect(1.05f, 0.86667f, frameRate % 10, chW, chH);
            renderer.DrawFontRect(1.05f - chW, 0.86667f, (frameRate / 10) % 10, chW, chH);
            renderer.DrawFontRect(1.05f - 2 * chW, 0.86667f, (frameRate / 100) % 1000, chW, chH);

            // cursor
            // renderer.DrawRect(mouseX, mouseY, 0.1f, 0.1f);

            frameCount++;
            renderer.pSwapChain->Present(1, 0);
        }
    }
    return (0);
}