#include "desktop_capture.hpp"
#include <iostream>
#include <cstdlib>

Platform DesktopCapture::getCurrentPlatform() {
#ifdef _WIN32
    return Platform::Windows;
#elif __APPLE__
    return Platform::MacOS;
#elif __linux__
    const char* waylandDisplay = getenv("WAYLAND_DISPLAY");
    if (waylandDisplay) {
        return Platform::Linux_Wayland;
    } else {
        const char* x11Display = getenv("DISPLAY");
        if (x11Display) {
            return Platform::Linux_X11;
        }
    }
    return Platform::Unknown;
#else
    return Platform::Unknown;
#endif
}

std::string DesktopCapture::platformToString(Platform platform) {
    switch (platform) {
        case Platform::Windows: return "Windows";
        case Platform::MacOS: return "MacOS";
        case Platform::Linux_X11: return "Linux (X11)";
        case Platform::Linux_Wayland: return "Linux (Wayland)";
        default: return "Unknown";
    }
}

void DesktopCapture::captureScreen() {
    Platform currentPlatform = getCurrentPlatform();
    std::cout << platformToString(currentPlatform) << std::endl;
}

