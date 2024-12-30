// C
#include <cmath>
#include <cstdlib>

// Cpp
#include <iostream>
#include <string>

// Third Party
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// Users
const int         screenWidth  = 800;
const int         screenHeight = 450;
const int         targetFPS    = 120;
const std::string raymarchShaderPath =
    "E:/Study/CodeProj/PlayGround/Source/Raylib/Shader/hybrid_raymarch.fs";
const std::string rasterShaderPath =
    "E:/Study/CodeProj/PlayGround/Source/Raylib/Shader/hybrid_raster.fs";

// Marco
#define PLATFORM_DESKTOP

#if defined(PLATFORM_DESKTOP)
#    define GLSL_VERSION 460
#else  // PLATFORM_ANDROID, PLATFORM_WEB
#    define GLSL_VERSION 100
#endif

// Global Variables

struct RayLocs
{
    unsigned int camPos, camDir, screenCenter;
};

/**
 * @brief Load custom render texture, create a writable depth texture buffer
 *
 * @param width
 * @param height
 * @return RenderTexture2D
 */
static RenderTexture2D LoadRenderTextureDepthTex(int width, int height)
{
    RenderTexture2D target{0};
    target.id = rlLoadFramebuffer();  // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);  // 这个相当于OpenGL的glBindFramebuffer

        // Create color texture (default to RGBA)
        target.texture.id =
            rlLoadTexture(nullptr, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width   = width;
        target.texture.height  = height;
        target.texture.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture buffer (instead of raylib default renderbuffer)
        target.depth.id      = rlLoadTextureDepth(width, height, false);
        target.depth.width   = width;
        target.depth.height  = height;
        target.depth.format  = 19;  // DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0,
                            RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH,
                            RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id))
        {
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);
        }

        rlDisableFramebuffer();  // 解绑
    }
    else { TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created"); }

    return target;
}

/**
 * @brief Unload render texture from GPU memory (VRAM)
 *
 * @param target
 */
static void UnloadRenderTextureDepthTex(RenderTexture2D target)
{
    if (target.id > 0)
    {
        // Color texture attached to FBO is deleted
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);

        // NOTE: Depth texture is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}

int main(int argc, char** argv)
{
    InitWindow(800, 450, "raylib [core] example - basic window");

    Shader shdrRaymarch = LoadShader(nullptr, raymarchShaderPath.c_str());
    Shader shdrRaster   = LoadShader(nullptr, rasterShaderPath.c_str());

    RayLocs marchLocs{
        .camPos       = static_cast<unsigned int>(GetShaderLocation(shdrRaymarch, "camPos")),
        .camDir       = static_cast<unsigned int>(GetShaderLocation(shdrRaymarch, "camDir")),
        .screenCenter = static_cast<unsigned int>(GetShaderLocation(shdrRaymarch, "screenCenter")),
    };

    Vector2 screenCenter(screenWidth / 2.0f, screenHeight / 2.0f);
    SetShaderValue(shdrRaymarch, marchLocs.screenCenter, &screenCenter, SHADER_UNIFORM_VEC2);

    RenderTexture2D target = LoadRenderTextureDepthTex(screenWidth, screenHeight);

    Camera camera{
        .position   = Vector3{0.5f, 1.0f, 1.5f},  // Camera position
        .target     = Vector3{0.0f, 0.5f, 0.0f},  // Camera looking at point
        .up         = Vector3{0.0f, 1.0f, 0.0f},  // Camera up vector (rotation towards target)
        .fovy       = 45.0f,                      // Camera field-of-view Y
        .projection = CAMERA_PERSPECTIVE          // Camera projection type
    };
    float camDist = 1.0f / (tanf(camera.fovy * 0.5f * DEG2RAD));

    SetTargetFPS(targetFPS);
    while (!WindowShouldClose())
    {
        // update
        UpdateCamera(&camera, CameraMode::CAMERA_ORBITAL);
        SetShaderValue(shdrRaymarch, marchLocs.camPos, &(camera.position), RL_SHADER_UNIFORM_VEC3);
        Vector3 camDir = Vector3Scale(
            Vector3Normalize(Vector3Subtract(camera.target, camera.position)), camDist);
        SetShaderValue(shdrRaymarch, marchLocs.camDir, &(camDir), RL_SHADER_UNIFORM_VEC3);

        // Draw into our custom render texture (framebuffer)
        BeginTextureMode(target);
        ClearBackground(WHITE);
        {
            // Raymarch Scene
            rlEnableDepthTest();
            BeginShaderMode(shdrRaymarch);
            DrawRectangleRec(Rectangle{0, 0, (float)screenWidth, (float)screenHeight}, WHITE);
            EndShaderMode();

            // Rasterize Scene
            // BeginMode3D(camera);
            // BeginShaderMode(shdrRaster);
            // {
            //     DrawCubeWiresV(Vector3{0.0f, 0.5f, 1.0f}, Vector3{1.0f, 1.0f, 1.0f}, RED);
            //     DrawCubeV(Vector3{0.0f, 0.5f, 1.0f}, Vector3{1.0f, 1.0f, 1.0f}, PURPLE);
            //     DrawCubeWiresV(Vector3{0.0f, 0.5f, -1.0f}, Vector3{1.0f, 1.0f, 1.0f}, DARKGREEN);
            //     DrawCubeV(Vector3{.0f, 0.5f, -1.0f}, Vector3{1.0f, 1.0f, 1.0f}, YELLOW);
            //     DrawGrid(10, 1.0f);
            // }
            // EndShaderMode();
            // EndMode3D();
        }
        EndTextureMode();

        // Draw into screen our custom render texture
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTextureRec(target.texture, Rectangle{0, 0, (float)screenWidth, (float)-screenHeight},
                       Vector2{0, 0}, WHITE);  // 设置视口
        DrawFPS(10, 10);
        EndDrawing();
    }

    // clean up
    UnloadRenderTextureDepthTex(target);
    UnloadShader(shdrRaymarch);
    UnloadShader(shdrRaster);

    CloseWindow();
    return EXIT_SUCCESS;
}
