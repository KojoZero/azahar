#pragma once
#include "core/frontend/framebuffer_layout.h"

class Cursor {
public:
    // Clamp the cursor position within touchscreen boundaries
    void Restrict();

    // The main function that calls the other ones
    void Update();

    // Move the cursor in terms of fractional 3ds coordinates
    void Move(float deltaX, float deltaY);

    // Calculate the touch position of cursor in coordinates for emu_window
    void UpdateTouchPosition();

    // Return whether touchscreen is being pressed
    bool GetIsPressed();

    void SetLayout(const Layout::FramebufferLayout* newLayout);

    unsigned GetXTouchPos();
    unsigned GetYTouchPos();

    // Get cursor position in 3DS coordinates (0-319, 0-239)
    float GetXPos() const;
    float GetYPos() const;

private:
    // Cursor coordinates in 3DS coordinates
    float xPos = 0;
    float yPos = 0;

    // Cursor coordinates mapped to 3DS internal coordinates
    unsigned xTouchPos = 0;
    unsigned yTouchPos = 0;

    // Whether touchscreen is pressed
    bool isPressed = false;

    const Layout::FramebufferLayout* layout = nullptr;
};
