from conan import ConanFile
from conan.tools.files import copy
from os import path


class Chatterino(ConanFile):
    name = "Chatterino"
    requires = "openssl/1.1.1t", "boost/1.81.0"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {"openssl*:shared": True}
    generators = "CMakeDeps", "CMakeToolchain"

    def generate(self):
        copy_bin = lambda dep, selector, subdir: copy(
            self,
            selector,
            dep.cpp_info.bindirs[0],
            path.join(self.build_folder, subdir),
            keep_path=False,
        )
        for dep in self.dependencies.values():
            # macOS
            copy_bin(dep, "*.dylib", "bin")
            # Windows
            copy_bin(dep, "*.dll", "bin")
            copy_bin(dep, "*.dll", "Chatterino2")  # used in CI
            # Linux
            copy(
                self,
                "*.so*",
                dep.cpp_info.libdirs[0],
                path.join(self.build_folder, "bin"),
                keep_path=False,
            )
