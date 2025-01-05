#include"shader_utils.h"
#include"desktop_capture.hpp"
#include"media_player.hpp"
#include"UIManager.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstring>
#include <thread>


namespace Config {
  constexpr char WINDOW_TITLE[] = "Rewind";
  constexpr int INITIAL_WIDTH = 800;
  constexpr int INITIAL_HEIGHT = 600;
  const bool DEBUG = false;
}

class Rewind {
  public:
    const char* filename = "output.mp4";
    int frame_width = 0;
    int frame_height = 0;
    unsigned char* frame_data = nullptr;
    float screen_aspect_ratio = 0.0f;


    Rewind() {

    }

    int run() {
      MediaPlayer mp;
      mp.loadFile(this->filename);
      VideoFrame firstFrame = mp.videoBuffer[0];
      std::cout << firstFrame.width << std::endl;
      this->frame_width = firstFrame.width;
      this->frame_height = firstFrame.height;

      if (this->frame_width == 0 || this->frame_height == 0) {
        std::cout << "Invalid frame dimensions" << std::endl;
        return -1;
      }

      screen_aspect_ratio = (float)this->frame_width / this->frame_height;

      if (firstFrame.data == nullptr) {
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
      GLFWwindow* window = glfwCreateWindow(Config::INITIAL_WIDTH, Config::INITIAL_HEIGHT, Config::WINDOW_TITLE, NULL, NULL);
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



      // Generate textures
      GLuint textureY, textureU, textureV;
      glGenTextures(1, &textureY);
      glGenTextures(1, &textureU);
      glGenTextures(1, &textureV);

      // Initialize Y texture
      glBindTexture(GL_TEXTURE_2D, textureY);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame_width, frame_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

      // Initialize U texture
      glBindTexture(GL_TEXTURE_2D, textureU);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame_width/2, frame_height/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

      // Initialize V texture
      glBindTexture(GL_TEXTURE_2D, textureV);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame_width/2, frame_height/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

      // Register the framebuffer size callback
      glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

      int audioBufferIndex = 0;
      int videoBufferIndex = 0;
      int width, height, sideBarWidth, bottomBarHeight;

      UIManager ui;
      ui.init(&width, &height, window, "#version 330", &this->screen_aspect_ratio);

      double playbackStartTime = glfwGetTime();
      double lastFrameTime = 0;

      // Main render loop
      while (!glfwWindowShouldClose(window)) {
          glfwGetWindowSize(window, &width, &height);
          sideBarWidth = std::min(std::max((int)(width*0.2), 200), 400);

          // Render
          glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
          glClear(GL_COLOR_BUFFER_BIT);
          glUseProgram(shaderProgram);

          // Get current and elapsed time
          double currentTime = glfwGetTime();
          double playbackTime = currentTime - playbackStartTime;

          // Calculate timing based on video and audio
          double videoPts = mp.videoBuffer[0].pts;
          double audioPts = mp.audioBuffer[0].pts;

          // Audio and video synchronization tolerance
          const double syncTolerance = 0.05; // 50ms
          bool audioVideoInSync = std::abs(videoPts - audioPts) < syncTolerance;

          // Seek if audio / video not in sync
          if (!audioVideoInSync && !mp.videoBuffer.empty() && !mp.audioBuffer.empty()) {
            if (videoPts > audioPts) {
              double seekTime = audioPts - syncTolerance;
              mp.seek(seekTime, true);
            }
            else {
              double seekTime = videoPts + syncTolerance;
              mp.seek(seekTime, false);
            }

            audioBufferIndex = 0;
            videoBufferIndex = 0;
          } else if (audioVideoInSync && !mp.videoBuffer.empty() && !mp.audioBuffer.empty()) {

            // Render video
            VideoFrame frame = mp.videoBuffer[videoBufferIndex];

            // Determine if should render frame using elapsed time
            bool shouldRenderFrame = frame.pts <= playbackTime;

            if (shouldRenderFrame) {
              // Generate texture from frame
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_2D, textureY);
              glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height, GL_RED, GL_UNSIGNED_BYTE, frame.data[0]);
              glUniform1i(glGetUniformLocation(shaderProgram, "textureY"), 0);

              // Bind and update U plane texture
              glActiveTexture(GL_TEXTURE1);
              glBindTexture(GL_TEXTURE_2D, textureU);
              glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width / 2, frame.height / 2, GL_RED, GL_UNSIGNED_BYTE, frame.data[1]);
              glUniform1i(glGetUniformLocation(shaderProgram, "textureU"), 1);

              // Bind and update V plane texture
              glActiveTexture(GL_TEXTURE2);
              glBindTexture(GL_TEXTURE_2D, textureV);
              glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width / 2, frame.height / 2, GL_RED, GL_UNSIGNED_BYTE, frame.data[2]);
              glUniform1i(glGetUniformLocation(shaderProgram, "textureV"), 2);

              // Play audio
              AudioFrame audioFrame = mp.audioBuffer[audioBufferIndex];
              //playAudio(audioFrame);

              audioBufferIndex++;
              videoBufferIndex++;
              lastFrameTime = currentTime;
            }
          } else {
            std::cout << "Audio and video are out of sync but no seek triggered." << std::endl;
          }

          glBindVertexArray(VAO);
          glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices)/sizeof(vertices[0])/5);

          ui.render();

          // Process input
          handleInput(window);

          // Render Toolbar
          int toolBarHeight;

          // Edit vertices
          bottomBarHeight = (int)std::min(std::max(height*0.25, 100.0), 200.0);

          int availableWidth = width-sideBarWidth;
          int availableHeight = height-bottomBarHeight;

          // Maintain aspect ratio
          float normalizedScreenWidth, normalizedScreenHeight;
          getScreenDimensions(&normalizedScreenWidth, &normalizedScreenHeight, availableHeight, availableWidth);

          // Update triangle vertices
          updateScreenVertices(vertices, width, height, sideBarWidth, toolBarHeight, normalizedScreenWidth, normalizedScreenHeight, availableWidth, availableHeight);

          glBindBuffer(GL_ARRAY_BUFFER, VBO);
          glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

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


    // Callback to handle window resizing
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void getScreenDimensions(float* screenWidth, float* screenHeight, int availableHeight, int availableWidth) {
      if ((float)availableWidth/availableHeight > this->screen_aspect_ratio) {
        *screenHeight = availableHeight;
        *screenWidth = availableHeight * this->screen_aspect_ratio;
      } else {
        *screenWidth = availableWidth;
        *screenHeight = availableWidth / this->screen_aspect_ratio;
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
