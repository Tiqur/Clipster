#include "video_manager.h"

#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

VideoManager::VideoManager() {
  std::cout << "Starting video manager..." << std::endl;
}

VideoManager::~VideoManager() {
}


bool VideoManager::LoadFrame(const char* filename, int* width_out, int* height_out, unsigned char** data_out) {

  // Open file
  AVFormatContext* av_format_ctx = avformat_alloc_context();
  if (!av_format_ctx) {
    std::cout << "Couldn't create AVFormatContext" << std::endl;
    return false;
  }

  if (avformat_open_input(&av_format_ctx, filename, NULL, NULL) != 0) {
    std::cout << "Couldn't open video file" << std::endl;
    return false;
  }

  int video_stream_index = -1;
  int audio_stream_index = -1;
  AVCodecParameters* video_codec_params = nullptr;
  AVCodecParameters* audio_codec_params = nullptr;
  const AVCodec* video_codec = nullptr;
  const AVCodec* audio_codec = nullptr;

  // Find video and audio streams
  for (int i = 0; i < av_format_ctx->nb_streams; i++) {
    AVCodecParameters* codec_params = av_format_ctx->streams[i]->codecpar;
    if (!codec_params) {
        continue;
    }

    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        continue;
    }

    if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
        video_stream_index = i;
        video_codec_params = codec_params;
        video_codec = codec;
    } else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
        audio_stream_index = i;
        audio_codec_params = codec_params;
        audio_codec = codec;
    }

    if (video_stream_index != -1 && audio_stream_index != -1) {
      break;
    }
  }

  std::cout << "Video stream index: " << video_stream_index << std::endl;
  std::cout << "Audio stream index: " << audio_stream_index << std::endl;

  if (video_stream_index == -1) {
    std::cout << "Couldn't find valid video stream" << std::endl;
    return false;
  }

  if (audio_stream_index == -1) {
    std::cout << "Couldn't find valid audio stream" << std::endl;
    return false;
  }

  AVCodecContext* av_codec_ctx = avcodec_alloc_context3(video_codec);
  if (!av_codec_ctx) {
    std::cout << "Couldn't create AVCodecContext" << std::endl;
    return false;
  }

  if (avcodec_parameters_to_context(av_codec_ctx, video_codec_params) < 0) {
    std::cout << "Couldn't initialize AVCodecContext" << std::endl;
    return false;
  }

  if (avcodec_open2(av_codec_ctx, video_codec, NULL) < 0) {
    std::cout << "Couldn't open codec" << std::endl;
    return false;
  }

  AVFrame* av_frame = av_frame_alloc();
  if (!av_frame) {
    std::cout << "Couldn't allocate frame" << std::endl;
    return false;
  }

  AVPacket* av_packet = av_packet_alloc();
  if (!av_packet) {
    std::cout << "Couldn't allocate packet" << std::endl;
    return false;
  }

  int response;
  while (av_read_frame(av_format_ctx, av_packet) >= 0) {
    if (av_packet->stream_index != video_stream_index) { // Modify later for audio
      continue;
    }

    response = avcodec_send_packet(av_codec_ctx, av_packet);
    if (response < 0) {
      std::cout << "Failed to send packet" << std::endl;
      return false;
    }

    response = avcodec_receive_frame(av_codec_ctx, av_frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      continue;
    } else if (response < 0) {
      std::cout << "Failed to receive frame" << std::endl;
      return false;
    }

    av_packet_unref(av_packet);
    break;
  }

  if (response < 0 || av_frame->width == 0 || av_frame->height == 0) {
    std::cout << "Invalid frame data" << std::endl;
    return false;
  }

  unsigned char* data = new unsigned char[av_frame->width * av_frame->height * 3];

  // Use Shader for this
  // Convert to RGB
  for (int y = 0; y < av_frame->height; y++) {
    for (int x = 0; x < av_frame->width; x++) {
      data[y*av_frame->width * 3 + x + 0] = av_frame->data[0][y * av_frame->linesize[0] + x];
      data[y*av_frame->width * 3 + x + 1] = av_frame->data[0][y * av_frame->linesize[0] + x];
      data[y*av_frame->width * 3 + x + 2] = av_frame->data[0][y * av_frame->linesize[0] + x];
    }
  }

  *width_out = av_frame->width;
  *height_out = av_frame->height;
  *data_out = data;

  avformat_close_input(&av_format_ctx);
  avformat_free_context(av_format_ctx);
  av_frame_free(&av_frame);
  av_packet_free(&av_packet);
  avcodec_free_context(&av_codec_ctx);

  return true;
}
