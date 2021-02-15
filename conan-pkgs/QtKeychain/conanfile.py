from conans import ConanFile, CMake, tools


class QtKeychainConan(ConanFile):
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
        git.clone("https://github.com/frankosterfeld/qtkeychain.git")
        git.checkout("9a22739ea5d36bb3de46dbb93b22da2b2c119461")

    def build(self):
        cmake = CMake(self)
        cmake.definitions['BUILD_TRANSLATIONS'] = 'OFF'
        cmake.definitions['BUILD_TEST_APPLICATION'] = 'OFF'
        # cmake.definitions['QTKEYCHAIN_STATIC'] = 'ON'
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
