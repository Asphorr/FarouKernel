#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <memory>

class Graphics {
public:
    enum Mode {
        POINT, LINE, RECTANGLE, CIRCLE, TEXT, STRING, IMAGE
    };

private:
    static constexpr int screenWidth = 800;
    static constexpr int screenHeight = 600;

    std::unique_ptr<unsigned char[]> imageData_;
    int currentMode_ = -1;
    int currentX_ = 0;
    int currentY_ = 0;
    int currentFontSize_ = 12;
    int currentLength_ = 0;

public:
    explicit Graphics();
    ~Graphics();

    void setGraphicsMode(int mode);
    void resetGraphicsMode();

    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawRectangle(int x, int y, int width, int height);
    void drawCircle(int x, int y, int radius);
    void drawText(const char *text, int x, int y, int fontSize);
    void drawString(const char *text, int x, int y, int fontSize, int length);
    void drawImage(const unsigned char *imageData, int x, int y, int width, int height);

    int getScreenWidth() const;
    int getScreenHeight() const;

private:
    void updateCurrentMode_(int newMode);
    void updateCurrentPosition_(int newX, int newY);
    void updateCurrentFontSize_(int newFontSize);
    void updateCurrentLength_(int newLength);
};

Graphics::Graphics() : imageData_{nullptr}, currentMode_{-1}, currentX_{0}, currentY_{0}, currentFontSize_{12}, currentLength_{0} {}

Graphics::~Graphics() {
    if (imageData_) {
        free(imageData_.get());
    }
}

void Graphics::setGraphicsMode(int mode) {
    switch (mode) {
    case POINT:
        break;
    case LINE:
        break;
    case RECTANGLE:
        break;
    case CIRCLE:
        break;
    case TEXT:
        break;
    case STRING:
        break;
    default:
        throw std::invalid_argument{"Invalid graphics mode"};
    }

    currentMode_ = mode;
}

void Graphics::resetGraphicsMode() {
    currentMode_ = -1;
}

void Graphics::drawPoint(int x, int y) {
    if (!imageData_) {
        throw std::runtime_error{"No image data available"};
    }

    auto pixelIndex = (y * screenWidth + x) * 4;
    imageData_[pixelIndex] = 0xFF;
    imageData_[pixelIndex + 1] = 0xFF;
    imageData_[pixelIndex + 2] = 0xFF;
    imageData_[pixelIndex + 3] = 0xFF;
}

void Graphics::drawLine(int x1, int y1, int x2, int y2) {
    if (!imageData_) {
        throw std::runtime_error{"No image data available"};
    }

    auto dx = abs(x2 - x1);
    auto dy = abs(y2 - y1);
    auto sx = x1 < x2 ? 1 : -1;
    auto sy = y1 < y2 ? 1 : -1;
    auto err = dx - dy;

    while (true) {
        drawPoint(x1, y1);
        if (x1 == x2 && y1 == y2) {
            break;
        }
        auto e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void Graphics::drawRectangle(int x, int y, int width, int height) {
    if (!imageData_) {
        throw std::runtime_error{"No image data available"};
    }

    auto left = x;
    auto right = x + width - 1;
    auto top = y;
    auto bottom = y + height - 1;

    drawLine(left, top, right, top);
    drawLine(right, top, right, bottom);
    drawLine(right, bottom, left, bottom);
    drawLine(left, bottom, left, top);
}

void Graphics::drawCircle(int x, int y, int radius) {
    if (!imageData_) {
        throw std::runtime_error{"No image data available"};
    }

    auto centerX = x;
    auto centerY = y;
    auto rSquared = radius * radius;

    for (auto dy = -radius; dy <= radius; ++dy) {
        auto dx = sqrt(rSquared - dy * dy);
        drawPoint(centerX + dx, centerY + dy);
        drawPoint(centerX - dx, centerY + dy);
        drawPoint(centerX + dx, centerY - dy);
        drawPoint(centerX - dx, centerY - dy);
    }
}

void Graphics::drawText(const char *
