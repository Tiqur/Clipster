#include "media_player.hpp"
#include <iostream>


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

      // Process and push to videoQueue
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

      // Process and push to audioQueue
      processAudioFrame(this->audioFrame);
    } else {
      std::cout << "Unsupported stream type for packet" << std::endl;
      continue;
    }
  }

  std::cout << "Video queue size: " << this->videoQueue.size() << std::endl;
  std::cout << "Audio queue size: " << this->audioQueue.size() << std::endl;
  return this->pFormatContext;
}

void MediaPlayer::processVideoFrame(AVFrame* frame) {
  VideoFrame vf;
  vf.width = frame->width;
  vf.height = frame->height;
  //vf.pts = frame->pts;

  // TODO: Be sure to free this
  vf.data = new unsigned char[frame->width * frame->height * 3];

  for (int y = 0; y < frame->height; y++) {
    for (int x = 0; x < frame->width; x++) {
      int yi = y * frame->linesize[0] + x;
      int ui = (y/2) * frame->linesize[1] + (x/2);
      int vi = (y/2) * frame->linesize[2] + (x/2);

      int Y = frame->data[0][yi];
      int U = frame->data[1][ui];
      int V = frame->data[2][vi];

      // RGB conversion
      int R = Y + 1.402 * (V-128);
      int G = Y - 0.344136 * (U-128) - 0.714136 * (V-128);
      int B = Y + 1.772 * (U-128);

      // Clamp values
      R = std::min(255, std::max(0, R));
      G = std::min(255, std::max(0, G));
      B = std::min(255, std::max(0, B));

      int idx = (y * frame->width + x) * 3;
      vf.data[idx] = R;
      vf.data[idx + 1] = G;
      vf.data[idx + 2] = B;
    }
  }
 
  this->videoQueue.push(vf);
}

void MediaPlayer::processAudioFrame(AVFrame* frame) {
  AudioFrame af;
  //af.data = 
  //af.size = 
  //af.pts = 

  this->audioQueue.push(af);
}

void MediaPlayer::play() {

}

void MediaPlayer::pause() {

}

void MediaPlayer::seek(unsigned int time) {

}
