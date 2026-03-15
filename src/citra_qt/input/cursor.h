#pragma once
#include <QPainter>
#include <QPixmap>

class Cursor {
public:
    Cursor();
    ~Cursor();
    //Clamp the cursor position within touchscreen boundaries
    void Restrict();

    //Move the cursor in terms of fractions 3ds coordinates
    void Move(float deltaX, float deltaY);

    //Calculate the render position of cursor for current layout. Parameters are the framebufferf
    void UpdateRenderPosition(int minX, int minY, int maxX, int maxY);

    //Calculate the touch position of cursor in 3ds coordinates (integer)
    void UpdateTouchPosition();

    //Draw the cursor on screen
    void Render();

    void DrawBlackPixel(float x, float y);

    void DrawWhitePixel(float x, float y);

    //Return whether touchscreen is being pressed
    bool GetIsPressed();

    void setPainter(QPainter* newPainter);

private:
    //Cursor Coordinates in terms of fractional 3ds coordinates
    float xPos;
    float yPos;

    //Cursor Coordinates mapped to 3ds internal coordinates
    int xTouchPos;
    int yTouchPos;

    //Cursor Coordinates mapped to layout coordinates and snapped to grid for draw. The value represents the bottom left of the rendered center pixel
    float xRenderPos;
    float yRenderPos;

    float pixelHeight;
    float pixelWidth;

    //Whether touchscreen is pressed
    bool isPressed;

    QPainter *painter;
};
