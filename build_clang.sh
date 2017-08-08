export CC=/usr/bin/clang-4.0
export CXX=/usr/bin/clang++-4.0
mkdir build
cd build
cmake -DCMAKE_USER_MAKE_TOOLCHAIN_PREFIX=llvm- ${PWD}/..
make

