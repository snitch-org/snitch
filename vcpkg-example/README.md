# Snitch example
 
Snitch example that uses [vcpkg][vcpkg].
 
## Set up vcpkg
 
Clone the vcpkg and run the bootstrap script.
 
```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg && ./bootstrap-vcpkg.bat
```
 
## Build and run the project

### CMake

We provide a project folder called `vcpkg-example` to use `snitch`. You can build this example project according to the following code.
```bash
"In directory of vcpkg-example"
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build .
```
After run the above code, you can find all the `snitch` things installed by vcpkg in directory folder named `vcpkg-installed`. 

### MSbuild

MSbuild project can be integrated directly with the `vcpkg integrate install` command.

[vcpkg]: https://github.com/microsoft/vcpkg