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

void MouseTracker::OnMouseMove(double deltaX, double deltaY) {
    x += deltaX;
    y += deltaY;
}

void MouseTracker::Restrict(double minX, double minY, double maxX, double maxY) {
    x = std::min(std::max(minX, x), maxX);
    y = std::min(std::max(minY, y), maxY);
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

            x = std::max(static_cast<int>(bottomScreen.left),
                         std::min(newX, static_cast<int>(bottomScreen.right))) -
                bottomScreen.left;
            y = std::max(static_cast<int>(bottomScreen.top),
                         std::min(newY, static_cast<int>(bottomScreen.bottom))) -
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

            x = std::max(static_cast<int>(bottomScreen.left),
                         std::min(newX, static_cast<int>(bottomScreen.right))) -
                bottomScreen.left;
            y = std::max(static_cast<int>(bottomScreen.top),
                         std::min(newY, static_cast<int>(bottomScreen.bottom))) -
                bottomScreen.top;
        }
    }

    if (LibRetro::settings.analog_touch_enabled) {
        // Check right analog input
        state |= LibRetro::CheckInput(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2); //Orinally R3

        // TODO: Provide config option for ratios here
        int maxSpeed = LibRetro::settings.maxspeed;
        auto widthSpeed = (bottomScreen.GetWidth() / 20.0) * std::pow(1.5,(maxSpeed-5)/5);
        auto heightSpeed = (bottomScreen.GetHeight() / 20.0) * std::pow(1.5,(maxSpeed-5)/5);

        // Use controller movement
        double joystickNormX =
            ((double)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_X) /
             INT16_MAX);
        double joystickNormY =
            ((double)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_Y) /
             INT16_MAX);

        // Deadzone the controller inputs
        double absX = std::abs(joystickNormX);
        double absY = std::abs(joystickNormY);
        double deadzone = LibRetro::settings.deadzone;
        double radialLength = std::min(1.0, std::sqrt((absX*absX)+(absY*absY)));
        int signX = 0;
        int signY = 0;
        double joystickScaledX = 0;
        double joystickScaledY = 0;
        double joystickDeflectionX = 0;
        double joystickDeflectionY = 0;
        if (radialLength <= deadzone) {
            joystickScaledX = 0;
            joystickScaledY = 0;
        } else {
            double scaledLength = (radialLength - deadzone) / (1 - deadzone);
            double scaleFactor = scaledLength/radialLength;
            if (joystickNormX < 0) {
                signX = -1;
            } else {
                signX = 1;
            }
            if (joystickNormY < 0) {
                signY = -1;
            } else {
                signY = 1;
            }
            //See joystick deflection after deadzone
            joystickDeflectionX = absX*scaleFactor; //Gives adjusted joystick deflection after deadzone
            joystickDeflectionY = absY*scaleFactor;
            double adjustJoystickRadialLength = std::min(1.0, std::sqrt((joystickDeflectionX*joystickDeflectionX)+(joystickDeflectionY*joystickDeflectionY)));

            //Set these values to adjust the response curve of the joystick
            double edgeboostdeadzone = LibRetro::settings.edgeboostdeadzone; //This is the radial length at which the boost multiplier starts to apply. From this value to 1.0, the boost multiplier will scale the speed linearly.
            double preboostratio = LibRetro::settings.preboostratio; //Max speed when joystick is below the edgeboostdeadzone. The Boost multiplier will scale from this speed to the full max width/height speed (1.0)
            double responsecurve = LibRetro::settings.responsecurve; //responsecurve of the exponential response curve
            double boostMultiplier = 0.0;
            if (edgeboostdeadzone == 0.0) {
                joystickScaledX = signX*std::pow(std::min(1.0,(joystickDeflectionX)),responsecurve); //No edgeboost and no scaling
                joystickScaledY = signY*std::pow(std::min(1.0,(joystickDeflectionY)),responsecurve);
            } else {
                double adjustedScaleFactor = 1.0/edgeboostdeadzone;
                if (adjustJoystickRadialLength >= edgeboostdeadzone) {
                    boostMultiplier = preboostratio+((1-preboostratio)*((adjustJoystickRadialLength-edgeboostdeadzone)/(1- edgeboostdeadzone))); //Boost multiplier scales linearly once the radialjoystick deflection is greater than 0.9
                    joystickScaledX = signX*std::pow(std::min(1.0,((joystickDeflectionX)*adjustedScaleFactor)),responsecurve)*boostMultiplier;
                    joystickScaledY = signY*std::pow(std::min(1.0,((joystickDeflectionY)*adjustedScaleFactor)),responsecurve)*boostMultiplier;
                } else {
                    joystickScaledX = signX*std::pow(std::min(1.0,((joystickDeflectionX)*adjustedScaleFactor)),responsecurve)*preboostratio; //divide deflection by 0.9 to full range between 0 and 0.9 deflection. Then Multiply by preboostratio so max deflection below 0.9 gives a max speed of specified fraction
                    joystickScaledY = signY*std::pow(std::min(1.0,((joystickDeflectionY)*adjustedScaleFactor)),responsecurve)*preboostratio;
                }
            }
            //LOG THE FOLLOWING: joystickNormX, joystickNormY, joystickDeflectionX, joystickDeflectionY, adjustJoystickRadialLength, edgeboostdeadzone, adjustedScaleFactor, boostMultipler, joystickScaledX, joystickScaledY
            // if (log_cb)
            //     log_cb(RETRO_LOG_INFO, "Joystick Norm X: %f, Joystick Norm Y: %f, Joystick Deflection X: %f, Joystick Deflection Y: %f, Adjusted Joystick Radial Length: %f, Boost Multiplier Deadzone: %f, Adjusted Scale Factor: %f, Boost Multiplier: %f, Joystick Scaled X: %f, Joystick Scaled Y: %f\n",
            //     joystickNormX, joystickNormY, joystickDeflectionX, joystickDeflectionY,
            //     adjustJoystickRadialLength, edgeboostdeadzone, adjustedScaleFactor,
            //     boostMultiplier, joystickScaledX, joystickScaledY);
            // retro_log_printf_t log_cb = GetLoggingBackend();
            // if (log_cb)
            //     log_cb(RETRO_LOG_INFO, "Deadzone: %f, Edge Boost Deadzone: %f, Pre-Boost Ratio: %f, Response Curve: %f, Max Speed: %d\n", deadzone, edgeboostdeadzone, preboostratio, responsecurve, maxSpeed);

        }

        OnMouseMove(static_cast<double>(joystickScaledX * heightSpeed),
                    static_cast<double>(joystickScaledY * heightSpeed));
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
