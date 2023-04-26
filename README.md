# Program Input Static Filtering

### Build and Installation process:

As Clang is part of the LLVM project, it is setup as a submodule in this project. To fetch it, do:

` git submodule update --recursive  --init `

#### Installation dependencies:
* CMake

`sudo apt install cmake`
* Ninja

`sudo apt install ninja-build`

#### Build Clang:

```
mkdir build && cd build
cmake -G Ninja ../llvm-project/llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
ninja
ninja check       # Test LLVM only.
ninja clang-test  # Test Clang only.
ninja install
```

Finally, we set Clang as its own compiler:
```
cd build
ccmake ../llvm-project/llvm
ninja
```
The second command will bring up a GUI for configuring Clang. You need to set the entry for CMAKE_CXX_COMPILER. Press 't' to turn on advanced mode. Scroll down to CMAKE_CXX_COMPILER, and set it to /usr/bin/clang++, or wherever you installed it. Press 'c' to configure, then 'g' to generate CMake’s files.
Finally, run ninja one more time to re-build.
