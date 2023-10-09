class Page {
public:
    explicit Page(uint32_t frameNumber) : frameNumber_(frameNumber), dirty_(false), valid_(true) {}
    ~Page() = default;

    void setFrameNumber(uint32_t frameNumber) { frameNumber_ = frameNumber; }
    [[nodiscard]] uint32_t getFrameNumber() const { return frameNumber_; }

    void setDirty(bool dirty) { dirty_ = dirty; }
    [[nodiscard]] bool isDirty() const { return dirty_; }

    void setValid(bool valid) { valid_ = valid; }
    [[nodiscard]] bool isValid() const { return valid_; }

private:
    uint32_t frameNumber_;
    bool dirty_;
    bool valid_;
};
