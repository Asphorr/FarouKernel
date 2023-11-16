#ifndef FILE_H
#define FILE_H

#include <string>
#include <memory>
#include <fstream>
#include "concepts.h"

class File {
public:
   explicit File(const std::string& filename);
   virtual ~File();
   bool isOpen() const;
   bool open();
   void close();
   template <WritableType T>
   bool write(const T& value);
   template <ReadableType T>
   bool read(T& value);
private:
   std::string _filename;
   std::unique_ptr<std::ofstream> _fileStream;
   bool _isOpen;
};

#endif
