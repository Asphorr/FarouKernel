#include <iostream>
#include <string>

int main() {
    constexpr auto helloWorld = "Hello, world!";
    std::cout << helloWorld << '\n';

    char ch{};
    std::cin >> ch;

    std::string str;
    std::getline(std::cin, str);

    std::cout << "Read character: " << ch << "\n";
    std::cout << "Read string: " << str << "\n";

    return 0;
}
