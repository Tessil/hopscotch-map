import os

from conan.packager import ConanMultiPackager


if __name__ == "__main__":
    username = "tessil"
    channel = "stable"
    upload_remote = "https://api.bintray.com/conan/tessil/tsl" if os.getenv("TRAVIS_TAG") else None
    test_folder = os.path.join("conan", "test_package")

    builder = ConanMultiPackager(username=username,
                                 channel=channel,
                                 upload=upload_remote,
                                 test_folder=test_folder)
    builder.add_common_builds()
    builder.builds = [builder.builds[0]]
    builder.run()
