#ifndef DESKTOPCAPTURE_HPP
#define DESKTOPCAPTURE_HPP

#include <string>

enum class Platform {
    Windows,
    MacOS,
    Linux_X11,
    Linux_Wayland,
    Unknown
};

class DesktopCapture {
public:
    Platform getCurrentPlatform();
    std::string platformToString(Platform platform);
    void captureScreen();
};

#endif // DESKTOPCAPTURE_HPP

