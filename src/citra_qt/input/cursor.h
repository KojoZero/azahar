#pragma once
#include <QPainter>
#include <QPixmap>
#include "core/frontend/framebuffer_layout.h"
#include "common/logging/log.h"
class Cursor {
public:
    //Clamp the cursor position within touchscreen boundaries
    void Restrict();

    //The main function that calls the other ones
    void Update();

    //Move the cursor in terms of fractional 3ds coordinates
    void Move(float deltaX, float deltaY);

    //Calculate the render position of cursor for current layout.
    void UpdateRenderPosition();

    //Calculate the touch position of cursor in coordinates for emu_window
    void UpdateTouchPosition();

    //Draw the cursor on screen using the current render position coordinates as the center
    void Render();

    void DrawBlackPixel(int offsetX, int offsetY);

    void DrawWhitePixel(int offsetX, int offsetY);

    //Return whether touchscreen is being pressed
    bool GetIsPressed();

    void SetPainter(QPainter* newPainter);

    void SetLayout(Layout::FramebufferLayout* newLayout);

    unsigned GetXTouchPos();

    unsigned GetYTouchPos();
private:
    //Cursor Coordinates in terms of fractional 3ds coordinates
    float xPos;
    float yPos;

    //Cursor Coordinates mapped to 3ds internal coordinates
    unsigned xTouchPos;
    unsigned yTouchPos;

    //Cursor Coordinates mapped to layout coordinates and snapped to grid for draw. The value represents the bottom left of the rendered center pixel
    float xRenderPos;
    float yRenderPos;

    float pixelHeight;
    float pixelWidth;

    //Whether touchscreen is pressed
    bool isPressed;

    QPainter* painter;
    const Layout::FramebufferLayout* layout;
};
