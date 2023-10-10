#!/bin/bash

# Download the latest release from GitHub
curl -L https://github.com/Asphorr/funiculous/releases/latest/download/funiculus.tar.gz | tar xzvf -C /tmp

# Move to the extracted directory
cd /tmp/funiculus*

# Compile the source files
g++ -o funiculus *.cpp

# Run the installed binary
./funiculus

# Remove temporary files
rm -rf /tmp/funiculus*
