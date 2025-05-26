#include "file.h"

int main() {
   // Create a File object
   File file("example.txt");

   // Open the file
   if (!file.open()) {
       std::cerr << "Failed to open file\n";
       return 1;
   }

   // Write to the file
   int value = 123;
   if (!file.write(value)) {
       std::cerr << "Failed to write to file\n";
       return 1;
   }

   // Read from the file
   int readValue;
   if (!file.read(readValue)) {
       std::cerr << "Failed to read from file\n";
       return 1;
   }

   // Print the read value
   std::cout << "Read value: " << readValue << '\n';

   // Close the file
   file.close();

   return 0;
}
