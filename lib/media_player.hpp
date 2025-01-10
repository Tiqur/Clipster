#ifndef MEDIAPLAYER_HPP
#define MEDIAPLAYER_HPP

#include <string>
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

struct VideoFrame {
  std::vector<uint8_t> data[3];
  int linesize[3];
  int width;
  int height;
  double pts;

  VideoFrame() {
    width = 0;
    height = 0;
    pts = 0.0;

    for (int i = 0; i < 3; i++) {
      data[i].clear();
      linesize[i] = 0;
    }
  }
};

struct AudioFrame {
  std::vector<uint8_t> data;
  int size;
  double pts;

  AudioFrame() {
    data.clear();
    size = 0;
    pts = 0.0;
  }
};

class MediaPlayer {
private:
  AVFormatContext* pFormatContext = nullptr;
  AVCodecContext* videoCodecContext = nullptr;
  AVCodecContext* audioCodecContext = nullptr;
  std::string fileName;
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
  VideoFrame processVideoFrame(AVFrame* frame);
  AudioFrame processAudioFrame(AVFrame* frame);
  void renderVideo();
  void playAudio();
  double lastFrameTime = 0.0;
  const int cacheSize = 64;

  // Holds value of current pts
  int currentPtsInVideoBuffer = 0;
  int currentPtsInAudioBuffer = 0;

  // Cycles through cache size
  int audioCacheIndex = 0;
  int videoCacheIndex = 0;

  bool shouldRenderFrame;
  bool paused = false;
  double currentTime = 0.0;
  double playbackStartTime = 0.0;
  void fillCacheFromPTS(double targetPTS, size_t frameCount);


public:
  MediaPlayer();
  ~MediaPlayer();
  bool loadFile(const std::string fileName);
  void play();
  void pause();
  void seek(double targetTime);
  void syncMedia(double currentTime);
  VideoFrame getVideoFrame();
  AudioFrame getAudioFrame();
  bool isPaused();
  void reset();
  std::vector<VideoFrame> videoFrameCache;
  std::vector<AudioFrame> audioFrameCache;
  double getProgress();
  double getTotalDuration();
  std::vector<double> videoPtsBuffer;
  std::vector<double> audioPtsBuffer;

};

#endif // MEDIAPLAYER_HPP
