#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <iterator>
#include <functional>

class Process {
public:
    explicit Process(int id): _id{id}, _state{"running"} {}

    [[nodiscard]] int id() const noexcept { return _id; }
    [[nodiscard]] const char* state() const noexcept { return _state; }

private:
    int _id;
    const char* _state;
};

using ProcessPtr = std::unique_ptr<Process>;

struct CompareProcessId {
    bool operator()(const ProcessPtr& p1, const ProcessPtr& p2) const {
        return p1->id() > p2->id();
    }
};

template <typename T>
using Vector = std::vector<T, std::allocator<T>>;

Vector<ProcessPtr> sortAndFilterProcesses(Vector<ProcessPtr>& processes) {
    Vector<ProcessPtr> sortedProcesses;
    std::sort(processes.begin(), processes.end(), CompareProcessId());
    std::copy_if(processes.begin(), processes.end(), std::back_inserter(sortedProcesses),
                 [] (auto&& process) { return process->state() != "zombie"; });
    return sortedProcesses;
}

int main() {
    Vector<ProcessPtr> processes;
    processes.emplace_back(new Process(1));
    processes.emplace_back(new Process(2));
    processes.emplace_back(new Process(3));
    processes.emplace_back(new Process(4));
    processes.emplace_back(new Process(5));

    auto sortedProcesses = sortAndFilterProcesses(processes);

    for (const auto& process : sortedProcesses) {
        std::cout << process->id() << ": " << process->state() << '\n';
    }

    return 0;
}
