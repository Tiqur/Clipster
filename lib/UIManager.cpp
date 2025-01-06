#include "UIManager.hpp"
#include <iostream>

UIManager::UIManager() {

}

UIManager::~UIManager() {

}

bool UIManager::init(MediaPlayer* mediaPlayer, int* windowWidth, int* windowHeight, GLFWwindow* window, char* openglVersion, float* videoAspectRatio) {
    if (!windowWidth || !windowHeight || !window || !openglVersion) {
        std::cerr << "Invalid arguments passed to UIManager::init!" << std::endl;
        return false;
    }

    this->mediaPlayer = mediaPlayer;
    this->p_open = true;
    this->glfwWindow = window;
    this->openglVersion = openglVersion;
    this->windowWidth = windowWidth;
    this->windowHeight = windowHeight;
    this->videoAspectRatio = videoAspectRatio;


    // Initialize ImGui
    IMGUI_CHECKVERSION();
    std::cout << "Creating ImGui context..." << std::endl;
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    std::cout << "Initializing ImGui GLFW backend..." << std::endl;
    if (!ImGui_ImplGlfw_InitForOpenGL(this->glfwWindow, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend!" << std::endl;
        return false;
    }

    std::cout << "Initializing ImGui OpenGL backend..." << std::endl;
    if (!ImGui_ImplOpenGL3_Init(this->openglVersion)) {
        std::cerr << "Failed to initialize ImGui OpenGL backend!" << std::endl;
        return false;
    }

    // Setup window flags
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoTitleBar;

    this->sideBarWindowFlags = windowFlags;
    this->bottomBarWindowFlags = windowFlags;
    this->seekPreviewWindowFlags = windowFlags;

    return true;}

void UIManager::render() {
  // New frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Render components
  renderToolBar();
  renderSideBar();
  renderBottomBar();
  renderVideoPlayer();

  // Render to window
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::renderToolBar() {
  ImGui::BeginMainMenuBar();
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open")) {}
    if (ImGui::MenuItem("Save")) {}
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Edit")) {
    if (ImGui::MenuItem("Undo")) {}
    if (ImGui::MenuItem("Redo")) {}
    ImGui::EndMenu();
  }

  // Update height
  this->toolBarHeight = ImGui::GetWindowSize()[1];
  ImGui::EndMainMenuBar();
}

void UIManager::renderSideBar() {
  this->sideBarWidth = std::min(std::max((int)(*this->windowWidth*0.2), 200), 400);
  ImGui::SetNextWindowPos(ImVec2(0, this->toolBarHeight));
  ImGui::SetNextWindowSize(ImVec2(this->sideBarWidth, (float)*this->windowHeight - this->toolBarHeight));
  ImGui::Begin("Sidebar", &this->p_open, this->sideBarWindowFlags);
  ImGui::Text("Clips");
  ImGui::SeparatorText("Clips");

  //if (ImGui::Button("Create Clip")) {
  //  std::cout << "[INFO]: Create Clip" << std::endl;
  //  this->clips.push_back(Clip(0, 10, "Clip"));
  //}

  //for (int i = 0; i < clips.size(); i++) {
  //  Clip c = clips[i];

  //  ImGui::InputText(("##ClipName" + std::to_string(i)).c_str(), c.name, c.buffer_size);

  //  ImGui::SameLine(); 
  //  if (ImGui::Button(("Delete##" + std::to_string(i)).c_str())) {
  //    clips.erase(clips.begin() + i);
  //    break;
  //  }
  ImGui::End();
}

void UIManager::renderSeekBar() {
  static bool g_seeking = false;
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

  ImVec2 barPos = ImGui::GetCursorScreenPos();
  ImVec2 barSize = ImVec2(-FLT_MIN, 20.0f);
  
  // Calculate bar width (accounting for padding)
  double barWidth = ImGui::GetContentRegionAvail().x;
  
  // Handle mouse interaction
  ImVec2 mousePos = ImGui::GetIO().MousePos;
  bool mouseDown = ImGui::IsMouseDown(0);

  // Pre-calculate target time in case of seek
  double totalDuration = this->mediaPlayer->getTotalDuration();
  double relative_x = std::min(std::max((mousePos.x - barPos.x) / barWidth, 0.0), barWidth);
  double targetTime = std::min(totalDuration, std::max(relative_x * totalDuration, 0.0));

  // Draw the progress bar
  static float g_progress = 0.0f;
  g_progress = (mouseDown && ImGui::IsItemClicked()) ? (targetTime / totalDuration) : this->mediaPlayer->getProgress() / 100.0;
  ImGui::ProgressBar(g_progress, barSize, "");
  
  // Check if mouse is over the progress bar
  bool hovered = mousePos.y >= barPos.y && mousePos.y <= barPos.y + barSize.y && mousePos.x >= barPos.x && mousePos.x <= barPos.x + barWidth;

  if (hovered) {
    if (ImGui::IsMouseClicked(0)) {
      g_seeking = true;
    }
    renderSeekPreview(barPos.y);
  }
  
  if (g_seeking && mouseDown) {
    this->mediaPlayer->seek(targetTime);
    this->mediaPlayer->pause();
  }
  
  if (!mouseDown) {
      g_seeking = false;
  }

  ImGui::PopStyleColor(2);

  // TODO: Convert timestamp to percent
  std::vector<float> bookmarks = { 0.25f, 0.5f, 0.75f };
  renderBookmarks(barPos, bookmarks);
  renderClipBoxes(barPos, this->clips, (int)totalDuration*1000);

  // TODO: Move to helper function
  double currentFrameTime = mediaPlayer->getVideoFrame().pts;
  double currentTimeInMicroseconds = currentFrameTime * 1000000;
  int hours = (int)(currentTimeInMicroseconds / 3600000000);
  int minutes = (int)((currentTimeInMicroseconds - hours * 3600000000) / 60000000);
  int seconds = (int)((currentTimeInMicroseconds - hours * 3600000000 - minutes * 60000000) / 1000000);
  int milliseconds = (int)((currentTimeInMicroseconds - hours * 3600000000 - minutes * 60000000 - seconds * 1000000) / 1000);

  if (std::isnan(currentFrameTime) || currentFrameTime < 0) {
    ImGui::Text("Invalid timestamp");
    return;
  }

  ImGui::Text("%02d:%02d:%02d:%03d", hours, minutes, seconds, milliseconds);
}

void UIManager::renderBottomBar() {
  this->bottomBarHeight = (int)std::min(std::max(*this->windowHeight*0.25, 100.0), 200.0)-this->toolBarHeight;
  this->bottomBarWidth = *this->windowWidth-this->sideBarWidth;
  ImGui::SetNextWindowPos(ImVec2(this->sideBarWidth, *this->windowHeight-this->bottomBarHeight));
  ImGui::SetNextWindowSize(ImVec2(this->bottomBarWidth, this->bottomBarHeight));
  ImGui::Begin("Bottom Bar", &this->p_open, this->bottomBarWindowFlags);

  static float g_progress = 0.0f;

  renderSeekBar();

  // Render media buttons
  renderMediaButtons();

  ImGui::End();
}

void UIManager::renderSeekPreview(int seekBarYPos) {
  float verticalPadding = 10.0f;
  float previewWindowHeight = 200.0f;

  ImVec2 mouse_pos = ImGui::GetIO().MousePos;
  ImVec2 previewWindowSize = ImVec2(previewWindowHeight, previewWindowHeight/(*this->videoAspectRatio));
  ImGui::SetNextWindowPos(ImVec2(mouse_pos.x-previewWindowSize.x/2, seekBarYPos-previewWindowSize.y-verticalPadding));
  ImGui::SetNextWindowSize(previewWindowSize);
  ImGui::Begin("SeekPreview", &p_open, this->seekPreviewWindowFlags);
  ImGui::End();
}


void UIManager::renderClipBoxes(ImVec2 bar_position, std::vector<Clip>& clips, int video_duration_ms) {
    static int dragging_handle = -1;
    static int active_clip_index = -1;
    
    ImVec2 bar_size = ImGui::GetItemRectSize();
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool mouse_down = ImGui::IsMouseDown(0);
    
    // If mouse is released, reset dragging state
    if (!mouse_down) {
        dragging_handle = -1;
        active_clip_index = -1;
    }
    
    for (int i = 0; i < clips.size(); i++) {
        Clip& clip = clips[i];
        float x_position_start = bar_position.x + ((float)clip.time_start / video_duration_ms) * bar_size.x;
        float x_position_end = bar_position.x + ((float)clip.time_end / video_duration_ms) * bar_size.x;
        
        ImVec2 top_left = ImVec2(x_position_start, bar_position.y);
        ImVec2 bottom_right = ImVec2(x_position_end, bar_position.y + bar_size.y);
        
        // Draw clip box
        ImGui::GetWindowDrawList()->AddRectFilled(top_left, bottom_right, IM_COL32(0, 255, 0, 50));
        
        // Handle config
        float handle_h_padding = 3.0f;
        float handle_v_padding = 1.0f;
        auto handle_color = IM_COL32(0, 255, 0, 255);
        
        ImVec2 left_handle_min(x_position_start - handle_h_padding, bar_position.y - handle_v_padding);
        ImVec2 left_handle_max(x_position_start + handle_h_padding, bottom_right.y + handle_v_padding);
        ImVec2 right_handle_min(x_position_end - handle_h_padding, bar_position.y - handle_v_padding);
        ImVec2 right_handle_max(x_position_end + handle_h_padding, bottom_right.y + handle_v_padding);
        
        // Draw handles
        ImGui::GetWindowDrawList()->AddRectFilled(left_handle_min, left_handle_max, handle_color);
        ImGui::GetWindowDrawList()->AddRectFilled(right_handle_min, right_handle_max, handle_color);
        
        // Check for handle interaction
        if (dragging_handle == -1 && ImGui::IsMouseClicked(0)) {
            // Left handle
            if (mouse_pos.x >= left_handle_min.x && mouse_pos.x <= left_handle_max.x &&
                mouse_pos.y >= left_handle_min.y && mouse_pos.y <= left_handle_max.y) {
                dragging_handle = 0;
                active_clip_index = i;
            }
            // Right handle
            else if (mouse_pos.x >= right_handle_min.x && mouse_pos.x <= right_handle_max.x &&
                     mouse_pos.y >= right_handle_min.y && mouse_pos.y <= right_handle_max.y) {
                dragging_handle = 1;
                active_clip_index = i;
            }
        }
        
        // Handle dragging
        if (mouse_down && active_clip_index == i && dragging_handle != -1) {
            float relative_x = (mouse_pos.x - bar_position.x) / bar_size.x;
            int new_time = relative_x * video_duration_ms;
            
            new_time = std::min(std::max(new_time, 0), video_duration_ms);
            
            if (dragging_handle == 0) {
                // Left handle - update start time
                clip.time_start = std::min(new_time, clip.time_end);
            } else {
                // Right handle - update end time
                clip.time_end = std::max(new_time, clip.time_start);
            }
        }
    }
}

void UIManager::renderVideoPlayer() {

}

void UIManager::renderBookmarks(ImVec2 barPos, std::vector<float> bookmarks) {
  for (float bookmark : bookmarks) {
      ImVec2 bar_size = ImGui::GetItemRectSize();
      float x_position = barPos.x + bookmark * bar_size.x;
      ImVec2 start = ImVec2(x_position, barPos.y);
      ImVec2 end = ImVec2(x_position, barPos.y + bar_size.y);
      ImGui::GetWindowDrawList()->AddLine(start, end, IM_COL32(255, 0, 0, 255), 2.0f);
  }
}


int UIManager::getToolBarHeight() {
  return this->toolBarHeight;
}

void UIManager::renderMediaButtons() {
  static bool paused = false;
  paused = mediaPlayer->isPaused();

  const char* label = paused ? "Play" : "Pause";

  ImGuiStyle& style = ImGui::GetStyle();

  float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
  float avail = ImGui::GetContentRegionAvail().x;
  float off = (avail - size) * 0.5f;

  if (off > 0.0f)
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

  if(ImGui::Button(label)) {
    paused = !paused;
    if (paused) {
      this->mediaPlayer->pause();
    } else {
      this->mediaPlayer->play();
    }
  }
}
