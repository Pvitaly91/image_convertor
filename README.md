# Image Convertor

Windows GUI application for converting AVIF images from URL to JPEG format.

## Requirements

- **Windows 10 or Windows 11**
- **Visual Studio 2022** (with "Desktop development with C++" workload)
- **vcpkg** (package manager)
- **CMake 3.21+** (included with Visual Studio 2022)

## Setup Instructions

### Step 1: Install vcpkg

If you don't have vcpkg installed:

```powershell
# Open PowerShell as Administrator and run:
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Set VCPKG_ROOT environment variable (REQUIRED for CMakePresets.json)
setx VCPKG_ROOT "C:\vcpkg"
# Or via PowerShell:
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
```

**Important:** After setting `VCPKG_ROOT`, restart your terminal/PowerShell and Visual Studio.

### Step 2: Enable vcpkg Integration with Visual Studio

Option A - **System-wide integration** (recommended):
```powershell
cd C:\vcpkg
.\vcpkg integrate install
```

Option B - **CMake toolchain file** (alternative):
Set the `CMAKE_TOOLCHAIN_FILE` environment variable:
```powershell
[Environment]::SetEnvironmentVariable("CMAKE_TOOLCHAIN_FILE", "C:\vcpkg\scripts\buildsystems\vcpkg.cmake", "User")
```

### Step 3: Clone the Repository

```powershell
git clone https://github.com/Pvitaly91/image_convertor.git
cd image_convertor
```

### Step 4: Open in Visual Studio 2022

1. Open **Visual Studio 2022**
2. Select **File → Open → Folder...**
3. Navigate to the cloned `image_convertor` folder and click **Select Folder**
4. Visual Studio will automatically detect the CMake project

### Step 5: Configure the Build

1. Wait for CMake configuration to complete (check Output window → Show output from: CMake)
2. VS2022 will automatically use `CMakePresets.json` to configure the vcpkg toolchain
3. Select the build configuration:
   - Click on the dropdown in the toolbar
   - Choose `x64-debug` or `x64-release`
4. If configuration fails, verify `VCPKG_ROOT` is set correctly:
   ```powershell
   echo $env:VCPKG_ROOT
   ```

### Step 6: Build the Project

Press **Ctrl+Shift+B** or select **Build → Build All**

### Step 7: Run the Application

Press **F5** (Debug) or **Ctrl+F5** (Run without debugging)

## Project Structure

```
image_convertor/
├── CMakeLists.txt      # CMake build configuration
├── CMakePresets.json   # CMake presets for VS2022 + vcpkg
├── vcpkg.json          # vcpkg manifest (dependencies)
├── README.md           # This file
└── src/
    └── main.cpp        # Application source code
```

## Features (Current Implementation)

- Win32 GUI application with:
  - URL input field (supports http:// and https://)
  - Convert button
  - Multi-line status display
- Pictures folder as default output location
- Worker thread for non-blocking UI
- Status updates: "Downloading...", "Decoding...", "Saving...", "Done"
- Creates a dummy output file in Pictures folder

## Dependencies

Managed via vcpkg manifest mode:
- **libavif** - AVIF image format library
- **libjpeg-turbo** - High-speed JPEG library

System libraries:
- **winhttp** - HTTP client API
- **shell32** - Shell functions (folder paths)
- **ole32** - COM support

## Troubleshooting

### CMake configuration fails with "Could not find a package configuration file provided by libavif/avif"

This error means vcpkg hasn't installed the dependencies yet, or the toolchain isn't being used.

**Step 1: Verify VCPKG_ROOT is set correctly**
```powershell
# Check in PowerShell:
echo $env:VCPKG_ROOT
# Should output your vcpkg path, e.g., G:\DEV\vcpkg

# If empty, set it permanently:
setx VCPKG_ROOT "G:\DEV\vcpkg"
# Then close and reopen PowerShell/VS2022
```

**Step 2: Manually install dependencies (if manifest mode doesn't trigger)**
```powershell
cd $env:VCPKG_ROOT
.\vcpkg install libavif:x64-windows libjpeg-turbo:x64-windows
```

**Step 3: Clean CMake cache and reconfigure**
1. Close Visual Studio
2. Delete these folders in your project directory:
   - `out/`
   - `.vs/`
3. Reopen VS2022: File → Open → Folder
4. Select a preset: x64-debug or x64-release

**Step 4: Add diagnostic output (optional)**
Temporarily add these lines at the top of CMakeLists.txt (after `cmake_minimum_required`):
```cmake
message(STATUS "=== VCPKG DIAGNOSTICS ===")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_ROOT env: $ENV{VCPKG_ROOT}")
message(STATUS "=========================")
```
If CMAKE_TOOLCHAIN_FILE is empty, VS2022 is not using the vcpkg toolchain.

### Build fails with "cannot find library"
- Run `vcpkg integrate install` in PowerShell
- Delete the `out` folder and reconfigure CMake

### Application doesn't start
- Check that all DLLs are present in the output directory
- Run in Debug mode to see error messages

## TODO: Implementing Real Conversion Logic

The current implementation uses stub functions. To implement actual conversion:

### 1. WinHTTP Download (in `WorkerThreadProc`, Step 1)

```cpp
// Location: src/main.cpp, WorkerThreadProc function, after "Downloading..." status

// Parse URL
URL_COMPONENTS urlComp = {};
urlComp.dwStructSize = sizeof(urlComp);
urlComp.dwSchemeLength = -1;
urlComp.dwHostNameLength = -1;
urlComp.dwUrlPathLength = -1;

WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp);

// Create session
HINTERNET hSession = WinHttpOpen(L"ImageConvertor/1.0", 
    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
    WINHTTP_NO_PROXY_NAME, 
    WINHTTP_NO_PROXY_BYPASS, 0);

// Connect and download...
// Store downloaded data in std::vector<uint8_t> avifData;
```

### 2. libavif Decoding (Step 2)

```cpp
// Location: src/main.cpp, WorkerThreadProc function, after "Decoding..." status

#include <avif/avif.h>

avifDecoder* decoder = avifDecoderCreate();
avifResult result = avifDecoderSetIOMemory(decoder, avifData.data(), avifData.size());
result = avifDecoderParse(decoder);
result = avifDecoderNextImage(decoder);

avifRGBImage rgb = {};
avifRGBImageSetDefaults(&rgb, decoder->image);
avifRGBImageAllocatePixels(&rgb);
avifImageYUVToRGB(decoder->image, &rgb);

// rgb.pixels contains RGB data
// rgb.width, rgb.height - dimensions
// rgb.rowBytes - bytes per row
```

### 3. libjpeg-turbo Encoding (Step 3)

```cpp
// Location: src/main.cpp, WorkerThreadProc function, after "Saving..." status

#include <turbojpeg.h>

tjhandle tjInstance = tjInitCompress();
unsigned char* jpegBuf = nullptr;
unsigned long jpegSize = 0;

tjCompress2(tjInstance, rgb.pixels, rgb.width, rgb.rowBytes, rgb.height,
    TJPF_RGB, &jpegBuf, &jpegSize, TJSAMP_444, 90, TJFLAG_FASTDCT);

// Write jpegBuf to file
// ...

tjFree(jpegBuf);
tjDestroy(tjInstance);
avifRGBImageFreePixels(&rgb);
avifDecoderDestroy(decoder);
```

### 4. Add Include Headers

Add at the top of main.cpp:
```cpp
#include <avif/avif.h>
#include <turbojpeg.h>
#include <vector>
```

## License

MIT License
