#!/bin/bash
# clang-format -style=Google -dump-config > .clang-format
# astyle --style=风格 src/*.cpp include/*.h
src="$(ls src)"
include="$(ls include)"
full=
for i in $src; do
    tmp=$(echo "src/$i ")
    full+=$tmp
done
for i in $include; do
    tmp=$(echo "include/$i ")
    full+=$tmp
done

for i in $full; do
    astyle --suffix=none --style=google $i
done