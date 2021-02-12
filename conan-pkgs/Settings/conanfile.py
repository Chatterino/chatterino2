from conans import ConanFile, CMake, tools


class SettingsConan(ConanFile):
    name = "settings"
    version = "0.1"
    license = "MIT"
    author = "Edgar Edgar@AnotherFoxGuy.com"
    url = "https://github.com/Chatterino/chatterino2"
    description = "pajlada's C++ Settings library"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def requirements(self):
        self.requires.add('rapidjson/cci.20200410')
        self.requires.add('boost/[1.x.x]')

    def source(self):
        git = tools.Git()
        git.clone("https://github.com/AnotherFoxGuy/settings.git")
        git.checkout("cmake-fix", "recursive")

    def build(self):
        cmake = CMake(self)
        cmake.definitions['CONAN_EXPORTED'] = 'ON'
        cmake.definitions['BUILD_TESTS'] = 'OFF'
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
