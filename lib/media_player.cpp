#include "media_player.hpp"
#include <iostream>
#include <algorithm>


MediaPlayer::MediaPlayer() {

}

MediaPlayer::~MediaPlayer() {
  avformat_close_input(&this->pFormatContext);
  avformat_free_context(this->pFormatContext);
  av_frame_free(&this->videoFrame);
  av_frame_free(&this->audioFrame);
  av_packet_free(&this->packet);
  avcodec_free_context(&this->videoCodecContext);
  avcodec_free_context(&this->audioCodecContext);
}

bool MediaPlayer::initializeStreams() {
  // Loop through each stream
  for (int i = 0; i < this->pFormatContext->nb_streams; i++) {
    // Get current stream and assign to variable
    AVStream* currentStream = this->pFormatContext->streams[i];

    // Get codec variables (describes the properties of the codec used by the stream)
    AVCodecParameters* pLocalCodecParameters = currentStream->codecpar;

    // Find the registered decoder for the corresponding codec id
    const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    // Determine codec/media type
    switch (pLocalCodecParameters->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        std::cout << "Video codec found" << std::endl;
        this->videoStreamIndex = i;
        this->videoCodecParams = pLocalCodecParameters;
        this->videoCodec = pLocalCodec;
        break;
      case AVMEDIA_TYPE_AUDIO:
        std::cout << "Audio codec found" << std::endl;
        this->audioStreamIndex = i;
        this->audioCodecParams = pLocalCodecParameters;
        this->audioCodec = pLocalCodec;
        break;
      default:
        std::cout << "Unknown or unsupported codec type" << std::endl;
        break;
    }

    if (this->audioStreamIndex != -1 && this->videoStreamIndex != -1) {
      return true;
    }
  }

  return false;
}

void MediaPlayer::reset() {
  this->videoStreamIndex = -1;
  this->audioStreamIndex = -1;
  this->videoCodecParams = nullptr;
  this->audioCodecParams = nullptr;
  this->videoCodec = nullptr;
  this->audioCodec = nullptr;
  this->videoCodecContext = nullptr;
  this->audioCodecContext = nullptr;
}

bool MediaPlayer::loadFile(const char* fileName) {
  reset();

  this->fileName = fileName;

  // Holds information about media file format
  this->pFormatContext = avformat_alloc_context();

  // Read header info int pFormatContext
  avformat_open_input(&pFormatContext, this->fileName, NULL, NULL);

  // Initialize streams
  if (!this->initializeStreams()) {
    std::cout << "Something went wrong while initializing streams" << std::endl;
    return false;
  }

  // Load stream info into pFormatContext (codec type, duration, etc)
  avformat_find_stream_info(pFormatContext, NULL);

  // Allocate codec context
  this->videoCodecContext = avcodec_alloc_context3(this->videoCodec);
  this->audioCodecContext = avcodec_alloc_context3(this->audioCodec);

  // Fill codec context with parameters
  avcodec_parameters_to_context(this->videoCodecContext, this->videoCodecParams);
  avcodec_parameters_to_context(this->audioCodecContext, this->audioCodecParams);
  
  // Open codec
  avcodec_open2(this->videoCodecContext, this->videoCodec, NULL);
  avcodec_open2(this->audioCodecContext, this->audioCodec, NULL);

  // Initialize packet (reused for both video and audio)
  this->packet = av_packet_alloc();

  // Initialize frames
  this->videoFrame = av_frame_alloc();
  this->audioFrame = av_frame_alloc();

  int response;

  // Read packets from streams
  std::cout << "Reading packets..." << std::endl;
  while (av_read_frame(this->pFormatContext, this->packet) >= 0) {
    // Determine which codec context to use (video or audio)
    if (this->packet->stream_index == this->videoStreamIndex) {

      // Send raw data packet to decoder
      response = avcodec_send_packet(this->videoCodecContext, this->packet);
      if (response < 0) {
        std::cout << "Failed to send packet" << std::endl;
        return false;
      }

      // Receive raw data frame
      response = avcodec_receive_frame(this->videoCodecContext, this->videoFrame);
      if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        continue;
      } else if (response < 0) {
        std::cout << "Failed to receive frame" << std::endl;
        return false;
      }

      // Process and push to videoBuffer
      processVideoFrame(this->videoFrame);

    } else if (this->packet->stream_index == this->audioStreamIndex) {

      // Send raw data packet to decoder
      response = avcodec_send_packet(this->audioCodecContext, this->packet);
      if (response < 0) {
        std::cout << "Failed to send packet" << std::endl;
        return false;
      }

      // Receive raw data frame
      response = avcodec_receive_frame(this->audioCodecContext, this->audioFrame);
      if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        continue;
      } else if (response < 0) {
        std::cout << "Failed to receive frame" << std::endl;
        return false;
      }

      // Process and push to audioBuffer
      processAudioFrame(this->audioFrame);
    } else {
      std::cout << "Unsupported stream type for packet" << std::endl;
      continue;
    }
  }

  std::cout << "Video queue size: " << this->videoBuffer.size() << std::endl;
  std::cout << "Audio queue size: " << this->audioBuffer.size() << std::endl;

  return this->pFormatContext;
}

void MediaPlayer::processVideoFrame(AVFrame* frame) {
  VideoFrame vf;
  vf.width = frame->width;
  vf.height = frame->height;

  // Copy pixel data for each plane
  for (int i = 0; i < 3; i++) {
    int size = frame->linesize[i] * (i == 0 ? frame->height : frame->height/2);
    vf.data[i] = new uint8_t[size];
    memcpy(vf.data[i], frame->data[i], size);
    vf.linesize[i] = frame->linesize[i];
  }
  
  AVStream* stream = this->pFormatContext->streams[this->videoStreamIndex];
  vf.pts = frame->pts * av_q2d(stream->time_base);

  this->videoBuffer.push_back(vf);
}

void MediaPlayer::processAudioFrame(AVFrame* frame) {
  AudioFrame af;
  //af.data = 
  //af.size = 

  AVStream* stream = this->pFormatContext->streams[this->audioStreamIndex];
  af.pts = frame->pts * av_q2d(stream->time_base);

  this->audioBuffer.push_back(af);
}

void MediaPlayer::play() {
  if (this->paused) {
    this->playbackStartTime = this->playbackStartTime + (this->currentTime - this->lastFrameTime);
    this->lastFrameTime = this->currentTime;
    this->paused = false;
  } 
}

void MediaPlayer::pause() {
  if (!this->paused) {
    this->paused = true;
  }
}

// TODO: More efficient way to do this
void MediaPlayer::seek(double targetTime) {
  // Reset video frame index to closest
  for (size_t i = 0; i < this->videoBuffer.size()-1; ++i) {
    if (this->videoBuffer[i].pts >= targetTime) {
      this->videoBufferIndex = i;
      break;
    }
  }

  // Reset audio frame index to closest
  for (size_t i = 0; i < this->audioBuffer.size()-1; ++i) {
    if (this->audioBuffer[i].pts >= targetTime) {
      this->audioBufferIndex = i;
      break;
    }
  }
}

void MediaPlayer::syncMedia(double currentTime) {
  this->currentTime = currentTime;

  // Get current and elapsed time
  double playbackTime = this->currentTime - this->playbackStartTime;

  // Calculate timing based on video and audio
  double videoPts = this->videoBuffer[0].pts;
  double audioPts = this->audioBuffer[0].pts;

  // Audio and video synchronization tolerance
  const double syncTolerance = 0.05; // 50ms
  bool audioVideoInSync = std::abs(videoPts - audioPts) < syncTolerance;


  // Seek if audio / video not in sync
  if (!audioVideoInSync && !this->videoBuffer.empty() && !this->audioBuffer.empty()) {
    // TODO: Test if this works
    this->seek(videoPts);
  } else if (audioVideoInSync && !this->videoBuffer.empty() && !this->audioBuffer.empty()) {

    // Render video
    VideoFrame frame = this->videoBuffer[this->videoBufferIndex];

    // Determine if should render frame using elapsed time
    this->shouldRenderFrame = ((frame.pts <= playbackTime) || (this->videoBufferIndex == 0)) && !this->paused;


    if (this->shouldRenderFrame) {
      lastFrameTime = this->currentTime;
      this->videoBufferIndex = std::max(0, std::min(this->videoBufferIndex+1, (int)this->videoBuffer.size()-1));
      this->audioBufferIndex = std::max(0, std::min(this->audioBufferIndex+1, (int)this->audioBuffer.size()-1));
    }
  } else {
    std::cout << "Audio and video are out of sync but no seek triggered." << std::endl;
  }
}

VideoFrame MediaPlayer::getVideoFrame() {
  return this->videoBuffer[this->videoBufferIndex];
}

AudioFrame MediaPlayer::getAudioFrame() {
  return this->audioBuffer[this->audioBufferIndex];
}

bool MediaPlayer::shouldRenderMedia() {
  return this->shouldRenderFrame;
}

double MediaPlayer::getTotalDuration() {
  return this->videoBuffer.back().pts - this->videoBuffer.front().pts;
}

bool MediaPlayer::isPaused() {
  return this->paused;
}

double MediaPlayer::getProgress() {
  if (this->videoBuffer.size() == 0) {
    return 0.0;
  }
  
  return std::max(0.0, std::min(100.0, (this->getVideoFrame().pts / this->getTotalDuration()) * 100.0));
}
