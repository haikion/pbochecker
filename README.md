# PBO Checker

A fast, cross-platform utility for verifying PBO signatures in Arma 3 mods.

## Overview

PBO Checker verifies the digital signatures of PBO (BI Packed Object) files used in Arma 3 mods. It validates that PBO files are properly signed and haven't been tampered with.

Key features:
- Fast verification by only reading parts of PBOs needed for signature checking
- Multiple operation modes (single PBO, entire mod directory)
- Cross-platform support (Windows, Linux)
- Single binary deployment
- Automatic detection of bikey and bisign files

## Usage

```
pbochecker.exe <pbo> [bikey] [bisign]
pbochecker.exe <mod_directory>
pbochecker.exe <mod_set_directory>
```

### Parameters:
- pbo: Path to the PBO file
- bikey: Path to the public key file (optional)
- bisign: Path to the bisign file (optional)
- mod_directory: Path to the mod directory (starting with @)
- mod_set_directory: Path to a parent directory containing mod directories

### Examples:

Verify a single PBO with a specific key file:
```
pbochecker.exe MyMod/addons/example.pbo @MyMod/keys/mymod.bikey
```

Verify a single PBO with automatic key and bisign detection:
```
pbochecker.exe @MyMod/addons/example.pbo
```

Verify an entire mod directory:
```
pbochecker.exe @MyMod
```

## Building

### Requirements:
- Qt 6.x or later
- OpenSSL development libraries
- C++23 compatible compiler

### Build Commands:

#### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/your/Qt/installation
make
```

#### Windows
You can use the provided batch files:
- `makeStatic.bat` - For building with MSVC
- `makeStaticOpenSSL.bat` - For building static OpenSSL libraries

Building a single executable requires a static Qt build. The batch files contain hardcoded paths that need to be adjusted to match your environment before execution.

## Library Usage

The PBO Checker functionality is also available as a library that can be integrated into other projects:

### Integrating with CMake

Add the following to your CMakeLists.txt:

```cmake
# Add PBO Checker library
add_subdirectory(path/to/pbochecker/lib)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Add to your target
add_executable(your_application
    your_sources.cpp
    ${PBOCHECKER_LIB_SOURCES}
)
target_include_directories(your_application PRIVATE path/to/pbochecker/lib)
target_link_libraries(your_application PRIVATE OpenSSL::SSL OpenSSL::Crypto)
```

### Example Code

```cpp
#include "pbochecker.h"

PboChecker pboChecker;

// Verify a PBO with a specific key
const auto [success, failedPboPaths] = pboChecker.checkPbo("path/to/file.pbo", "path/to/key.bikey") ;

// Or verify with automatic key detection
const auto [success2, failedPboPaths2] = pboChecker.checkPbo("path/to/file.pbo");
```

## Related Projects

- [armake2](https://github.com/KoffeinFlummi/armake2) - A toolkit for Arma 3 modding
- [libpbo](https://github.com/StidOfficial/libpbo) - A library for working with PBO files

## License

This project is licensed under the MIT License - see the LICENSE file for details.
