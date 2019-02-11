import re
from conans import ConanFile, CMake, tools


def get_version():
    """ Extract version of library from CMakeLists.txt file """
    try:
        content = tools.load("CMakeLists.txt")
        version = re.search(r"project\(tsl-hopscotch-map VERSION (.*)\)", content).group(1)
        return version.strip()
    except Exception:
        return None


class TslHopscotchMapConan(ConanFile):
    name = "tsl-hopscotch-map"
    url = "https://github.com/Tessil/hopscotch-map"
    version = get_version()
    license = "MIT"
    description="C++ implementation of a fast hash map and hash set using hopscotch hashing."
    exports_sources = "*"
    exports = "LICENSE"

    def package(self):
        self.copy("LICENSE", dst="licenses", keep_path=False, ignore_case=True)
        cmake = CMake(self)
        cmake.configure()
        cmake.install()

    def package_id(self):
        self.info.header_only()
