#include "citra_qt/input/cursor.h"
#include <cmath>
#include <algorithm>


void Cursor::Restrict(){
    xPos = std::clamp(xPos, 0.0f, 319.0f);
    yPos = std::clamp(yPos, 0.0f, 239.0f);

}

void Cursor::Move(float deltaX, float deltaY){
    xPos += deltaX;
    yPos += deltaY;
}

void Cursor::UpdateTouchPosition(){
    xTouchPos = std::floor(xPos);
    yTouchPos = std::floor(yPos);
}

void Cursor::UpdateRenderPosition(int minX, int minY, int maxX, int maxY){
    pixelWidth = (maxX - minX)/320.0f; //Calculate how wide a 3ds pixel is in terms of current layout
    pixelHeight = (maxY - minY)/240.0f; //Calculate how tall a 3ds pixel is in terms of current layout
    xRenderPos = (std::floor(xPos)*pixelWidth)+minX;
    yRenderPos = (std::floor(yPos)*pixelHeight)+minY;
}

void Cursor::Render(){
    // Center->Left->Up->Up->Draw(B)->Right->Draw(B)->Right->Draw(B)
    Cursor::DrawBlackPixel(xRenderPos-(1*pixelWidth), yRenderPos+(2*pixelHeight));
    Cursor::DrawBlackPixel(xRenderPos-(0*pixelWidth), yRenderPos+(2*pixelHeight));
    Cursor::DrawBlackPixel(xRenderPos+(1*pixelWidth), yRenderPos+(2*pixelHeight));

    // Center->Up->Left->Left->Draw(B)->Right->Draw(W)->Right->Draw(W)->Right->Draw(W)->Right->Draw(B)
    Cursor::DrawBlackPixel(xRenderPos-(2*pixelWidth), yRenderPos+(1*pixelHeight));
    Cursor::DrawWhitePixel(xRenderPos-(1*pixelWidth), yRenderPos+(1*pixelHeight));
    Cursor::DrawWhitePixel(xRenderPos-(0*pixelWidth), yRenderPos+(1*pixelHeight));
    Cursor::DrawWhitePixel(xRenderPos+(1*pixelWidth), yRenderPos+(1*pixelHeight));
    Cursor::DrawBlackPixel(xRenderPos+(2*pixelWidth), yRenderPos+(1*pixelHeight));

    // Center->Left->Left->Draw(B)->Right->Draw(W)->Right->Draw(W)->Right->Draw(W)->Right->Draw(B)
    Cursor::DrawBlackPixel(xRenderPos-(2*pixelWidth), yRenderPos);
    Cursor::DrawWhitePixel(xRenderPos-(1*pixelWidth), yRenderPos);
    Cursor::DrawWhitePixel(xRenderPos-(0*pixelWidth), yRenderPos);
    Cursor::DrawWhitePixel(xRenderPos+(1*pixelWidth), yRenderPos);
    Cursor::DrawBlackPixel(xRenderPos+(2*pixelWidth), yRenderPos);

    // Center->Down->Left->Left->Draw(B)->Right->Draw(W)->Right->Draw(W)->Right->Draw(W)->Right->Draw(B)
    Cursor::DrawBlackPixel(xRenderPos-(2*pixelWidth), yRenderPos-(1*pixelHeight));
    Cursor::DrawWhitePixel(xRenderPos-(1*pixelWidth), yRenderPos-(1*pixelHeight));
    Cursor::DrawWhitePixel(xRenderPos-(0*pixelWidth), yRenderPos-(1*pixelHeight));
    Cursor::DrawWhitePixel(xRenderPos+(1*pixelWidth), yRenderPos-(1*pixelHeight));
    Cursor::DrawBlackPixel(xRenderPos+(2*pixelWidth), yRenderPos-(1*pixelHeight));

    // Center->Left->Down->Down->Draw(B)->Right->Draw(B)->Right->Draw(B)
    Cursor::DrawBlackPixel(xRenderPos-(1*pixelWidth), yRenderPos-(2*pixelHeight));
    Cursor::DrawBlackPixel(xRenderPos-(0*pixelWidth), yRenderPos-(2*pixelHeight));
    Cursor::DrawBlackPixel(xRenderPos+(1*pixelWidth), yRenderPos-(2*pixelHeight));
}

void Cursor::DrawBlackPixel(float x, float y){
    painter->fillRect((int) x,(int)y,(int) pixelWidth, (int)pixelHeight, Qt::black);
}

void Cursor::DrawWhitePixel(float x, float y){
    painter->fillRect((int) x,(int)y,(int) pixelWidth, (int)pixelHeight, Qt::white);
}

void Cursor::setPainter(QPainter* newPainter){
    painter = newPainter;
}
bool Cursor::GetIsPressed(){

}
