#include "ClientApplication.h"

#include "misc/cpp/imgui_stdlib.h"
#include "utils/log.h"

ClientApplication::ClientApplication() : guiLayer(new Core::GUI::ImGuiLayer) {

}

ClientApplication::~ClientApplication() {
    client.Stop();
    if (receiveThread.joinable())
        receiveThread.join();

    delete guiLayer;
}

bool ClientApplication::Init() {
    if (!guiLayer->Init())
        return false;

    guiLayer->SetClientSideWork([this]() { this->Render(); });

    client.pkgRecCallback = [this] (const Core::Networking::Package& pkg) {
        using namespace Core::Networking;

        switch (pkg.getHeader().type) {
            case Package::Type::TextMessage:
                this->chat.push_back(pkg.getBody().data["message"]);
            break;
            case Package::Type::BoardUpdate:
                this->lines.push_back(ImVector<ImVec2>{});
            int linesCount = pkg.getBody().data.at("numberOfPoints");
            for (int i = 0; i < linesCount; i++) {
                ImVec2 point;
                point.x = pkg.getBody().data.at("points").at(i).at(0);
                point.y = pkg.getBody().data.at("points").at(i).at(1);
                this->lines.back().push_back(point);
            }
            break;
        }
    };

    return true;
}

void ClientApplication::Run() {
    this->guiLayer->Run();
}

void ClientApplication::Render() {
    if (!client.IsConnected() || connecting) {
        ImGui::Begin("Lobby", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

        ImGui::InputText("Address", &address);
        ImGui::InputText("Port", &port);
        ImGui::InputText("Username", &username);

        if (!connecting) {
            if (ImGui::Button("Connect")) {
                client.SetUsername(username);
                connecting = true;
                auto ec = client.ConnectTo(address, port);

                if (!ec) {
                    receiveThread = std::thread([this] {
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

            if (guiLayer->GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
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
            const ImVec2 mouse_pos_in_canvas(guiLayer->GetIO().MousePos.x - origin.x, guiLayer->GetIO().MousePos.y - origin.y);

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
                    nlohmann::json data;
                    data["options"]["color"] = color;
                    data["options"]["thickness"] = thickness;
                    data["numberOfPoints"] = lines.back().size();
                    for (const auto& p : lines.back())
                        data["points"].push_back({ p.x, p.y });

                    using namespace Core::Networking;

                    // Send new line to the server
                    client.AsyncSendPackage(Package {
                        Package::Header { data.dump().size(), Package::Type::BoardUpdate, client.GetID() },
                        Package::Body { data }
                    });
                }
            }

            if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
            {
                scrolling.x += guiLayer->GetIO().MouseDelta.x;
                scrolling.y += guiLayer->GetIO().MouseDelta.y;
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
}

Core::GUI::ImGuiLayer& ClientApplication::GetGUI() const { return *this->guiLayer; }

