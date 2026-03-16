#include "citra_qt/input/cursor.h"
#include <cmath>
#include <algorithm>
#include "citra_qt/bootmanager.h"

void Cursor::Restrict(){
    xPos = std::clamp(xPos, 0.0f, 319.0f);
    yPos = std::clamp(yPos, 0.0f, 239.0f);

}

void Cursor::Move(float deltaX, float deltaY){
    xPos += deltaX;
    yPos += deltaY;
}

void Cursor::Update(){
    //Implement Analog Input + L2 listener here and get push the output to move
    Cursor::Move(0.01666, 0.01666);
    Cursor::Restrict();
    Cursor::UpdateTouchPosition();
    //Implement R2 listener here and call
    Cursor::UpdateRenderPosition();
    Cursor::Render();

}

void Cursor::UpdateTouchPosition(){
    xTouchPos = std::round((((xPos+1)/320)*(layout->bottom_screen.right - layout->bottom_screen.left))+(layout->bottom_screen.left));
    yTouchPos = std::round((((yPos+1)/240)*(layout->bottom_screen.top - layout->bottom_screen.bottom))+(layout->bottom_screen.bottom));
}

void Cursor::UpdateRenderPosition(){
    pixelWidth = (layout->bottom_screen.right - layout->bottom_screen.left)/320.0f; //Calculate how wide a 3ds pixel is in terms of current layout
    pixelHeight = (layout->bottom_screen.top - layout->bottom_screen.bottom)/240.0f; //Calculate how tall a 3ds pixel is in terms of current layout
    xRenderPos = (std::floor(xPos)*pixelWidth)+layout->bottom_screen.left;
    yRenderPos = (std::floor(yPos)*pixelHeight)+layout->bottom_screen.bottom;
}

void Cursor::Render(){
    // Center->Left->Up->Up->Draw(B)->Right->Draw(B)->Right->Draw(B)
    Cursor::DrawBlackPixel(-1, 2);
    Cursor::DrawBlackPixel(0, 2);
    Cursor::DrawBlackPixel(1, 2);

    // Center->Up->Left->Left->Draw(B)->Right->Draw(W)->Right->Draw(W)->Right->Draw(W)->Right->Draw(B)
    Cursor::DrawBlackPixel(-2, 1);
    Cursor::DrawWhitePixel(-1, 1);
    Cursor::DrawWhitePixel(0, 1);
    Cursor::DrawWhitePixel(1, 1);
    Cursor::DrawBlackPixel(2, 1);

    // Center->Left->Left->Draw(B)->Right->Draw(W)->Right->Draw(W)->Right->Draw(W)->Right->Draw(B)
    Cursor::DrawBlackPixel(-2, 0);
    Cursor::DrawWhitePixel(-1, 0);
    Cursor::DrawWhitePixel(0, 0);
    Cursor::DrawWhitePixel(1, 0);
    Cursor::DrawBlackPixel(2, 0);

    // Center->Down->Left->Left->Draw(B)->Right->Draw(W)->Right->Draw(W)->Right->Draw(W)->Right->Draw(B)
    Cursor::DrawBlackPixel(-2, -1);
    Cursor::DrawWhitePixel(-1, -1);
    Cursor::DrawWhitePixel(0, -1);
    Cursor::DrawWhitePixel(1, -1);
    Cursor::DrawBlackPixel(2, -1);

    // Center->Left->Down->Down->Draw(B)->Right->Draw(B)->Right->Draw(B)
    Cursor::DrawBlackPixel(-1, -2);
    Cursor::DrawBlackPixel(0, -2);
    Cursor::DrawBlackPixel(1, -2);
}

void Cursor::DrawBlackPixel(int offsetX, int offsetY){
    int x = xRenderPos+(offsetX*pixelWidth);
    int y = yRenderPos+(offsetY*pixelWidth);
    painter->fillRect(x, y, pixelWidth, pixelHeight, Qt::black);
}

void Cursor::DrawWhitePixel(int offsetX, int offsetY){
    int x = xRenderPos+(offsetX*pixelWidth);
    int y = yRenderPos+(offsetY*pixelWidth);
    painter->fillRect(x, y, pixelWidth, pixelHeight, Qt::white);
}

void Cursor::SetPainter(QPainter* newPainter){
    painter = newPainter;
}

void Cursor::SetLayout(Layout::FramebufferLayout* newLayout){
    layout = newLayout;
}
bool Cursor::GetIsPressed(){
    return isPressed;
}

unsigned Cursor::GetXTouchPos(){
    return xTouchPos;
}

unsigned Cursor::GetYTouchPos(){
    return yTouchPos;
}
