#include <cassert>
#include <compare>
#include <concepts>
#include <format>
#include <iostream>
#include <ranges>
#include <span>
#include <type_traits>
#include <variant>

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
requires std::ranges::range<Range> && std::same_as<std::remove_cvref_t<decltype(*std::begin(std::declval<Range>()))>, ProcessPtr>
[[nodiscard]] auto filterZombies(Range&& processes) -> decltype(std::forward<Range>(processes) | std::views::filter([](const ProcessPtr& process){return process->state() != "zombie";})) {
 return std::forward<Range>(processes) | std::views::filter([](const ProcessPtr& process){return process->state() != "zombie";});
}

int main() {
 // Create a vector of ProcessPtr objects
 std::vector<ProcessPtr> processes;
 processes.push_back(std::make_unique<Process>(1));
 processes.push_back(std::make_unique<Process>(2));
 processes.push_back(std::make_unique<Process>(3));
 processes.push_back(std::make_unique<Process>(4));
 processes.push_back(std::make_unique<Process>(5));

 // Sort the vector of ProcessPtr objects by their IDs
 std::ranges::sort(processes, CompareProcessId());

 // Filter out any zombie processes from the vector
 auto filteredProcesses = filterZombies(processes);

 // Print the remaining non-zombie processes
 for (const auto& process : filteredProcesses) {
    std::cout << process->id() << ": " << process->state() << "\n";
 }

 return 0;
}
