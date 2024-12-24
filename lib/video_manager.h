#include <string>

class VideoManager
{
  public:
    VideoManager();
    ~VideoManager();

    void setPath(char* newPath);
    int decodeFrame(unsigned int time);
    void generateScrubImageMap();

  private:
    std::string filePath;
};
