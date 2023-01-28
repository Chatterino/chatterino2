from conan import ConanFile
from conans.tools import os_info

class Chatterino2(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {"with_crashpad": [True, False]}
    default_options: dict[str, bool] = {"with_crashpad": True, "openssl:shared": True}

    def requirements(self):
        self.requires("openssl/1.1.1s", )
        self.requires("boost/1.80.0")
        if self.options.with_crashpad:
            self.requires("crashpad/cci.20220219")
            # we need to pin zlib
            self.requires("zlib/1.2.13")
    
    def configure(self):
        self.options['openssl'].shared = True

    def imports(self):
        if os_info.is_windows:
            for pattern in ["*.dll", "crashpad_handler.exe"]:
                self.copy(pattern, src="bin", dst="bin", keep_path=False)
                self.copy(pattern, src="bin", dst="Chatterino2", keep_path=False)
        else:
            self.copy("*.so*", src="lib", dst="bin", keep_path=False)
            self.copy("crashpad_handler", src="lib", dst="bin", keep_path=False)
