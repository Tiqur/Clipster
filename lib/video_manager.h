#include <string>

class VideoManager
{
  public:
    VideoManager();
    ~VideoManager();

    bool LoadFrame(const char* filename, int* width_out, int* height_out, unsigned char** data_out);
};
