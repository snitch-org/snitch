# Example of using *snitch* with vcpkg
 
This is an example of building a project with *snitch* as a dependency, using [vcpkg][vcpkg].
 
## Set up vcpkg
 
Clone vcpkg and run the bootstrap script.
 
```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg && ./bootstrap-vcpkg.bat
```
 
## Build and run the project

### CMake

The folder containing this README includes CMake files for an example empty project using *snitch*. You can build this example project with the following code.
```bash
"In directory of vcpkg-example"
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build .
```
After running the above code, you can find *snitch* installed by vcpkg in a directory named `vcpkg-installed`.

### MSbuild

MSbuild project can be integrated directly with the `vcpkg integrate install` command.

[vcpkg]: https://github.com/microsoft/vcpkg