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

bool MediaPlayer::loadFile(const std::string fileName) {
  reset();

  this->fileName = fileName;

  // Holds information about media file format
  this->pFormatContext = avformat_alloc_context();

  // Read header info int pFormatContext
  avformat_open_input(&pFormatContext, this->fileName.c_str(), NULL, NULL);

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

      this->videoPtsBuffer.push_back(this->videoFrame->pts * av_q2d(this->pFormatContext->streams[this->videoStreamIndex]->time_base));

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

      this->audioPtsBuffer.push_back(this->audioFrame->pts);

    } else {
      std::cout << "Unsupported stream type for packet" << std::endl;
      continue;
    }
  }

  //std::cout << "Video queue size: " << this->videoBuffer.size() << std::endl;
  //std::cout << "Audio queue size: " << this->audioBuffer.size() << std::endl;
  std::cout << "Video Pts: " << this->videoPtsBuffer.size() << std::endl;
  std::cout << "Audio Pts: " << this->audioPtsBuffer.size() << std::endl;
  

  // Fill cache with initial frames
  this->fillCacheFromPTS(0, this->cacheSize);

  //for (int i = 0; i < this->videoFrameCache.size(); i++)
  //  std::cout << i << this->videoFrameCache[i].pts << ", ";
  //std::cout << std::endl;

  //this->fillCacheFromPTS(this->videoPtsBuffer[50], this->cacheSize);
  //for (int i = 0; i < this->videoFrameCache.size(); i++)
  //  std::cout << i << this->videoFrameCache[i].pts << ", ";
  //std::cout << std::endl;


  return this->pFormatContext;
}

VideoFrame MediaPlayer::processVideoFrame(AVFrame* frame) {
  VideoFrame vf;
  vf.width = frame->width;
  vf.height = frame->height;

  // Copy pixel data for each plane 
  for (int i = 0; i < 3; i++) {
    int size = frame->linesize[i] * (i == 0 ? frame->height : frame->height/2);
    vf.data[i].resize(size);
    memcpy(vf.data[i].data(), frame->data[i], size);
    vf.linesize[i] = frame->linesize[i];
  }
  
  AVStream* stream = this->pFormatContext->streams[this->videoStreamIndex];
  vf.pts = frame->pts * av_q2d(stream->time_base);

  return vf;
}

AudioFrame MediaPlayer::processAudioFrame(AVFrame* frame) {
  AudioFrame af;
  //af.data = 
  //af.size = 

  AVStream* stream = this->pFormatContext->streams[this->audioStreamIndex];
  af.pts = frame->pts * av_q2d(stream->time_base);

  return af;
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
  for (size_t i = 0; i < this->videoPtsBuffer.size()-1; ++i) {
    if (this->videoPtsBuffer[i] >= targetTime) {
      this->currentPtsInVideoBuffer = i;
      break;
    }
  }

  // Reset audio frame index to closest
  for (size_t i = 0; i < this->audioPtsBuffer.size()-1; ++i) {
    if (this->audioPtsBuffer[i] >= targetTime) {
      this->currentPtsInAudioBuffer = i;
      break;
    }
  }

  this->lastFrameTime = this->currentTime;
  this->playbackStartTime = this->currentTime - targetTime;
  this->fillCacheFromPTS(this->videoPtsBuffer[this->currentPtsInVideoBuffer], this->cacheSize);
}

void MediaPlayer::syncMedia(double currentTime) {
  this->currentTime = currentTime;

  // Get current and elapsed time
  double playbackTime = this->currentTime - this->playbackStartTime;

  // Calculate timing based on video and audio
  double videoPts = this->getVideoFrame().pts;
  double audioPts = this->getAudioFrame().pts;

  // Audio and video synchronization tolerance
  const double syncTolerance = 0.05; // 50ms
  bool audioVideoInSync = std::abs(videoPts - audioPts) < syncTolerance;

  //if (!audioVideoInSync)
  //  this->seek(videoPts);

  // Determine if should render frame using elapsed time
  VideoFrame frame = this->getVideoFrame();
  this->shouldRenderFrame = ((frame.pts <= playbackTime) || (this->videoCacheIndex == 0)) && !this->paused;

  if (this->shouldRenderFrame) {
    lastFrameTime = this->currentTime;

    // TODO: double buffer cache and swap pointers for efficiency? (separate thread)
    if ((float)this->videoCacheIndex >= this->cacheSize-1) {
      std::cout << "Reached end of cache,  Refilling..." << std::endl;

      size_t nextIndex = this->currentPtsInVideoBuffer + 1;

      if (nextIndex >= this->videoPtsBuffer.size()) {
          this->pause();
          return;
      }
    
      double startPTS = this->videoPtsBuffer[nextIndex];
      this->fillCacheFromPTS(startPTS, this->cacheSize);
      std::cout << "PTS index: " << this->currentPtsInVideoBuffer << std::endl;
      std::cout << "PTS: " << this->videoPtsBuffer[this->currentPtsInVideoBuffer] << std::endl;
      this->videoCacheIndex = 0;
      this->audioCacheIndex = 0;
    }


    this->videoCacheIndex = std::clamp(this->videoCacheIndex+1, 0, std::min((int)this->videoFrameCache.size(), this->cacheSize)-1);
    this->audioCacheIndex = std::clamp(this->audioCacheIndex+1, 0, std::min((int)this->audioFrameCache.size(), this->cacheSize)-1);
    this->currentPtsInVideoBuffer = std::clamp(this->currentPtsInVideoBuffer+1, 0, (int)this->videoPtsBuffer.size()-1);
    this->currentPtsInAudioBuffer = std::clamp(this->currentPtsInAudioBuffer+1, 0, (int)this->audioPtsBuffer.size()-1);
  }
}

void MediaPlayer::fillCacheFromPTS(double targetPTS, size_t frameCount) {
  std::cout << "TargetPTS: " << targetPTS << std::endl;
    
    // Convert pts to the stream's timebase
    AVRational time_base = this->pFormatContext->streams[videoStreamIndex]->time_base;
    int64_t seekPTS = (int64_t)(targetPTS * time_base.den / time_base.num);

    if (targetPTS > videoPtsBuffer.back() || targetPTS < videoPtsBuffer.front()) {
        fprintf(stderr, "Invalid PTS value for seeking\n");
        return;
    }

    // Seek to closest keyframe that is earlier than PTS
    if (av_seek_frame(this->pFormatContext, videoStreamIndex, seekPTS, AVSEEK_FLAG_BACKWARD) < 0) {
      fprintf(stderr, "Error while seeking.\n");
      return;
    }
    
    // Clear decoder buffers
    avcodec_flush_buffers(this->videoCodecContext);
    avcodec_flush_buffers(this->audioCodecContext);

    // Clear Caches
    for (auto& frame : videoFrameCache)
      for (int i = 0; i < 3; i++)
        frame.data[i].clear();

    for (auto& frame : audioFrameCache)
      frame.data.clear();

    this->videoFrameCache.clear();
    this->audioFrameCache.clear();
    
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (!packet || !frame) {
        fprintf(stderr, "Failed to allocate packet or frame.\n");
        av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    
    size_t framesFound = 0;
    bool foundFirstFrame = false;
    int ret;
    

    while (av_read_frame(this->pFormatContext, packet) >= 0) {
      if (packet->stream_index == videoStreamIndex) {
        ret = avcodec_send_packet(this->videoCodecContext, packet);
        if (ret < 0) {
          fprintf(stderr, "Error sending packet for decoding\n");
          break;
        }

        while ((ret = avcodec_receive_frame(this->videoCodecContext, frame)) == 0) {
          if (!foundFirstFrame && frame->best_effort_timestamp >= seekPTS) {
            foundFirstFrame = true;
          }

          if (foundFirstFrame) {
            VideoFrame vf = this->processVideoFrame(frame);
            this->videoFrameCache.push_back(vf);
            framesFound++;

            if (framesFound >= frameCount) {
              av_packet_unref(packet);
              goto cleanup;  // Break out of both loops
            }
          }
        }

        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
          fprintf(stderr, "Error receiving frame\n");
          break;
        }
    } else if (packet->stream_index == audioStreamIndex) {
        ret = avcodec_send_packet(this->audioCodecContext, packet);
        if (ret < 0) {
          fprintf(stderr, "Error sending audio packet for decoding\n");
          break;
        }

        while ((ret = avcodec_receive_frame(this->audioCodecContext, frame)) == 0) {
          AudioFrame af = this->processAudioFrame(frame);
          this->audioFrameCache.push_back(af);
        }

        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
          fprintf(stderr, "Error receiving audio frame\n");
          break;
        }
      }
      av_packet_unref(packet);
    }

cleanup:
    av_packet_free(&packet);
    av_frame_free(&frame);
}

VideoFrame MediaPlayer::getVideoFrame() {
  return this->videoFrameCache[this->videoCacheIndex];
}

AudioFrame MediaPlayer::getAudioFrame() {
  return this->audioFrameCache[this->audioCacheIndex];
}

double MediaPlayer::getTotalDuration() {
  return this->videoPtsBuffer.back() - this->videoPtsBuffer.front();
}

bool MediaPlayer::isPaused() {
  return this->paused;
}

double MediaPlayer::getProgress() {
  if (this->videoFrameCache.size() < 1) {
    return 0.0;
  }
  
  return std::max(0.0, std::min(100.0, (this->getVideoFrame().pts / this->getTotalDuration()) * 100.0));
}
