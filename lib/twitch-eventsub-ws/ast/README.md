## Available scripts

- `generate-and-replace-dir.py`  
  Usage: `./generate-and-replace-dir.py <path-to-dir-containing-header-files>`  
  Will find all header files in that directory (it won't search recursively), and if they also have a matching source file, generate json deserialize definition & implementations for them, and replacing the markers in the given file.

- `get-builtin-include-dirs.py`  
  Usage: `./get-builtin-include-dirs.py`  
  Prints what builtin include dirs will be used for any of the other scripts.

- `generate.py`  
  Usage: `./generate.py <path-to-header-file>`  
  Generates definitions & implementations for the given header file and write them to temporary files.

- `replace-in-file.py`  
  Usage: `./replace-in-file.py <path-to-header-file> <path-to-source-file>`
  Reads sdin for two file paths containing the definition & implementations.  
  Can be used in tandem with `generate.py`, but realistically it's only there because I used it at some point.

## Environment variables

The following environment variables can be configured to change the behaviours of this project

- `LLVM_PATH`  
  Will be used to change where to find the `clang++` executable file.  
  Example: `LLVM_PATH=/opt/llvm` ./get-builtin-include-dirs.py will use `/opt/llvm/bin/clang++`

- `LIBCLANG_LIBRARY_FILE`  
  Will be used to change where clang cindex can find the dynamic library.  
  Must be an absolute path to the file.  
  Example: `LIBCLANG_LIBRARY_FILE=/opt/llvm/lib/libclang.so.15.0.7`

- `LIBCLANG_LIBRARY_PATH`  
  Will be used to change where clang cindex can look for the dynamic library.  
  Must be an absolute path to the directory.  
  Example: `LIBCLANG_LIBRARY_PATH=/opt/llvm/lib`
