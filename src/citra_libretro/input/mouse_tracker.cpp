// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include <memory>

#include "citra_libretro/core_settings.h"
#include "citra_libretro/environment.h"
#include "citra_libretro/input/mouse_tracker.h"
#include "common/settings.h"
#include "core/frontend/framebuffer_layout.h"

#ifdef ENABLE_OPENGL
#include <glad/glad.h>
#include "video_core/shader/generator/glsl_shader_gen.h"
#endif

#ifdef ENABLE_VULKAN
#include "core/core.h"
#include "video_core/gpu.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#endif

namespace LibRetro {

namespace Input {

/// Shared cursor coordinate calculation
struct CursorCoordinates {
    float centerX, centerY;
    float renderWidth, renderHeight;
    float boundingLeft, boundingTop, boundingRight, boundingBottom;
    float verticalLeft, verticalRight, verticalTop, verticalBottom;
    float horizontalLeft, horizontalRight, horizontalTop, horizontalBottom;

    CursorCoordinates(int bufferWidth, int bufferHeight, float projectedX, float projectedY,
                      float renderRatio, const Layout::FramebufferLayout& layout) {
        // Convert to normalized device coordinates
        centerX = (projectedX / bufferWidth) * 2 - 1;
        centerY = (projectedY / bufferHeight) * 2 - 1;

        renderWidth = renderRatio / bufferWidth;
        renderHeight = renderRatio / bufferHeight;

        boundingLeft = (layout.bottom_screen.left / (float)bufferWidth) * 2 - 1;
        boundingTop = (layout.bottom_screen.top / (float)bufferHeight) * 2 - 1;
        boundingRight = (layout.bottom_screen.right / (float)bufferWidth) * 2 - 1;
        boundingBottom = (layout.bottom_screen.bottom / (float)bufferHeight) * 2 - 1;

        // Calculate cursor dimensions
        verticalLeft = std::fmax(centerX - renderWidth / 5, boundingLeft);
        verticalRight = std::fmin(centerX + renderWidth / 5, boundingRight);
        verticalTop = -std::fmax(centerY - renderHeight, boundingTop);
        verticalBottom = -std::fmin(centerY + renderHeight, boundingBottom);

        horizontalLeft = std::fmax(centerX - renderWidth, boundingLeft);
        horizontalRight = std::fmin(centerX + renderWidth, boundingRight);
        horizontalTop = -std::fmax(centerY - renderHeight / 5, boundingTop);
        horizontalBottom = -std::fmin(centerY + renderHeight / 5, boundingBottom);
    }
};

/// Helper function to check if coordinates are within the touchscreen area
/// (uses the same logic as EmuWindow::IsWithinTouchscreen)
static bool IsWithinTouchscreen(const Layout::FramebufferLayout& layout, unsigned framebuffer_x,
                                unsigned framebuffer_y) {
    // Note: LibRetro doesn't support SeparateWindows, so we can skip that check

    Settings::StereoRenderOption render_3d_mode = Settings::values.render_3d.GetValue();

    if (render_3d_mode == Settings::StereoRenderOption::SideBySide ||
        render_3d_mode == Settings::StereoRenderOption::ReverseSideBySide) {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                ((framebuffer_x >= layout.bottom_screen.left / 2 &&
                  framebuffer_x < layout.bottom_screen.right / 2) ||
                 (framebuffer_x >= (layout.bottom_screen.left / 2) + (layout.width / 2) &&
                  framebuffer_x < (layout.bottom_screen.right / 2) + (layout.width / 2))));
    } else if (render_3d_mode == Settings::StereoRenderOption::CardboardVR) {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                ((framebuffer_x >= layout.bottom_screen.left &&
                  framebuffer_x < layout.bottom_screen.right) ||
                 (framebuffer_x >= layout.cardboard.bottom_screen_right_eye + (layout.width / 2) &&
                  framebuffer_x < layout.cardboard.bottom_screen_right_eye +
                                      layout.bottom_screen.GetWidth() + (layout.width / 2))));
    } else {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                framebuffer_x >= layout.bottom_screen.left &&
                framebuffer_x < layout.bottom_screen.right);
    }
}

MouseTracker::MouseTracker() {
    // Create renderer-specific cursor renderer based on current graphics API
    cursor_renderer = nullptr;
    switch (Settings::values.graphics_api.GetValue()) {
    case Settings::GraphicsAPI::OpenGL:
#ifdef ENABLE_OPENGL
        cursor_renderer = std::make_unique<OpenGLCursorRenderer>();
#endif
        break;
    case Settings::GraphicsAPI::Vulkan:
#ifdef ENABLE_VULKAN
        cursor_renderer = std::make_unique<VulkanCursorRenderer>();
#endif
        break;
    case Settings::GraphicsAPI::Software:
        cursor_renderer = std::make_unique<SoftwareCursorRenderer>();
        break;
    }
}

MouseTracker::~MouseTracker() = default;

void MouseTracker::OnMouseMove(float deltaX, float deltaY) {
    x += deltaX;
    y += deltaY;
}

void MouseTracker::Restrict(float minX, float minY, float maxX, float maxY) {
    x = std::min<float>(std::max<float>(minX, x), maxX);
    y = std::min<float>(std::max<float>(minY, y), maxY);
}

void MouseTracker::Update(int bufferWidth, int bufferHeight,
                          const Layout::FramebufferLayout& layout) {
    bool state = false;

    if (LibRetro::settings.mouse_touchscreen) {
        // Check mouse input
        state |= LibRetro::CheckInput(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);

        // Read in and convert pointer values to absolute values on the canvas
        auto pointerX = LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
        auto pointerY = LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
        auto newX = static_cast<int>((pointerX + 0x7fff) / (float)(0x7fff * 2) * bufferWidth);
        auto newY = static_cast<int>((pointerY + 0x7fff) / (float)(0x7fff * 2) * bufferHeight);

        // Use mouse pointer movement
        if ((pointerX != 0 || pointerY != 0) && (newX != lastMouseX || newY != lastMouseY)) {
            lastMouseX = newX;
            lastMouseY = newY;

            // Use layout system to validate and map coordinates
            if (IsWithinTouchscreen(layout, newX, newY)) {
                x = std::max<float>(static_cast<int>(layout.bottom_screen.left),
                             std::min<float>(newX, static_cast<int>(layout.bottom_screen.right))) -
                    layout.bottom_screen.left;
                y = std::max<float>(static_cast<int>(layout.bottom_screen.top),
                             std::min<float>(newY, static_cast<int>(layout.bottom_screen.bottom))) -
                    layout.bottom_screen.top;
            }
        }
    }

    if (LibRetro::settings.touch_touchscreen) {
        // Check touchscreen input
        state |= LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);

        // Read in and convert pointer values to absolute values on the canvas
        auto pointerX = LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
        auto pointerY = LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
        auto newX = static_cast<int>((pointerX + 0x7fff) / (float)(0x7fff * 2) * bufferWidth);
        auto newY = static_cast<int>((pointerY + 0x7fff) / (float)(0x7fff * 2) * bufferHeight);

        // Use mouse pointer movement
        if ((pointerX != 0 || pointerY != 0) && (newX != lastMouseX || newY != lastMouseY)) {
            lastMouseX = newX;
            lastMouseY = newY;

            // Use layout system to validate and map coordinates
            if (IsWithinTouchscreen(layout, newX, newY)) {
                x = std::max<float>(static_cast<int>(layout.bottom_screen.left),
                             std::min<float>(newX, static_cast<int>(layout.bottom_screen.right))) -
                    layout.bottom_screen.left;
                y = std::max<float>(static_cast<int>(layout.bottom_screen.top),
                             std::min<float>(newY, static_cast<int>(layout.bottom_screen.bottom))) -
                    layout.bottom_screen.top;
            }
        }
    }

    if (LibRetro::settings.analog_touch_enabled) {
        // Check right analog input
        state |= LibRetro::CheckInput(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2); //Orinally R3

        // TODO: Provide config option for ratios here
        int maxSpeed = LibRetro::settings.maxspeed;
        float realSpeed;
        switch (maxSpeed) {
            case 1:
                realSpeed = 0.4f;
                break;
            case 2:
                realSpeed = 0.6f;
                break;
            case 3:
                realSpeed = 0.8f;
                break;
            case 4:
                realSpeed = 1.0f;
                break;
            case 5:
                realSpeed = 1.2f;
                break;
            case 6:
                realSpeed = 1.4f;
                break;
            case 7:
                realSpeed = 1.6f;
                break;
            case 8:
                realSpeed = 1.8f;
                break;
            case 9:
                realSpeed = 2.0f;
                break;
            default:
                realSpeed = 0.8f; // Default to max speed
        }
        float heightSpeed = (layout.bottom_screen.GetHeight() / 20.0) * realSpeed;
        // Use controller movement
        float joystickNormX =
            ((float)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_X) /
             INT16_MAX);
        float joystickNormY =
            ((float)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_Y) /
             INT16_MAX);
        float deadzone = LibRetro::settings.deadzone;
        bool speedup_enabled = LibRetro::settings.speedup_enabled;
        float responsecurve = LibRetro::settings.responsecurve;
        float speedupratio = LibRetro::settings.speedupratio;
        float radialLength = std::sqrt((joystickNormX * joystickNormX) + (joystickNormY * joystickNormY));
        float joystickScaledX = 0.0f;
        float joystickScaledY = 0.0f;
        if (radialLength > deadzone) {
            // Get X and Y as a relation to the radial length
            float dirX = joystickNormX / radialLength;
            float dirY = joystickNormY / radialLength;
            // Apply deadzone and response curve
            float scaledLength = (radialLength - deadzone) / (1.0f - deadzone);
            float curvedLength = std::pow(std::min<float>(1.0f, scaledLength), responsecurve);
            // Final output
            float finalLength = speedup_enabled ? curvedLength * speedupratio : curvedLength;
            joystickScaledX = dirX * finalLength;
            joystickScaledY = dirY * finalLength;
        } else {
            joystickScaledX = 0.0f;
            joystickScaledY = 0.0f;
        }
        OnMouseMove(static_cast<float>(joystickScaledX * heightSpeed),
                    static_cast<float>(joystickScaledY * heightSpeed));
    }

    Restrict(0, 0, layout.bottom_screen.GetWidth(), layout.bottom_screen.GetHeight());

    // Make the coordinates 0 -> 1
    projectedX = (float)x / layout.bottom_screen.GetWidth();
    projectedY = (float)y / layout.bottom_screen.GetHeight();

    // Ensure that the projected position doesn't overlap outside the bottom screen framebuffer.
    // TODO: Provide config option
    renderRatio = (float)layout.bottom_screen.GetHeight() / 30;

    // Map the mouse coord to the bottom screen's position
    projectedX = layout.bottom_screen.left + projectedX * layout.bottom_screen.GetWidth();
    projectedY = layout.bottom_screen.top + projectedY * layout.bottom_screen.GetHeight();

    isPressed = state;

    this->framebuffer_layout = layout;
}

void MouseTracker::Render(int bufferWidth, int bufferHeight, void* framebuffer_data) {
    if (!LibRetro::settings.render_touchscreen) {
        return;
    }

    // Delegate to renderer-specific implementation
    if (cursor_renderer) {
        cursor_renderer->Render(bufferWidth, bufferHeight, projectedX, projectedY, renderRatio,
                                framebuffer_layout, framebuffer_data);
    }
}

#ifdef ENABLE_OPENGL
// OpenGL-specific cursor renderer implementation
OpenGLCursorRenderer::OpenGLCursorRenderer() {
    // Could potentially also use Citra's built-in shaders, if they can be
    //  wrangled to cooperate.

    std::string vertex;
    if (Settings::values.use_gles) {
        vertex += fragment_shader_precision_OES;
    }

    vertex += R"(
        in vec2 position;

        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    std::string fragment;
    if (Settings::values.use_gles) {
        fragment += fragment_shader_precision_OES;
    }
    fragment += R"(
        out vec4 color;

        void main()
        {
            color = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )";

    vao.Create();
    vbo.Create();

    glBindVertexArray(vao.handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);

    shader.Create(vertex.c_str(), fragment.c_str());

    auto positionVariable = (GLuint)glGetAttribLocation(shader.handle, "position");
    glEnableVertexAttribArray(positionVariable);

    glVertexAttribPointer(positionVariable, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

OpenGLCursorRenderer::~OpenGLCursorRenderer() {
    shader.Release();
    vao.Release();
    vbo.Release();
}

void OpenGLCursorRenderer::Render(int bufferWidth, int bufferHeight, float projectedX,
                                  float projectedY, float renderRatio,
                                  const Layout::FramebufferLayout& layout, void* framebuffer_data) {
    // Use shared coordinate calculation
    CursorCoordinates coords(bufferWidth, bufferHeight, projectedX, projectedY, renderRatio,
                             layout);

    glUseProgram(shader.handle);

    glBindVertexArray(vao.handle);

    // clang-format off
    GLfloat cursor[] = {
            // | in the cursor
            coords.verticalLeft,  coords.verticalTop,
            coords.verticalRight, coords.verticalTop,
            coords.verticalRight, coords.verticalBottom,

            coords.verticalLeft,  coords.verticalTop,
            coords.verticalRight, coords.verticalBottom,
            coords.verticalLeft,  coords.verticalBottom,

            // - in the cursor
            coords.horizontalLeft,  coords.horizontalTop,
            coords.horizontalRight, coords.horizontalTop,
            coords.horizontalRight, coords.horizontalBottom,

            coords.horizontalLeft,  coords.horizontalTop,
            coords.horizontalRight, coords.horizontalBottom,
            coords.horizontalLeft,  coords.horizontalBottom
    };
    // clang-format on

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);

    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursor), cursor, GL_STATIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, 12);

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}
#endif

#ifdef ENABLE_VULKAN
// Vulkan-specific cursor renderer implementation
VulkanCursorRenderer::VulkanCursorRenderer() {
    // Vulkan cursor rendering will be integrated into the main rendering pipeline
}

VulkanCursorRenderer::~VulkanCursorRenderer() = default;

void VulkanCursorRenderer::Render(int bufferWidth, int bufferHeight, float projectedX,
                                  float projectedY, float renderRatio,
                                  const Layout::FramebufferLayout& layout, void* framebuffer_data) {
    // Use shared coordinate calculation
    CursorCoordinates coords(bufferWidth, bufferHeight, projectedX, projectedY, renderRatio,
                             layout);

    // TODO: Implement actual Vulkan cursor drawing using the renderer's command buffer
    // This would involve:
    // 1. Creating a simple vertex buffer with cursor geometry using coords
    // 2. Using a basic shader pipeline
    // 3. Recording draw commands into the current command buffer
    // 4. Using blend mode similar to OpenGL (ONE_MINUS_DST_COLOR, ONE_MINUS_SRC_COLOR)

    // For now, this is a placeholder - the cursor won't be visible in Vulkan mode
    // but the touchscreen input will still work
}
#endif

// Software-specific cursor renderer implementation
SoftwareCursorRenderer::SoftwareCursorRenderer() {
    // Software renderer initialization
}

SoftwareCursorRenderer::~SoftwareCursorRenderer() = default;

void SoftwareCursorRenderer::Render(int bufferWidth, int bufferHeight, float projectedX,
                                    float projectedY, float renderRatio,
                                    const Layout::FramebufferLayout& layout,
                                    void* framebuffer_data) {
    if (!framebuffer_data) {
        return; // No framebuffer data available
    }

    // Convert coordinates to screen space
    int centerX = static_cast<int>(projectedX);
    int centerY = static_cast<int>(projectedY);
    int radius = static_cast<int>(renderRatio);

    // Calculate cursor dimensions within bounds
    int verticalLeft = std::max(centerX - radius / 5, static_cast<int>(layout.bottom_screen.left));
    int verticalRight =
        std::min(centerX + radius / 5, static_cast<int>(layout.bottom_screen.right));
    int verticalTop = std::max(centerY - radius, static_cast<int>(layout.bottom_screen.top));
    int verticalBottom = std::min(centerY + radius, static_cast<int>(layout.bottom_screen.bottom));

    int horizontalLeft = std::max(centerX - radius, static_cast<int>(layout.bottom_screen.left));
    int horizontalRight = std::min(centerX + radius, static_cast<int>(layout.bottom_screen.right));
    int horizontalTop = std::max(centerY - radius / 5, static_cast<int>(layout.bottom_screen.top));
    int horizontalBottom =
        std::min(centerY + radius / 5, static_cast<int>(layout.bottom_screen.bottom));

    // Draw cursor directly to framebuffer (assuming RGBA8888 format)
    uint32_t* pixels = static_cast<uint32_t*>(framebuffer_data);
    const uint32_t cursorColor = 0xFFFFFFFF; // White cursor

    // Draw vertical line of cursor
    for (int y = verticalTop; y < verticalBottom; ++y) {
        for (int x = verticalLeft; x < verticalRight; ++x) {
            if (x >= 0 && x < bufferWidth && y >= 0 && y < bufferHeight) {
                int pixelIndex = y * bufferWidth + x;
                // XOR blend for visibility on any background
                pixels[pixelIndex] ^= cursorColor;
            }
        }
    }

    // Draw horizontal line of cursor
    for (int y = horizontalTop; y < horizontalBottom; ++y) {
        for (int x = horizontalLeft; x < horizontalRight; ++x) {
            if (x >= 0 && x < bufferWidth && y >= 0 && y < bufferHeight) {
                int pixelIndex = y * bufferWidth + x;
                // XOR blend for visibility on any background
                pixels[pixelIndex] ^= cursorColor;
            }
        }
    }
}

} // namespace Input

} // namespace LibRetro
