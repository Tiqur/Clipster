#ifndef UIMANAGER_HPP
#define UIMANAGER_HPP

#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"
#include "media_player.hpp"
#include <vector>
#include <iostream>

static void HelpMarker(const char* desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::BeginItemTooltip())
  {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}


class Clip {
public:
  double time_start;
  double time_end;
  int buffer_size = 256;
  char* name = new char[this->buffer_size];

  Clip(double time_start, double time_end, char* name) {
    this->time_start = time_start;
    this->time_end = time_end;
    this->setName(name);
    std::cout << name << " " << this->name << std::endl;
  }

  ~Clip() {
    //delete[] name;
  }

  void setName(char* newName) {
    if (newName) {
      strncpy(name, newName, this->buffer_size - 1);
      name[this->buffer_size - 1] = '\0';
    } else {
      name[0] = '\0';
    }
  }
};


class UIManager {
private:
  float* videoAspectRatio;
  char* openglVersion;
  MediaPlayer* mediaPlayer;
  GLFWwindow* glfwWindow;
  ImGuiWindowFlags sideBarWindowFlags = 0;
  ImGuiWindowFlags bottomBarWindowFlags = 0;
  ImGuiWindowFlags seekPreviewWindowFlags = 0;
  bool debug = false;
  int* windowHeight;
  int* windowWidth;
  std::vector<Clip> clips;

  int toolBarHeight;
  int sideBarWidth;
  int bottomBarWidth;
  int bottomBarHeight;
  bool p_open;

  void renderToolBar();
  void renderSideBar();
  void renderBottomBar();
  void renderVideoPlayer();
  void renderSeekBar();
  void renderSeekPreview(int seekBarYPos);
  void renderBookmarks(ImVec2 barPos, std::vector<float> bookmarks);
  void renderClipBoxes(ImVec2 bar_position, std::vector<Clip>& clips, double video_duration);
  void renderMediaButtons();
  double findNearestPts(double x);
  void renderClipCreator(ImVec2 barPos);


public:
  UIManager();
  ~UIManager();
  bool init(MediaPlayer* mediaPlayer, int* windowWidth, int* windowHeight, GLFWwindow* window, char* openglVersion, float* videoAspectRatio);
  void setDebug(bool b);
  void render();
  int getToolBarHeight();
};

#endif // UIMANAGER_HPP
