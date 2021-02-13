TODO
---------

## Build with conan
* Install cmake conan qt
* `cmake -P conan-pkgs/add_conan_pkgs.cmake`
* `mkdir build`  
  `cd build`  
  `cmake -DCMAKE_BUILD_TYPE=Release -DUSE_PACKAGE_MANAGER=ON ..`  
  `make -j`

## Build with cmake meta-build project
* Install cmake qt
* `cd lib`  
  `cmake -DCMAKE_BUILD_TYPE=Release ..`  
  `make -j`
  
* `mkdir build`  
  `cd build`  
  `cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_MODULE_PATH=lib/Dependencies_<name of the generator> ..`  
  `make -j`

## Build manually