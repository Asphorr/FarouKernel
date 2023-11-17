#include "arch.hpp"

int main() {
   // Create an instance of the Arch class
   Arch myArch(4.5, 3.2);

   // Set the dimensions of the structure
   myArch.setDimensions(5.0, 3.0);

   // Calculate the area of the structure
   double area = myArch.calculateArea();
   std::cout << "Area: " << area << std::endl;

   // Print the dimensions of the structure
   myArch.printDimensions();

   return 0;
}
