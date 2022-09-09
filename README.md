## Optimacros testcase

### Requirements
- C++17 or newer
- cmake
- gcc 5 or newer
- make

### Build
```
git clone https://github.com/juviee/optimacros_testcase
cd optimacros_testcase
cmake .
make
```
### Supported systems(tested)

- Windows: MinGW64 with MSYS2
- Linux: Void Linux, so on most rolling release distros must be working

### Additional configs

You can optimize amount of input data slicers. For that, change value in src/how2thread.hpp: Config_consts::slicer_no to appropriate