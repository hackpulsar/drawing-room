#ifndef CLIENTAPPLICATION_H
#define CLIENTAPPLICATION_H

#include "gui/ImGuiLayer.h"
#include "networking/TCPClient.h"

#include <string>

namespace Core::Rendering {
    struct Color {
        float r, g ,b, a;

        void LoadFromArray(const float color[4]) {
            r = color[0];
            g = color[1];
            b = color[2];
            a = color[3];
        }
    };

    struct Line {
        ImVector<ImVec2> points{};
        Color color;
        float thickness;
    };
}

namespace Client {
    class ClientApplication {
    public:
        ClientApplication();
        ~ClientApplication();

        bool Init();
        void Run();
        void Render();

        Core::GUI::ImGuiLayer &GetGUI() const;

    private:
        void RenderChat();
        void RenderCanvas();
        void RenderTools();

        Core::GUI::ImGuiLayer *guiLayer;

        std::string address = "localhost", port = "1499", username = "user";
        std::vector<std::string> chat;
        std::string message;

        Core::Networking::TCPClient client;
        std::thread receiveThread;

        ImVector<Core::Rendering::Line> lines;
        float color[4]{0.5f, 0.5f, 0.2f, 1.0f};
        float thickness = 2.f;

        std::atomic<bool> connecting = false;
    };

}

#endif //CLIENTAPPLICATION_H
