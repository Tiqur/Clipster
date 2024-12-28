#ifndef MEDIAPLAYER_HPP
#define MEDIAPLAYER_HPP

#include <string>

class MediaPlayer {
private:
  unsigned int currentTime;
  std::string fileName;

public:
  MediaPlayer();
  ~MediaPlayer();
  bool loadFile(std::string fileName);
  void play();
  void pause();
  void seek(unsigned int time);
};

#endif // MEDIAPLAYER_HPP
