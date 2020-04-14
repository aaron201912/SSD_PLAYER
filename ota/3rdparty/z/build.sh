rm -rf out
mkdir -p out
tar -vxf zlib-1.2.11.tar.gz -C ./out
path=`pwd`/out
export CC=$1-gcc
export AR=$1-ar
cd out/zlib-1.2.11/
./configure --prefix=${path}
make && make install
cd -
mkdir -p lib/static/
mv out/lib/libz.a lib/static/
mkdir -p lib/dynamic/
mv out/lib/libz.so lib/dynamic/
mv out/lib/libz.so.1 lib/dynamic/
mv out/lib/libz.so.1.2.11 lib/dynamic/
cp out/include . -rf
rm -rf out
