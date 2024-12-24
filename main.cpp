#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include"video_manager.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>


const int DEBUG = false;

// Vertex Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

//Fragment Shader source code
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";



float normalizeToCoordinateSpace(float a, float b) {
  return (2.0*a)/b-1;
}

// Callback to handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Rewind", NULL, NULL);
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

    GLint success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex shader compilation failed:\n" << infoLog << std::endl;
    }


    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment shader compilation failed:\n" << infoLog << std::endl;
    }


    GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader program linking failed:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);




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
        int width, height, sideBarWidth;
        glfwGetWindowSize(window, &width, &height);
        sideBarWidth = std::min(std::max((int)(width*0.2), 200), 400);

        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
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
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

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
        int toolBarHeight = ImGui::GetWindowSize()[1];
        ImGui::EndMainMenuBar();

        // Debug
        if (DEBUG) {
          ImGui::ShowMetricsWindow();
          ImGui::ShowDebugLogWindow();
          ImGui::ShowDemoWindow();
        }

        // Sidebar
        ImGui::SetNextWindowPos(ImVec2(0, toolBarHeight));
        ImGui::SetNextWindowSize(ImVec2(sideBarWidth, height-toolBarHeight));
        ImGui::Begin("ImGUI Window", &p_open, window_flags);
        ImGui::Text("Clips");
        ImGui::SeparatorText("Clips");
        if (ImGui::Button("Export All"))
          std::cout << "[INFO]: Export All" << std::endl;
        ImGui::SeparatorText("More");
        ImGui::End();


        // Edit vertices
        int scrubBarHeight = (int)std::min(std::max(height*0.25, 100.0), 200.0);

        int availableWidth = width-sideBarWidth;
        int availableHeight = height-scrubBarHeight;

        // Maintain aspect ratio
        float screenWidth, screenHeight;
        if ((float)availableWidth/availableHeight > aspect_ratio) {
          screenHeight = availableHeight;
          screenWidth = availableHeight * aspect_ratio;
        } else {
          screenWidth = availableWidth;
          screenHeight = availableWidth / aspect_ratio;
        }

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

        // Scrub Bar Triangle 1
        //vertices[18] = normalizeToCoordinateSpace(centerX + screenWidth, width);
        //vertices[19] = normalizeToCoordinateSpace(scrubBarHeight, height);
        //vertices[21] = normalizeToCoordinateSpace(centerX + screenWidth, width);
        //vertices[22] = -1.0f;
        //vertices[24] = normalizeToCoordinateSpace(centerX, width);
        //vertices[25] = -1.0f;

        // Scrub Bar Triangle 1
        //vertices[27] = normalizeToCoordinateSpace(centerX + screenWidth, width);
        //vertices[28] = -1.0f;
        //vertices[30] = normalizeToCoordinateSpace(centerX + screenWidth, width);
        //vertices[31] = -1.0f;
        //vertices[33] = normalizeToCoordinateSpace(centerX, width);
        //vertices[34] = normalizeToCoordinateSpace(scrubBarHeight, height);

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

