#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

// Using C++23 features
using namespace std::literals; // Enable string literals

struct User {
    std::string name;
    std::string email;
};

class System {
public:
    void addUser(std::string&& name, std::string&& email) {
        users.push_back({std::move(name), std::move(email)});
    }

    [[nodiscard]] bool hasUserWithEmail(const std::string& email) const noexcept {
        return std::any_of(users.begin(), users.end(), [&email](const auto& user) {
            return user.email == email;
        });
    }

private:
    std::vector<User> users;
};

int main() {
    System system;

    system.addUser("John Doe"s, "johndoe@example.com"s);
    system.addUser("Jane Doe"s, "janedoe@example.com"s);

    std::cout << "Has John Doe? "sv << system.hasUserWithEmail("johndoe@example.com"s) << '\n';
    std::cout << "Has Jane Doe? "sv << system.hasUserWithEmail("janedoe@example.com"s) << '\n';

    return 0;
}
