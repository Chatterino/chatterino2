from conans import ConanFile, CMake, tools


class LibCommuniConan(ConanFile):
    name = "serialize"
    version = "0.1"
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
        git.checkout("130ffc3ec722284ca454a1e70c5478a75f380144")

    def package(self):
        self.copy("include/*.hpp")

    def package_id(self):
        self.info.header_only()
