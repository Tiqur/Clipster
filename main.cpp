#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include"video_manager.h"
#include"shader_utils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>


const int DEBUG = true;
const char WINDOW_TITLE[] = "Rewind";
const int INITIAL_WIDTH = 800;
const int INITIAL_HEIGHT = 600;

class Clip {
public:
  int time_start;
  int time_end;
  char* name;

  Clip(int time_start, int time_end, char* name) {
    this->time_start = time_start;
    this->time_end = time_end;
    this->name = name;
  }
};

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

class Rewind {
  public:
    Rewind() {

    }

    int run() {
      VideoManager vm;

      const char* filename = "video.mp4";
      int frame_width = 0;
      int frame_height = 0;
      unsigned char* frame_data = nullptr;
      vm.LoadFrame(filename, &frame_width, &frame_height, &frame_data);

      if (frame_width == 0 || frame_height == 0) {
        std::cout << "Invalid frame dimensions" << std::endl;
        return -1;
      }

      const float aspect_ratio = (float)frame_width / frame_height;

      if (frame_data == nullptr) {
        std::cout << "Error: Frame data is null" << std::endl;
        return -1;
      }

      // Initialize GLFW
      if (!glfwInit()) {
          std::cerr << "Failed to initialize GLFW" << std::endl;
          return -1;
      }

      // Set GLFW context version to 3.3
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

      // Initialize vertices
      GLfloat vertices[] = 
      {
      // First triangle
      0.0f, 0.0f, 0.0f,  0.0f, 1.0f,  // bottom left
      0.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // bottom right 
      0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  // top left

      // Second triangle
      0.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // bottom right
      0.0f, 0.0f, 0.0f,  1.0f, 0.0f,  // top right
      0.0f, 0.0f, 0.0f,  0.0f, 0.0f   // top left
      };

      // Create a window
      GLFWwindow* window = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, WINDOW_TITLE, NULL, NULL);
      if (!window) {
          std::cerr << "Failed to create GLFW window" << std::endl;
          glfwTerminate();
          return -1;
      }

      // Make the OpenGL context current
      glfwMakeContextCurrent(window);

      // Initialize GLEW
      glewExperimental = GL_TRUE; // Ensure GLEW uses modern OpenGL techniques
      if (glewInit() != GLEW_OK) {
          std::cerr << "Failed to initialize GLEW" << std::endl;
          return -1;
      }

      // Set the viewport
      glViewport(0, 0, 800, 600);

      // Load Shaders
      GLuint shaderProgram = createShaderProgram("../shaders/vertex_shader.glsl", "../shaders/fragment_shader.glsl");


      GLuint VAO, VBO;
      glGenVertexArrays(1, &VAO);

      glGenBuffers(1, &VBO);
      glBindVertexArray(VAO);

      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);  // position
      glEnableVertexAttribArray(0);

      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));  // texture coords
      glEnableVertexAttribArray(2);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);



      // Generate texture
      unsigned int texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame_data);
      glGenerateMipmap(GL_TEXTURE_2D);



      // Register the framebuffer size callback
      glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

      // Initialize ImGUI
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      ImGui::StyleColorsDark();
      ImGui_ImplGlfw_InitForOpenGL(window, true);
      ImGui_ImplOpenGL3_Init("#version 330");

      ImGuiWindowFlags window_flags = 0;
      window_flags |= ImGuiWindowFlags_NoScrollbar;
      window_flags |= ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoResize;
      window_flags |= ImGuiWindowFlags_NoCollapse;
      window_flags |= ImGuiWindowFlags_NoTitleBar;
      static bool p_open = false;

      // Main render loop
      while (!glfwWindowShouldClose(window)) {
          int width, height, sideBarWidth, bottomBarHeight;
          glfwGetWindowSize(window, &width, &height);
          sideBarWidth = std::min(std::max((int)(width*0.2), 200), 400);

          // Render
          glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
          glClear(GL_COLOR_BUFFER_BIT);
          glUseProgram(shaderProgram);
          glBindTexture(GL_TEXTURE_2D, texture);
          glBindVertexArray(VAO);
          glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices)/sizeof(vertices[0])/5);

          // New frame
          ImGui_ImplOpenGL3_NewFrame();
          ImGui_ImplGlfw_NewFrame();
          ImGui::NewFrame();

          // Process input
          handleInput(window);

          // Render Toolbar
          int toolBarHeight;
          renderToolbar(&toolBarHeight);

          // Debug
          if (DEBUG) {
            ImGui::ShowMetricsWindow();
            ImGui::ShowDebugLogWindow();
            ImGui::ShowDemoWindow();
          }

          // Edit vertices
          bottomBarHeight = (int)std::min(std::max(height*0.25, 100.0), 200.0);

          int availableWidth = width-sideBarWidth;
          int availableHeight = height-bottomBarHeight;

          // Render Sidebar
          renderSidebar(width, height, p_open, toolBarHeight, sideBarWidth, window_flags);
          renderBottomBar(p_open, window_flags, height, bottomBarHeight-toolBarHeight, sideBarWidth, availableWidth);

          // Maintain aspect ratio
          float screenWidth, screenHeight;
          getScreenDimensions(&screenWidth, &screenHeight, availableHeight, availableWidth, aspect_ratio);

          // Update triangle vertices
          updateScreenVertices(vertices, width, height, sideBarWidth, toolBarHeight, screenWidth, screenHeight, availableWidth, availableHeight);

          glBindBuffer(GL_ARRAY_BUFFER, VBO);
          glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

          // Renders the ImGUI elements
          ImGui::Render();
          ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

          // Swap buffers and poll events
          glfwSwapBuffers(window);
          glfwPollEvents();
      }

      // Clean up and exit
      glfwDestroyWindow(window);
      glfwTerminate();

      glDeleteVertexArrays(1, &VAO);
      glDeleteBuffers(1, &VBO);
      glDeleteProgram(shaderProgram);
      return 0;
    }

  private:
    void handleInput(GLFWwindow* window) {
      if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
          glfwSetWindowShouldClose(window, true);
    }

    float normalizeToCoordinateSpace(float a, float b) {
      return (2.0*a)/b-1;
    }

    void renderToolbar(int* toolBarHeight) {
      // Toolbar
      ImGui::BeginMainMenuBar();
      if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("New")) {
          }
          if (ImGui::MenuItem("Open")) {
          }
          if (ImGui::MenuItem("Save")) {
          }
          ImGui::EndMenu();
      }
      // Edit menu
      if (ImGui::BeginMenu("Edit")) {
          if (ImGui::MenuItem("Undo")) {
          }
          if (ImGui::MenuItem("Redo")) {
          }
          ImGui::EndMenu();
      }
      // Help menu
      if (ImGui::BeginMenu("Help")) {
          if (ImGui::MenuItem("About")) {
          }
          ImGui::EndMenu();
      }
      *toolBarHeight = ImGui::GetWindowSize()[1];
      ImGui::EndMainMenuBar();
    }

    void renderSidebar(int width, int height, bool p_open, int toolBarHeight, int sideBarWidth, ImGuiWindowFlags window_flags) {
      ImGui::SetNextWindowPos(ImVec2(0, toolBarHeight));
      ImGui::SetNextWindowSize(ImVec2(sideBarWidth, height-toolBarHeight));
      ImGui::Begin("Sidebar", &p_open, window_flags);
      ImGui::Text("Clips");
      ImGui::SeparatorText("Clips");
      if (ImGui::Button("Export All"))
        std::cout << "[INFO]: Export All" << std::endl;
      ImGui::SeparatorText("Settings");
      static int volume = 50;
      ImGui::SliderInt("Volume", &volume, 0, 100);
      ImGui::SameLine(); HelpMarker("CTRL+click to input value.");
      ImGui::End();
    }

    void renderBottomBar(bool p_open, ImGuiWindowFlags window_flags, int height, int bottomBarHeight, int sideBarWidth, int bottomBarWidth) {
      ImGui::SetNextWindowPos(ImVec2(sideBarWidth, height-bottomBarHeight));
      ImGui::SetNextWindowSize(ImVec2(bottomBarWidth, bottomBarHeight));
      ImGui::Begin("Bottom Bar", &p_open, window_flags);

      static float g_progress = 0.0f;

      // TODO: Convert timestamp to percent
      std::vector<float> bookmarks = { 0.25f, 0.5f, 0.75f };
      renderSeekBar(&g_progress, bookmarks);

      // Render media buttons
      renderMediaButtons(bottomBarWidth);

      ImGui::End();
    }

    void renderSeekBar(float* g_progress, std::vector<float> bookmarks) {
      static bool g_seeking = false;
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));

      ImVec2 bar_position = ImGui::GetCursorScreenPos();
      ImVec2 bar_size = ImVec2(-FLT_MIN, 20.0f);
      
      // Draw the progress bar
      ImGui::ProgressBar(*g_progress, bar_size, "");
      
      // Calculate bar width (accounting for padding)
      float bar_width = ImGui::GetContentRegionAvail().x;
      
      // Handle mouse interaction
      ImVec2 mouse_pos = ImGui::GetIO().MousePos;
      bool mouse_down = ImGui::IsMouseDown(0);
      
      // Check if mouse is over the progress bar
      bool hovered = mouse_pos.y >= bar_position.y && mouse_pos.y <= bar_position.y + bar_size.y && mouse_pos.x >= bar_position.x && mouse_pos.x <= bar_position.x + bar_width;

      if (hovered && ImGui::IsMouseClicked(0)) {
          g_seeking = true;
      }
      
      if (g_seeking && mouse_down) {
          float relative_x = (mouse_pos.x - bar_position.x) / bar_width;
          
          if (relative_x < 0.0f) relative_x = 0.0f;
          if (relative_x > 1.0f) relative_x = 1.0f;
          
          *g_progress = relative_x;
      }
      
      if (!mouse_down) {
          g_seeking = false;
      }

      ImGui::PopStyleColor(2);

      std::vector<Clip> clips = { 
          Clip(0, 150, "Clip 1"), 
          Clip(200, 300, "Clip 2"), 
          Clip(400, 800, "Clip 3")
      };

      unsigned int video_duration_ms = 1000;
      renderBookmarks(bar_position, bookmarks);
      renderClipBoxes(bar_position, clips, video_duration_ms);
    }

    void renderBookmarks(ImVec2 bar_position, std::vector<float> bookmarks) {
      for (float bookmark : bookmarks) {
          ImVec2 bar_size = ImGui::GetItemRectSize();
          float x_position = bar_position.x + bookmark * bar_size.x;
          ImVec2 start = ImVec2(x_position, bar_position.y);
          ImVec2 end = ImVec2(x_position, bar_position.y + bar_size.y);
          ImGui::GetWindowDrawList()->AddLine(start, end, IM_COL32(255, 0, 0, 255), 2.0f);
      }
    }

    void renderClipBoxes(ImVec2 bar_position, std::vector<Clip> clips, unsigned int video_duration_ms) {
      ImVec2 bar_size = ImGui::GetItemRectSize();
      for (Clip clip : clips) {
        float x_position_start = bar_position.x + ((float)clip.time_start / video_duration_ms) * bar_size.x;
        float x_position_end = bar_position.x + ((float)clip.time_end / video_duration_ms) * bar_size.x;

        ImVec2 top_left = ImVec2(x_position_start, bar_position.y);
        ImVec2 bottom_right = ImVec2(x_position_end, bar_position.y + bar_size.y);

        //ImGui::GetWindowDrawList()->AddRect(top_left, bottom_right, IM_COL32(0, 0, 255, 255), 0.0f, 0, 2.0f);
        ImGui::GetWindowDrawList()->AddRectFilled(top_left, bottom_right, IM_COL32(0, 255, 0, 50));

        // Randomize hue
        auto handle_color = IM_COL32(0, 255, 0, 255);
        float handle_h_padding = 5.0f;
        float handle_v_padding = 1.0f;
        
        // Draw handlesc
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x_position_start, bar_position.y-handle_v_padding), ImVec2(x_position_start+handle_h_padding, bottom_right.y+handle_v_padding), handle_color);
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x_position_end, bar_position.y-handle_v_padding), ImVec2(bottom_right.x-handle_h_padding, bottom_right.y+handle_v_padding), handle_color);
      }
    }

    void renderMediaButtons(int window_width) {
      static bool paused = false;
      const char* label = paused ? "Play" : "Pause";

      ImGuiStyle& style = ImGui::GetStyle();

      float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
      float avail = ImGui::GetContentRegionAvail().x;
      float off = (avail - size) * 0.5f;

      if (off > 0.0f)
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

      if(ImGui::Button(label)) {
        paused = !paused;
      }
    }

    // Callback to handle window resizing
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void getScreenDimensions(float* screenWidth, float* screenHeight, int availableHeight, int availableWidth, float aspect_ratio) {
      if ((float)availableWidth/availableHeight > aspect_ratio) {
        *screenHeight = availableHeight;
        *screenWidth = availableHeight * aspect_ratio;
      } else {
        *screenWidth = availableWidth;
        *screenHeight = availableWidth / aspect_ratio;
      }
    }


    void updateScreenVertices(GLfloat* vertices, int width, int height, int sideBarWidth, int toolBarHeight, int screenWidth, int screenHeight, int availableWidth, int availableHeight) {
      float centerX = sideBarWidth + (availableWidth - screenWidth) / 2.0f;
      float centerY = toolBarHeight + (availableHeight - screenHeight) / 2.0f;

      // Screen Triangle 1
      vertices[0] = normalizeToCoordinateSpace(centerX, width);
      vertices[1] = -normalizeToCoordinateSpace(centerY + screenHeight, height);
      vertices[5] = normalizeToCoordinateSpace(centerX + screenWidth, width);
      vertices[6] = -normalizeToCoordinateSpace(centerY + screenHeight, height);
      vertices[10] = normalizeToCoordinateSpace(centerX, width);
      vertices[11] = -normalizeToCoordinateSpace(centerY, height);

      // Screen Triangle 2
      vertices[15] =  normalizeToCoordinateSpace(centerX + screenWidth, width);
      vertices[16] = -normalizeToCoordinateSpace(centerY + screenHeight, height);
      vertices[20] = normalizeToCoordinateSpace(centerX + screenWidth, width);
      vertices[21] = -normalizeToCoordinateSpace(centerY, height);
      vertices[25] = normalizeToCoordinateSpace(centerX, width);
      vertices[26] = -normalizeToCoordinateSpace(centerY, height);
    }
};


int main() {
  Rewind rw;
  rw.run();
}
