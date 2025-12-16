"""
Chronos GPU Partitioner - Setup Script

This setup script builds the native C++ library if possible,
but falls back gracefully to a stub implementation if the build fails.
"""

from setuptools import setup, find_packages
from setuptools.command.build_py import build_py
import subprocess
import os
import shutil
import sys
import warnings


class BuildWithCMake(build_py):
    """
    Custom build class that attempts to build the C++ library.

    If the build fails (e.g., missing OpenCL, CMake, etc.), it continues
    without the native library. The Python package will work in stub mode,
    providing helpful error messages to users.
    """

    def run(self):
        # Get the directory containing setup.py
        setup_dir = os.path.dirname(os.path.abspath(__file__))

        # Check if CMakeLists.txt exists
        cmake_file = os.path.join(setup_dir, "CMakeLists.txt")
        if not os.path.exists(cmake_file):
            print("WARNING: CMakeLists.txt not found, skipping native build")
            print("Chronos will run in stub mode.")
            build_py.run(self)
            return

        # Check for CMake
        if not self._check_cmake():
            print("WARNING: CMake not found, skipping native build")
            print("Chronos will run in stub mode.")
            print("Install CMake: https://cmake.org/download/")
            build_py.run(self)
            return

        build_dir = os.path.join(setup_dir, "build")
        os.makedirs(build_dir, exist_ok=True)

        print(f"Building C++ library from {setup_dir}")

        try:
            # Configure CMake
            cmake_cmd = [
                "cmake",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_TESTS=OFF",
                "-DBUILD_EXAMPLES=OFF",
                setup_dir
            ]
            print(f"Running: {' '.join(cmake_cmd)}")
            result = subprocess.run(
                cmake_cmd,
                cwd=build_dir,
                capture_output=True,
                text=True
            )

            if result.returncode != 0:
                self._print_build_warning(
                    "CMake configuration failed",
                    result.stderr
                )
                build_py.run(self)
                return

            # Build
            make_cmd = ["cmake", "--build", ".", "--config", "Release", "-j4"]
            print(f"Running: {' '.join(make_cmd)}")
            result = subprocess.run(
                make_cmd,
                cwd=build_dir,
                capture_output=True,
                text=True
            )

            if result.returncode != 0:
                self._print_build_warning(
                    "Build failed",
                    result.stderr
                )
                build_py.run(self)
                return

            # Copy library to package directory
            lib_dir = os.path.join(build_dir, "lib")
            package_dir = os.path.join(self.build_lib, "chronos")
            os.makedirs(package_dir, exist_ok=True)

            lib_name = self._get_lib_name()
            src = os.path.join(lib_dir, lib_name)

            if os.path.exists(src):
                print(f"Copying {src} to {package_dir}")
                shutil.copy(src, package_dir)
                print("Native library built successfully!")
            else:
                self._print_build_warning(
                    f"Library not found at {src}",
                    f"Contents of {lib_dir}: {os.listdir(lib_dir) if os.path.exists(lib_dir) else 'directory does not exist'}"
                )

        except subprocess.TimeoutExpired:
            self._print_build_warning("Build timed out", "")
        except FileNotFoundError as e:
            self._print_build_warning(f"Build tool not found: {e}", "")
        except Exception as e:
            self._print_build_warning(f"Build failed with exception: {e}", "")

        build_py.run(self)

    def _check_cmake(self) -> bool:
        """Check if CMake is available."""
        try:
            result = subprocess.run(
                ["cmake", "--version"],
                capture_output=True,
                text=True,
                timeout=10
            )
            return result.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired):
            return False

    def _get_lib_name(self) -> str:
        """Get the platform-specific library name."""
        import platform
        if platform.system() == "Darwin":
            return "libchronos.dylib"
        elif platform.system() == "Windows":
            return "chronos.dll"
        else:
            return "libchronos.so"

    def _print_build_warning(self, message: str, details: str):
        """Print a warning about build failure."""
        print("\n" + "=" * 70)
        print("WARNING: Native library build failed")
        print("=" * 70)
        print(f"\nReason: {message}")
        if details:
            print(f"\nDetails:\n{details[:500]}...")
        print("\nChronos will run in STUB MODE.")
        print("GPU partitioning functionality will not be available.")
        print("\nTo fix this, ensure you have:")
        if sys.platform == "darwin":
            print("  - Xcode command line tools: xcode-select --install")
            print("  - CMake: brew install cmake")
        elif sys.platform == "win32":
            print("  - Visual Studio Build Tools with C++ support")
            print("  - CMake: https://cmake.org/download/")
            print("  - GPU drivers with OpenCL support")
        else:
            print("  - CMake: sudo apt install cmake (Debian/Ubuntu)")
            print("  - OpenCL headers: sudo apt install ocl-icd-opencl-dev")
            print("  - Build tools: sudo apt install build-essential")
        print("\nThen reinstall: pip install --force-reinstall chronos-gpu")
        print("=" * 70 + "\n")


# Read README for long description
try:
    with open("README.md", "r", encoding="utf-8") as fh:
        long_description = fh.read()
except FileNotFoundError:
    long_description = "Fair GPU time-sharing with automatic expiration"

setup(
    name="chronos-gpu",
    version="1.1.0",
    author="Ojima Abraham",
    author_email="abrahamojima2018@gmail.com",
    description="Fair GPU time-sharing with automatic expiration and concurrent execution support",
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
    extras_require={
        "dev": ["pytest", "pytest-cov", "black", "mypy", "flake8"],
    },
    cmdclass={
        "build_py": BuildWithCMake,
    },
    include_package_data=True,
    package_data={
        "chronos": ["*.so", "*.dylib", "*.dll"],
    },
    license="Apache-2.0",
)
