#ifndef MAIN_HPP
#define MAIN_HPP

#include <iostream>

void printArchInfo();

class Arch {
private:
   std::string name;
   int version;
public:
   Arch();
   Arch(const std::string& name, int version);
   void setName(const std::string& newName);
   void setVersion(int newVersion);
   std::string getName() const;
   int getVersion() const;
   void printInfo() const;
};

#endif // MAIN_HPP
