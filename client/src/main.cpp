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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    io.FontGlobalScale = 1.0f;

    // TODO: search for a font in the system, not project
    io.FontDefault = io.Fonts->AddFontFromFileTTF("../../imgui/misc/fonts/Roboto-Medium.ttf", 18.0f);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

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

    Core::Networking::Package pkg = {
        Core::Networking::Package::Header {},
        Core::Networking::Package::Body { "hackpulsar" }
    };
    LOG_LINE(Core::Networking::Package::CompressToJSON(pkg).dump());

    std::string adress = "localhost", port = "1488", username = "test";
    std::vector<std::string> chat;
    std::string message;

    ImVector<ImVector<ImVec2>> lines;
    float color[4] { 0.2f, 0.2f, 0.2f, 1.0f };
    float thickness = 2.f;

    Core::Networking::TCPClient client;
    client.pkgRecCallback = [&chat, &lines] (const Core::Networking::Package& pkg) {
        using namespace Core::Networking;

        switch (pkg.getHeader().type) {
            case Package::Type::TextMessage:
                chat.push_back(pkg.getBody().data["message"]);
                break;
            case Package::Type::BoardUpdate:
                std::stringstream ss;
                ss << pkg.getBody().data;

                lines.push_back(ImVector<ImVec2>{});
                int linesCount;
                ss >> linesCount;
                for (int i = 0; i < linesCount; i++) {
                    ImVec2 point;
                    ss >> point.x >> point.y;
                    lines.back().push_back(point);
                }
                break;
        }
    };
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
            // Dockspace
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None; // Config flags for the Dockspace
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

            // If so, get the main viewport:
            const ImGuiViewport* viewport = ImGui::GetMainViewport();

            // Set the parent window's position, size, and viewport to match that of the main viewport. This is so the parent window
            // completely covers the main viewport, giving it a "full-screen" feel.
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);

            // Set the parent window's styles to match that of the main viewport:
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); // No corner rounding on the window
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // No border around the window

            // Manipulate the window flags to make it inaccessible to the user (no titlebar, resize/move, or navigation)
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
            // and handle the pass-thru hole, so the parent window should not have its own background:
            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::Begin("DockSpace", nullptr, window_flags);

            // Pop the two style rules set in Fullscreen mode - the corner rounding and the border size.
            ImGui::PopStyleVar(2);

            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }

            ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoScrollbar);

            ImGui::InputText(" ", &message);
            ImGui::SameLine();
            if (ImGui::Button("Send", { ImGui::GetContentRegionAvail().x, 0.f }) && !message.empty()) {
                using namespace Core::Networking;

                nlohmann::json data;
                data["message"] = message;
                client.AsyncSendPackage(Package {
                    Package::Header { message.size(), Package::Type::TextMessage, client.GetID() },
                    Package::Body { data }
                });

                message = "";
            }

            ImGui::BeginChild("Scrolling");
            for (const auto& m : chat) {
                ImGui::Text("%s", m.c_str());
            }
            ImGui::EndChild();

            ImGui::End(); // Chat

            ImGui::Begin("Board", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::BeginChild("Canvas", ImGui::GetContentRegionAvail());

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

                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    isDrawing = false;

                    // Construct package body
                    std::stringstream lineStream;
                    lineStream << lines.back().size() << "\n"; // Number of points
                    for (const auto& p : lines.back()) {
                        lineStream << p.x << "\n" << p.y << "\n";
                    }

                    using namespace Core::Networking;

                    // Send new line to the server
                    /*client.AsyncSendPackage(ActualPackage {
                        Header { lineStream.str().size(), Type::BoardUpdate, client.GetID() },
                        Body { lineStream.str() }
                    });*/
                }
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
                        IM_COL32(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255),
                        thickness
                    );
                }
            }
            draw_list->PopClipRect();

            ImGui::EndChild();
            ImGui::End();

            // Tools window
            ImGui::Begin("Tools");
            ImGui::ColorEdit4("Colour", color);
            ImGui::Spacing();
            ImGui::SliderFloat("Thickness", &thickness, 0.f, 10.f);
            ImGui::End();

            ImGui::End(); // Dockspace
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

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
