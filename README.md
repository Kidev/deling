# Deling

[![CI/CD](https://github.com/myst6re/deling/actions/workflows/build.yml/badge.svg)](https://github.com/myst6re/deling/actions/workflows/build.yml)

![Deling](src/qt/images/Deling.png)

Final Fantasy VIII field/worldmap archive editor ([Forum](https://forums.qhimm.com/index.php?topic=13050.0)).

## Building

If you are a developer and you want to build this software, follow the
instructions below.

### Requirements

 - CMake
 - Qt 6.5+
 - LZ4

### Windows

#### Qt + Qt Creator

0. Download the online installer from https://www.qt.io/download-qt-installer
1. Install Qt 6 with these items checked:
   - **Qt/Qt 6.x.x (last version):** Check `MSVC 2022 64-bit`, `Qt Creator`, `Qt Shader Tools`
   - **Developer And Designer Tools:** Check `CMake` and `Ninja`
2. Run Qt Creator and open an existing project, select the CMakeLists.txt in the source directory of Deling

#### Visual Studio

> **Please note:**
>
> By default Visual Studio will pick the **x86-Release** build configuration, but you can choose any other profile available.

0. **REQUIRED!** Follow the steps to install Qt, if you didn't already
1. Download the the latest [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) installer
2. Run the installer and import this [.vsconfig](.vsconfig) file in the installer to pick the required components to build this project
3. Make sure you select the English Language pack in the language list before clicking Install
4. Once installed, open this repository **as a folder** in Visual Studio 2022 and click the build button.

#### Optional: Visual Studio Code

0. **REQUIRED!** Follow the steps to install Qt, if you didn't already
1. **REQUIRED!** Follow the steps to install Visual Studio, if you didn't already
2. Download and install the latest [Visual Studio Code](https://code.visualstudio.com/) release
3. Install the following extensions:
   - https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools
   - https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools
4. Open this repository as a folder in Visual Studio code
5. Choose as build profile in the status bar `CMake: [Release]` (or one of the aforementioned profiles)
6. Click the button on the status bar `Build`

### macOS

In order to continue please make sure you have `brew` installed. See https://brew.sh/ for instructions.

#### Install dependencies
```sh
$ brew install cmake
$ brew install qt6
```
Close and reopen your terminal.

#### Build

> **Please note**: You can choose other build profiles if you want. See https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html for more information.

```sh
$ mkdir -p .dist/build .dist/install
$ cmake --preset=vcpkg -S . -B .dist/build -DCMAKE_INSTALL_PREFIX=.dist/install -DCMAKE_BUILD_TYPE=Debug
$ cmake --build .dist/build --config Debug
```

### Linux

#### Install dependencies

##### Debian/Ubuntu
```sh
$ apt-get install build-essential cmake qt6-default qttools6-dev
```

##### Arch Linux
```sh
$ pacman -S --needed base-devel cmake qt6
```

#### Build

> **Please note**: You can choose other build profiles if you want. See https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html for more information.

```sh
$ mkdir -p .dist/build .dist/install
$ cmake --preset=vcpkg -S . -B .dist/build -DCMAKE_INSTALL_PREFIX=.dist/install -DCMAKE_BUILD_TYPE=Debug
$ cmake --build .dist/build --config Debug
```

## How to contribute

Pull requests are welcome on [GitHub](https://github.com/myst6re/deling).
