from conans import ConanFile, CMake, tools


class WebSocketppConan(ConanFile):
    name = "WebSocketpp"
    version = "0.8.1"
    license = "MIT"
    author = "Edgar Edgar@AnotherFoxGuy.com"
    url = "https://github.com/Chatterino/chatterino2"
    description = "WebSocket++ is a header only C++ library that implements RFC6455 The WebSocket Protocol"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def source(self):
        git = tools.Git()
        git.clone("https://github.com/ziocleto/websocketpp.git")
        git.checkout("1e0138c7ccedc6be859d28270ccd6195f235a94e")

    def build(self):
        cmake = CMake(self)
        cmake.configure()

    def package(self):
        cmake = CMake(self)
        cmake.install()
