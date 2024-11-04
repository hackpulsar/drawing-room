#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

#include "networking/TCPClient.h"

#include <cstdio>
#include <string>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <utils/log.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Drawing room by @hackpulsar", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.FontGlobalScale = 1.25f;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);

    // Main loop
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else

    std::string adress = "localhost", port = "1488", username = "test";
    std::vector<std::string> chat;
    std::string message;

    Core::Networking::TCPClient client;
    client.msgRecCallback = [&chat] (const std::string& message) { chat.push_back(message); };
    std::thread clientThread;
    std::atomic connecting(false);

    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!client.IsConnected() || connecting) {
            ImGui::Begin("Lobby", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

            ImGui::InputText("Address", &adress);
            ImGui::InputText("Port", &port);
            ImGui::InputText("Username", &username);

            if (!connecting) {
                if (ImGui::Button("Connect")) {
                    client.SetUsername(username);
                    connecting = true;
                    auto ec = client.ConnectTo(adress, port);

                    if (!ec) {
                        clientThread = std::thread([&client, &connecting] {
                            if (client.Handshake()) {
                                connecting = false;
                                client.StartReading();
                            }
                        });
                    }
                    else
                        LOG_LINE("Error connecting: " << ec.what());
                }
            }
            else
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(0.0f, 0.0f), "Connecting..");

            ImGui::End();
        }
        else {
            ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::BeginChild("Scrolling", ImVec2(0, 400));
            for (const auto& m : chat) {
                ImGui::Text("%s", m.c_str());
            }
            ImGui::EndChild();

            ImGui::InputText(" ", &message);
            ImGui::SameLine();
            if (ImGui::Button("Send")) {
                using namespace Core::Networking;
                using namespace Core::Networking::Package;

                client.SendPackage(Core::Networking::ActualPackage {
                    Header { message.size(), Type::TextMessage },
                    Body { message + "\n" }
                });
                //client.AsyncSendString(message + "\n");
                message = "";
            }

            ImGui::End();

            ImGui::Begin("Board", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::BeginChild("Canvas", ImVec2(800, 600));

            static ImVector<ImVector<ImVec2>> lines;
            static ImVec2 scrolling(0.0f, 0.0f);
            static bool enableGrid = true;
            static bool isDrawing = false;

            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw borders and background
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // Interactions
            ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool isHovered = ImGui::IsItemHovered(); // Hovered
            const bool isActive = ImGui::IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            static ImVec2 lastPoint;

            if (isHovered && isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && !isDrawing) {
                // Create new line
                lines.push_back(ImVector<ImVec2>{});
                lines.back().push_back(mouse_pos_in_canvas);
                lines.back().push_back(mouse_pos_in_canvas);

                isDrawing = true;
                lastPoint = lines.back().back();
            }

            if (isDrawing) {
                lines.back().back() = mouse_pos_in_canvas;

                if (sqrtf(powf(lastPoint.x - mouse_pos_in_canvas.x, 2) + powf(lastPoint.y - mouse_pos_in_canvas.y, 2)) > 8.0f) {
                    lines.back().push_back(mouse_pos_in_canvas);
                    lastPoint = lines.back().back();
                }

                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    isDrawing = false;
            }

            if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }

            // Draw grid
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (enableGrid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
            }

            // Draw lines
            for (const auto& line : lines) {
                for (int i = 0; i < line.size() - 1; i++) {
                    draw_list->AddLine(
                        ImVec2(origin.x + line[i].x, origin.y + line[i].y),
                        ImVec2(origin.x + line[i + 1].x, origin.y + line[i + 1].y),
                        IM_COL32(255, 255, 0, 255),
                        2.0f
                    );
                }
            }
            draw_list->PopClipRect();

            ImGui::EndChild();
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        );
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    client.Stop();
    if (clientThread.joinable())
        clientThread.join();

    return 0;
}
