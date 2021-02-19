from conans import ConanFile, CMake, tools


class SerializeConan(ConanFile):
    name = "serialize"
    version = "0.2"
    license = "MIT"
    author = "Edgar Edgar@AnotherFoxGuy.com"
    url = "https://github.com/Chatterino/chatterino2"
    description = "c++ serialize/deserialize helper functions based on rapidjson"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    no_copy_source = True

    def source(self):
        git = tools.Git()
        git.clone("https://github.com/pajlada/serialize.git")
        git.checkout("7d37cbfd5ac3bfbe046118e1cec3d32ba4696469")

    def package(self):
        self.copy("*.hpp", "include", "include")

    def package_id(self):
        self.info.header_only()
