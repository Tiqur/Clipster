#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>


const int DEBUG = false;

// This will be dynamic to video dimensions
// Will also create black bars if window size doesn't conform
const float ASPECT_RATIO = 0.5625;

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

    // Create a window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Rewind", nullptr, nullptr);
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
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

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
        int toolbarHeight = ImGui::GetWindowSize()[1];
        ImGui::EndMainMenuBar();

        // Debug
        if (DEBUG) {
          ImGui::ShowMetricsWindow();
          ImGui::ShowDebugLogWindow();
          ImGui::ShowDemoWindow();
        }

        // Sidebar
        ImGui::SetNextWindowPos(ImVec2(0, toolbarHeight));
        ImGui::SetNextWindowSize(ImVec2(std::min(std::max((int)(width*0.2), 200), 400), height-toolbarHeight));
        ImGui::Begin("ImGUI Window", &p_open, window_flags);
        ImGui::Text("Clips");
        ImGui::SeparatorText("Clips");
        if (ImGui::Button("Export All"))
          std::cout << "[INFO]: Export All" << std::endl;
        ImGui::SeparatorText("More");
        ImGui::End();



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
    return 0;
}

