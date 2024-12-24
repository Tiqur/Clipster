#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>


const int DEBUG = false;

// This will be dynamic to video dimensions
// Will also create black bars if window size doesn't conform
const float ASPECT_RATIO = 16.0f/9.0f;

// Vertex Shader source code
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
//Fragment Shader source code
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.0f, 0.0f, 0.00f, 1.0f);\n"
"}\n\0";



float normalizeToCoordinateSpace(float a, float b) {
  return (2.0*a)/b-1;
}

// Callback to handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
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
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,

      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,

      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,

      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices)/3);

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
        if ((float)availableWidth/availableHeight > ASPECT_RATIO) {
          screenHeight = availableHeight;
          screenWidth = availableHeight * ASPECT_RATIO;
        } else {
          screenWidth = availableWidth;
          screenHeight = availableWidth / ASPECT_RATIO;
        }

        float centerX = sideBarWidth + (availableWidth - screenWidth) / 2.0f;
        float centerY = toolBarHeight + (availableHeight - screenHeight) / 2.0f;

        // Screen Triangle 1
        vertices[0] = normalizeToCoordinateSpace(centerX, width);
        vertices[1] = -normalizeToCoordinateSpace(centerY + screenHeight, height);
        vertices[3] = normalizeToCoordinateSpace(centerX + screenWidth, width);
        vertices[4] = -normalizeToCoordinateSpace(centerY + screenHeight, height);
        vertices[6] = normalizeToCoordinateSpace(centerX, width);
        vertices[7] = -normalizeToCoordinateSpace(centerY, height);

        // Screen Triangle 2
        vertices[9] =  normalizeToCoordinateSpace(centerX + screenWidth, width);
        vertices[10] = -normalizeToCoordinateSpace(centerY + screenHeight, height);
        vertices[12] = normalizeToCoordinateSpace(centerX + screenWidth, width);
        vertices[13] = -normalizeToCoordinateSpace(centerY, height);
        vertices[15] = normalizeToCoordinateSpace(centerX, width);
        vertices[16] = -normalizeToCoordinateSpace(centerY, height);

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

