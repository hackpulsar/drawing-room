#ifndef CLIENTAPPLICATION_H
#define CLIENTAPPLICATION_H

#include "gui/ImGuiLayer.h"
#include "networking/TCPClient.h"

#include <string>

class ClientApplication {
public:
    ClientApplication();
    ~ClientApplication();

    bool Init();
    void Run();
    void Render();

    Core::GUI::ImGuiLayer& GetGUI() const;

private:
    Core::GUI::ImGuiLayer* guiLayer;

    std::string address = "localhost", port = "1499", username = "user";
    std::vector<std::string> chat;
    std::string message;

    Core::Networking::TCPClient client;
    std::thread receiveThread;

    // TODO: lines stuff here
    ImVector<ImVector<ImVec2>> lines;
    float color[4] { 0.2f, 0.2f, 0.2f, 1.0f };
    float thickness = 2.f;

    std::atomic<bool> connecting = false;

};

#endif //CLIENTAPPLICATION_H
