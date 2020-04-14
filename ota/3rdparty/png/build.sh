#!/bin/bash
rm -rf out
mkdir -p out
tar -vxf libpng-1.6.37.tar.gz -C ./out
path=`pwd`/out
cd ./out/libpng-1.6.37
./configure --prefix=${path} CC=$1-gcc --host=$1 LIBS=-L${path}/../../z/lib/dynamic/ CPPFLAGS=-I${path}/../../z/include/
make && make install
cd -
mkdir -p lib/static/
mv out/lib/libpng16.a lib/static/
mv out/lib/libpng.a lib/static/
mkdir -p lib/dynamic/
mv out/lib/libpng.so lib/dynamic/
mv out/lib/libpng16.so lib/dynamic/
mv out/lib/libpng16.so.16 lib/dynamic/
mv out/lib/libpng16.so.16.37.0 lib/dynamic/
cp out/include . -rf
rm -rf out
