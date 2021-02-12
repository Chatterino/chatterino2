from conans import ConanFile, CMake, tools


class LibCommuniConan(ConanFile):
    name = "QtKeychain"
    version = "0.12.90"
    license = "MIT"
    author = "Edgar Edgar@AnotherFoxGuy.com"
    url = "https://github.com/Chatterino/chatterino2"
    description = "QtKeychain is a Qt API to store passwords and other secret data securely"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def source(self):
        git = tools.Git()
        git.clone("https://github.com/Chatterino/qtkeychain.git")
        git.checkout("308ea7e709113dc277be1653fe2044bb20236836")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
