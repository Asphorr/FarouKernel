#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

// A simple class representing a process
class Process {
public:
    // Constructor taking an ID and initializing the state to "running"
    explicit Process(int id) : id_(id), state_{"running"} {}

    // Getters for the ID and state members
    [[nodiscard]] int id() const noexcept { return id_; }
    [[nodiscard]] std::string_view state() const noexcept { return state_; }

private:
    int id_;
    std::string state_;
};

// Define a type alias for a unique pointer to a Process object
using ProcessPtr = std::unique_ptr<Process>;

// Define a comparison function object for sorting Process objects by their IDs
struct CompareProcessId {
    bool operator()(const ProcessPtr& lhs, const ProcessPtr& rhs) const {
        return lhs->id() > rhs->id();
    }
};

// Define a function template for filtering out zombie processes from a vector of ProcessPtr objects
template <typename Range>
auto filterZombies(Range&& processes) {
    return std::move(processes) | std::views::filter([](const ProcessPtr& process){return process->state() != "zombie";}) | std::views::transform([](const ProcessPtr& process){return *process;});
}

int main() {
    // Create a vector of ProcessPtr objects
    std::vector<ProcessPtr> processes{std::in_place_type<Process>{}, 1, 2, 3, 4, 5};

    // Sort the vector of ProcessPtr objects by their IDs
    std::ranges::sort(processes, CompareProcessId());

    // Filter out any zombie processes from the vector
    auto filteredProcesses = filterZombies(processes);

    // Print the remaining non-zombie processes
    for (const auto& process : filteredProcesses) {
        std::cout << process.id() << ": " << process.state() << "\n";
    }

    return 0;
}
