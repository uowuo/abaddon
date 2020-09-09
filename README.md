### Building:
#### Windows:
1. `git clone https://github.com/lorpus/abaddon && cd abaddon`
2. `vcpkg install gtkmm:x64-windows nlohmann-json:x64-windows ixwebsocket:x64-windows cpr:x64-windows zlib:x64-windows simpleini:x64-windows`
3. `mkdir build && cd build`
4. `cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=c:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVCPKG_TARGET_TRIPLET=x64-windows ..`
5. Build with Visual Studio

#### Mac:
1. Install gtkmm3 zlib openssl and nlohmann-json from homebrew
2. `git clone --recurse-submodules -j8 https://github.com/lorpus/abaddon`
3. `cd abaddon`
4. `mkdir build`
5. `cd build`
6. `cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_PKGCONFIG=ON -DDISABLE_MBEDTLS=ON ../`
7. `make`

### Downloads (from CI):
- Windows: [here](https://ci.appveyor.com/project/lorpus/abaddon/build/artifacts)
- OSX: [here](https://i.owo.okinawa/travis/abaddon) (downloading css from here is still necessary)

#### Dependencies:  
* [gtkmm](https://www.gtkmm.org/en/)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [IXWebSocket](https://github.com/machinezone/IXWebSocket)
* [C++ Requests: Curl for People](https://github.com/whoshuu/cpr/)
* [zlib](https://zlib.net/)
* [simpleini](https://github.com/brofield/simpleini)
