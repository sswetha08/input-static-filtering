# Program Input Static Filtering

### Build and Installation process:

**Note**: 

To speed-up build times it is recommended to build and run this tool on the following VCL instance:
Ubuntu 22.04 - 16 cores 128 GB RAM

Clone the repository: 

` git clone https://github.com/sswetha08/input-static-filtering.git `

As Clang is part of the LLVM project, it is setup as a submodule in this project. To fetch it, do:

` git submodule update --recursive  --init `

#### Installation dependencies:
* CMake

`sudo apt install cmake`
* Ninja

`sudo apt install ninja-build`

#### Build Clang:

```
cd input-static-filtering/
mkdir build && cd build
cmake -G Ninja ../llvm-project/llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=host
ninja
sudo ninja install
```

Finally, we build again by setting Clang as its own compiler:
```
sudo apt install g++-12

cmake -G Ninja ../llvm-project/llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++ -DLLVM_TARGETS_TO_BUILD=host
ninja
```

#### Add the Input Analyser tool to the build

You will need to move the directory input-analyzer (inside src) into the clang-tools-extra repository.

```
cd ~/input-static-filtering
cp -r src/input-analyzer llvm-project/clang-tools-extra/
echo 'add_subdirectory(input-analyzer)' >> llvm-project/clang-tools-extra/CMakeLists.txt
```

With that done, Ninja will be able to compile the tool: (quick as it is an incremental build)

```
cd build
ninja  
```
### Run the Tool:

Example: 
```
cd build
bin/input-analyzer ../tests/example3.c 
```
You can similarly run it on any of the example programs under the tests/ directory

### Outputs:

![image](https://user-images.githubusercontent.com/23032578/235474614-97ecbe69-7174-4bad-a197-7e24c7fbe5c9.png)

![image](https://user-images.githubusercontent.com/23032578/235474658-bed97524-9b2a-4905-aacc-f15487860613.png)



