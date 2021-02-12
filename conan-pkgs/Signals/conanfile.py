from conans import ConanFile, CMake, tools


class SignalsConan(ConanFile):
    name = "signals"
    version = "0.1"
    license = "MIT"
    author = "Edgar Edgar@AnotherFoxGuy.com"
    url = "https://github.com/Chatterino/chatterino2"
    description = "simple C++ signal library"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def source(self):
        git = tools.Git()
        git.clone("https://github.com/pajlada/signals.git")
        git.checkout("baf5bb04bd13b090e405e0447c89a811f7e23ddc")

    def package(self):
        self.copy("*.hpp", "include", "include")

    def package_id(self):
        self.info.header_only()
