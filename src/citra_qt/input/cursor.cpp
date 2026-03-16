#include "citra_qt/input/cursor.h"
#include <algorithm>
#include <cmath>

void Cursor::Restrict() {
    xPos = std::clamp(xPos, 0.0f, 319.0f);
    yPos = std::clamp(yPos, 0.0f, 239.0f);
}

void Cursor::Move(float deltaX, float deltaY) {
    xPos += deltaX;
    yPos += deltaY;
}

void Cursor::Update() {
    // Implement analog input here and push the output to Move
    Cursor::Move(0.01666f, 0.01666f);
    Cursor::Restrict();
    Cursor::UpdateTouchPosition();
}

void Cursor::UpdateTouchPosition() {
    if (!layout) return;
    xTouchPos = static_cast<unsigned>(std::round(
        (((xPos + 1) / 320) * (layout->bottom_screen.right - layout->bottom_screen.left)) +
        layout->bottom_screen.left));
    yTouchPos = static_cast<unsigned>(std::round(
        (((yPos + 1) / 240) * (layout->bottom_screen.top - layout->bottom_screen.bottom)) +
        layout->bottom_screen.bottom));
}

void Cursor::SetLayout(const Layout::FramebufferLayout* newLayout) {
    layout = newLayout;
}

bool Cursor::GetIsPressed() {
    return isPressed;
}

unsigned Cursor::GetXTouchPos() {
    return xTouchPos;
}

unsigned Cursor::GetYTouchPos() {
    return yTouchPos;
}

float Cursor::GetXPos() const {
    return xPos;
}

float Cursor::GetYPos() const {
    return yPos;
}
