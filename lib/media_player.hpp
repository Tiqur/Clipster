#ifndef MEDIAPLAYER_HPP
#define MEDIAPLAYER_HPP

#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class MediaPlayer {
private:
  unsigned int currentTime;
  AVFormatContext* pFormatContext = nullptr;
  AVCodecContext* videoCodecContext = nullptr;
  AVCodecContext* audioCodecContext = nullptr;
  const char* fileName = nullptr;
  bool initializeStreams();
  int videoStreamIndex = -1;
  int audioStreamIndex = -1;
  AVCodecParameters* videoCodecParams = nullptr;
  AVCodecParameters* audioCodecParams = nullptr;
  const AVCodec* videoCodec = nullptr;
  const AVCodec* audioCodec = nullptr;
  AVPacket* packet = nullptr; // Reused for both video and audio
  AVFrame* videoFrame = nullptr;
  AVFrame* audioFrame = nullptr;


public:
  MediaPlayer();
  ~MediaPlayer();
  bool loadFile(const char* fileName);
  void play();
  void pause();
  void seek(unsigned int time);

  bool loadFile(char* fileName);
  void reset();

};

#endif // MEDIAPLAYER_HPP
