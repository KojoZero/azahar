// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef ENABLE_OPENGL

#include <cmath>
#include <glad/glad.h>

#include "citra_libretro/core_settings.h"
#include "citra_libretro/environment.h"
#include "citra_libretro/input/mouse_tracker.h"
#include "common/settings.h"

#include "video_core/shader/generator/glsl_shader_gen.h"

namespace LibRetro {

namespace Input {

MouseTracker::MouseTracker() {
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

MouseTracker::~MouseTracker() {
    shader.Release();
    vao.Release();
    vbo.Release();
}

void MouseTracker::OnMouseMove(float deltaX, float deltaY) {
    x += deltaX;
    y += deltaY;
}

void MouseTracker::Restrict(float minX, float minY, float maxX, float maxY) {
    x = std::min<float>(std::max<float>(minX, x), maxX);
    y = std::min<float>(std::max<float>(minY, y), maxY);
}

void MouseTracker::Update(int bufferWidth, int bufferHeight,
                          Common::Rectangle<unsigned> bottomScreen) {
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

            x = std::max<float>(static_cast<int>(bottomScreen.left),
                         std::min<float>(newX, static_cast<int>(bottomScreen.right))) -
                bottomScreen.left;
            y = std::max<float>(static_cast<int>(bottomScreen.top),
                         std::min<float>(newY, static_cast<int>(bottomScreen.bottom))) -
                bottomScreen.top;
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

            x = std::max<float>(static_cast<int>(bottomScreen.left),
                         std::min<float>(newX, static_cast<int>(bottomScreen.right))) -
                bottomScreen.left;
            y = std::max<float>(static_cast<int>(bottomScreen.top),
                         std::min<float>(newY, static_cast<int>(bottomScreen.bottom))) -
                bottomScreen.top;
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
                realSpeed = 1.0f; // Default to max speed
        }
        //float widthSpeed = (bottomScreen.GetWidth() / 20.0) * realSpeed;
        float heightSpeed = (bottomScreen.GetHeight() / 20.0) * realSpeed;

        // Use controller movement
        float joystickNormX =
            ((float)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_X) /
             INT16_MAX);
        float joystickNormY =
            ((float)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_Y) /
             INT16_MAX);

        // Deadzone the controller inputs
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

            // Apply deadzone and curve
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

        // retro_log_printf_t log_cb = GetLoggingBackend();
        // if (log_cb)
        //     log_cb(RETRO_LOG_INFO, "slowdownratio: %f, slowdown_enabled: %d, responsecurve: %f, joystickScaledX: %f, joystickScaledY: %f\n",
        //             slowdownratio, slowdown_enabled, responsecurve, joystickScaledX, joystickScaledY);

        OnMouseMove(static_cast<float>(joystickScaledX * heightSpeed),
                    static_cast<float>(joystickScaledY * heightSpeed));
    }

    Restrict(0, 0, bottomScreen.GetWidth()-1.0, bottomScreen.GetHeight()-1.0);

    // Make the coordinates 0 -> 1
    projectedX = static_cast<float>(static_cast<int>(x)) / bottomScreen.GetWidth();
    projectedY = static_cast<float>(static_cast<int>(y)) / bottomScreen.GetHeight();

    // Ensure that the projected position doesn't overlap outside the bottom screen framebuffer.
    // TODO: Provide config option
    renderRatio = (float)bottomScreen.GetHeight() / 30;

    // Map the mouse coord to the bottom screen's position
    projectedX = bottomScreen.left + projectedX * bottomScreen.GetWidth();
    projectedY = bottomScreen.top + projectedY * bottomScreen.GetHeight();

    isPressed = state;

    this->bottomScreen = bottomScreen;
}

void MouseTracker::Render(int bufferWidth, int bufferHeight) {
    if (!LibRetro::settings.render_touchscreen) {
        return;
    }

    // Convert to OpenGL coordinates
    float centerX = (projectedX / bufferWidth) * 2 - 1;
    float centerY = (projectedY / bufferHeight) * 2 - 1;

    float renderWidth = renderRatio / bufferWidth;
    float renderHeight = renderRatio / bufferHeight;

    float boundingLeft = (bottomScreen.left / (float)bufferWidth) * 2 - 1;
    float boundingTop = (bottomScreen.top / (float)bufferHeight) * 2 - 1;
    float boundingRight = (bottomScreen.right / (float)bufferWidth) * 2 - 1;
    float boundingBottom = (bottomScreen.bottom / (float)bufferHeight) * 2 - 1;

    // Calculate the size of the vertical stalk
    float verticalLeft = std::fmax(centerX - renderWidth / 5, boundingLeft);
    float verticalRight = std::fmin(centerX + renderWidth / 5, boundingRight);
    float verticalTop = -std::fmax(centerY - renderHeight, boundingTop);
    float verticalBottom = -std::fmin(centerY + renderHeight, boundingBottom);

    // Calculate the size of the horizontal stalk
    float horizontalLeft = std::fmax(centerX - renderWidth, boundingLeft);
    float horizontalRight = std::fmin(centerX + renderWidth, boundingRight);
    float horizontalTop = -std::fmax(centerY - renderHeight / 5, boundingTop);
    float horizontalBottom = -std::fmin(centerY + renderHeight / 5, boundingBottom);

    glUseProgram(shader.handle);

    glBindVertexArray(vao.handle);

    // clang-format off
    GLfloat cursor[] = {
            // | in the cursor
            verticalLeft,  verticalTop,
            verticalRight, verticalTop,
            verticalRight, verticalBottom,

            verticalLeft,  verticalTop,
            verticalRight, verticalBottom,
            verticalLeft,  verticalBottom,

            // - in the cursor
            horizontalLeft,  horizontalTop,
            horizontalRight, horizontalTop,
            horizontalRight, horizontalBottom,

            horizontalLeft,  horizontalTop,
            horizontalRight, horizontalBottom,
            horizontalLeft,  horizontalBottom
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

} // namespace Input

} // namespace LibRetro

#endif
