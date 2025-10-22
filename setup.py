from setuptools import setup, find_packages
from setuptools.command.build_py import build_py
import subprocess
import os
import shutil
import sys


class BuildWithCMake(build_py):
    def run(self):
        # Get the directory containing setup.py (could be unpacked sdist or git repo)
        setup_dir = os.path.dirname(os.path.abspath(__file__))

        # Check if CMakeLists.txt exists
        cmake_file = os.path.join(setup_dir, "CMakeLists.txt")
        if not os.path.exists(cmake_file):
            print("ERROR: CMakeLists.txt not found!")
            print(f"Looking in: {setup_dir}")
            print("Contents:", os.listdir(setup_dir))
            sys.exit(1)

        build_dir = os.path.join(setup_dir, "build")
        os.makedirs(build_dir, exist_ok=True)

        print(f"Building C++ library from {setup_dir}")

        # Configure CMake
        cmake_cmd = ["cmake", "-DCMAKE_BUILD_TYPE=Release", setup_dir]
        print(f"Running: {' '.join(cmake_cmd)}")
        subprocess.check_call(cmake_cmd, cwd=build_dir)

        # Build
        make_cmd = ["cmake", "--build", ".", "--config", "Release", "-j4"]
        print(f"Running: {' '.join(make_cmd)}")
        subprocess.check_call(make_cmd, cwd=build_dir)

        # Copy library to package directory
        lib_dir = os.path.join(build_dir, "lib")
        package_dir = os.path.join(self.build_lib, "chronos")
        os.makedirs(package_dir, exist_ok=True)

        import platform

        if platform.system() == "Darwin":
            lib_name = "libchronos.dylib"
        elif platform.system() == "Windows":
            lib_name = "chronos.dll"
        else:
            lib_name = "libchronos.so"

        src = os.path.join(lib_dir, lib_name)
        if os.path.exists(src):
            print(f"Copying {src} to {package_dir}")
            shutil.copy(src, package_dir)
        else:
            print(f"ERROR: Could not find {lib_name} at {src}")
            print(
                f"Contents of {lib_dir}:",
                os.listdir(lib_dir)
                if os.path.exists(lib_dir)
                else "directory does not exist",
            )
            sys.exit(1)

        build_py.run(self)


with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="chronos-gpu",
    version="1.0.1",
    author="Ojima Abraham",
    author_email="abrahamojima2018@gmail.com",
    description="Fair GPU time-sharing with automatic expiration",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/oabraham1/chronos",
    packages=find_packages(where="python"),
    package_dir={"": "python"},
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Topic :: System :: Hardware",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS",
        "Operating System :: Microsoft :: Windows",
    ],
    python_requires=">=3.7",
    install_requires=[],
    cmdclass={
        "build_py": BuildWithCMake,
    },
    include_package_data=True,
    package_data={
        "chronos": ["*.so", "*.dylib", "*.dll"],
    },
    license="Apache-2.0",
)
