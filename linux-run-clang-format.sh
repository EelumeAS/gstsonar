#!/bin/sh
find . -type f -name "*.cpp" -o -name "*.c" -o -name "*.h" | xargs clang-format -i -style=file