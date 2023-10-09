class Graphics {
public:
    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawRectangle(int x, int y, int width, int height);
    void drawCircle(int x, int y, int radius);
    
    template<typename T>
    void drawText(T&& text, int x, int y, int fontSize);
    template<typename T>
    void drawString(T&& text, int x, int y, int fontSize, int length);
    
    void drawImage(const unsigned char* imageData, int x, int y, int width, int height);
    
    void setGraphicsMode(int mode);
    
    int getScreenWidth() const;
    int getScreenHeight() const;
    
private:
    int screenWidth_;
    int screenHeight_;
    int currentMode_;
};
