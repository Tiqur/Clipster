#ifndef MEDIAPLAYER_HPP
#define MEDIAPLAYER_HPP

#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

struct AudioFrame {
    uint8_t* data;
    int size;
    double pts;
};

struct VideoFrame {
    uint8_t* data[3];
    int linesize[3];
    int width;
    int height;
    double pts;
};

class MediaPlayer {
private:
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
  void processVideoFrame(AVFrame* frame);
  void processAudioFrame(AVFrame* frame);
  void renderVideo();
  void playAudio();
  double lastFrameTime = 0.0;
  int audioBufferIndex = 0;
  int videoBufferIndex = 0;
  bool shouldRenderFrame;
  bool paused = false;
  double currentTime = 0.0;

  // For synchronization between threads
  std::mutex videoMutex;
  std::mutex audioMutex;
  std::condition_variable cvVideo;
  std::condition_variable cvAudio;


public:
  MediaPlayer();
  ~MediaPlayer();
  bool loadFile(const char* fileName);
  void play();
  void pause();
  void seek(double targetTime, bool backward);
  void syncMedia(double currentTime);
  // Temp, move to private after
  double playbackStartTime = 0.0;
  VideoFrame getVideoFrame();
  AudioFrame getAudioFrame();
  bool shouldRenderMedia();
  bool loadFile(char* fileName);
  void reset();
  std::vector<VideoFrame> videoBuffer;
  std::vector<AudioFrame> audioBuffer;

};

#endif // MEDIAPLAYER_HPP
