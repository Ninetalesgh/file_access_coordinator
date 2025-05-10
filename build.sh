#!/bin/bash

rm -rf bin
mkdir bin
g++ src/main.cpp -o bin/file_access_coordinator -I/usr/include -lssh
chmod +x bin/file_access_coordinator
./bin/file_access_coordinator
