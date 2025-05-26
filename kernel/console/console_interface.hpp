// myconsole.cpp
export module MyConsole;
import <iostream>;
import <string>;

template <typename T>
concept Printable = requires(T t) {
    { t.print() };
};

class Console : public Printable {
public:
    void print() const override {
        std::cout << "Hello from MyConsole!" << std::endl;
    }
};

export template <>
void run<Console>() {
    auto c = new Console();
    c->print();
    delete c;
}
