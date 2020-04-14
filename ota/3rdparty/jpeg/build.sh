#!/bin/bash
rm -rf out
mkdir -p out
tar -vxf jpegsrc.v9d.tar.gz -C ./out
path=`pwd`/out
cd ./out/jpeg-9d
./configure --prefix=${path}  CC=$1-gcc --enable-shared --enable-static --host=$1
make && make install
cd -
mkdir -p lib/static/
mv out/lib/libjpeg.a lib/static/
mkdir -p lib/dynamic/
mv out/lib/libjpeg.so lib/dynamic/
mv out/lib/libjpeg.so.9 lib/dynamic/
mv out/lib/libjpeg.so.9.4.0 lib/dynamic/
cp out/include . -rf
rm -rf out
