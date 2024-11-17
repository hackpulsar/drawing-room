#ifndef IMGUILAYER_H
#define IMGUILAYER_H

#include "imgui.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <functional>

namespace Core::GUI {
    typedef std::function<void()> ClientSideWork;

    class ImGuiLayer {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        bool Init();
        void Run();

        ImGuiIO& GetIO() const;

        void SetClientSideWork(ClientSideWork&& work);

    private:
        GLFWwindow* window;
        ImVec4 clearColor;

        ClientSideWork clientSideWork;

    };
}

#endif //IMGUILAYER_H
