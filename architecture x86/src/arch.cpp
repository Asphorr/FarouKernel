// Implement the apply method for the SumFirstKElements concept
template <SumFirstKElements T>
constexpr decltype(auto) apply(T begin, T end, size_t k) {
   return std::accumulate(std::next(begin), std::min(std::next(begin, k), end), *begin);
}

// Implement the apply method for the SumLastKElements concept
template <SumFirstKElements T>
constexpr decltype(auto) apply_last(T begin, T end, size_t k) {
   return std::accumulate(std::prev(end), std::max(std::prev(end, k), begin), *end);
}

// Implement the max_element method for the MaxElement concept
template <MaxElement T>
constexpr auto max_element(T begin, T end) {
   return std::max_element(begin, end);
}

// Implement the min_element method for the MinElement concept
template <MinElement T>
constexpr auto min_element(T begin, T end) {
   return std::min_element(begin, end);
}

// Implement the median_element method for the MedianElement concept
template <typename T>
concept bool MedianElement = requires(T x, T y) {
   { x >= y } -> bool;
};

// Implement the median_element method for the MedianElement concept
template <MedianElement T>
constexpr auto median_element(T begin, T end) {
   std::vector<T> v(begin, end);
   std::sort(v.begin(), v.end());
   return v[v.size() / 2];
}

// Implement the mean_value method for the MeanValue concept
template <MeanValue T>
constexpr auto mean_value(T begin, T end) {
   return std::accumulate(begin, end, 0.0) / std::distance(begin, end);
}

// Implement the median_value method for the MedianValue concept
template <MeanValue T>
constexpr auto median_value(T begin, T end) {
   std::vector<T> v(begin, end);
   std::sort(v.begin(), v.end());
   return v[v.size() / 2];
}

int main() {
   // Read input from stdin
   int n;
   std::cout << "Enter the number of elements: ";
   while (!(std::cin >> n) || n <= 0) {
       std::cout << "Please enter a positive integer: ";
       std::cin.clear();
       std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
   }
   
   // Create vector of integers
   std::vector<int> numbers(n);
   for (auto& num : numbers) {
       std::cout << "Enter a number: ";
       while (!(std::cin >> num)) {
           std::cout << "Please enter an integer: ";
           std::cin.clear();
           std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
       }
   }
   
   // Sort the vector in descending order
   std::sort(numbers.rbegin(), numbers.rend());
   
   // Calculate sum of first k elements
   size_t k = n / 2;
   long long sum = apply(numbers.begin(), numbers.end(), k);
   
   // Calculate sum of last k elements
   long long sum_last = apply_last(numbers.begin(), numbers.end(), k);
   
   // Find maximum element
   auto max_it = max_element(numbers.begin(), numbers.end());
   
   // Find minimum element
   auto min_it = min_element(numbers.begin(), numbers.end());
   
   // Find median element
   auto median_it = median_element(numbers.begin(), numbers.end());
   
   // Calculate mean value
   double mean = mean_value(numbers.begin(), numbers.end());
   
   // Calculate median value
   double median = median_value(numbers.begin(), numbers.end());
   
   // Print results
   std::cout << "Sum of first " << k << " elements: " << sum << "\n";
   std::cout << "Sum of last " << k << " elements: " << sum_last << "\n";
   std::cout << "Maximum element: " << *max_it << "\n";
   std::cout << "Minimum element: " << *min_it << "\n";
   std::cout << "Median element: " << *median_it << "\n";
   std::cout << "Mean value: " << mean << "\n";
   std::cout << "Median value: " << median << "\n";
   
   return 0;
}
