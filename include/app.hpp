#ifndef __APP_HPP__
#define __APP_HPP__
#include <list>
#include <memory>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/// ImGui App boiler plates
class App final {
    public:
    int ok;
    GLFWwindow* window;
    const char* glsl_version;

    App(App&&) = default;
    App& operator=(App&&) = default;
    App (const App &) = delete;
    App& operator=(const App &) = delete;
    ~App() { this->Shutdown(); }

    inline static App Ok(GLFWwindow* w, const char* glsl_version) {
        return {.ok=1, .window=w, .glsl_version=glsl_version};
    }

    inline static App Err() {
        return {.ok=0, .window=nullptr, .glsl_version=nullptr};
    }

    static App Initialize();
    bool EventLoop();
    void Render(const ImVec4 &clear_color);

    private:
    void Shutdown();
};

class IWindow {
    public:
    virtual IWindow* Show() = 0;
    virtual bool Draw() = 0;
    virtual ~IWindow() {}

    static std::string uuid(const int len) {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        char buf[len+1];
        buf[len] = '\0';
        for (int i = 0; i < len; ++i)
            buf[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        return std::string(buf);
    }
};

class IStaticWindow: public IWindow {
    public:
    virtual IWindow* Show() override {
        return dynamic_cast<IWindow*>(this);
    }
    virtual bool Draw() override = 0;
    virtual ~IStaticWindow() {}
};

// TODO Consider visitor pattern?
// Provides a data binding interface for ImGui
template <typename Tdst>
class IBinding {
    public:
    virtual Tdst Export() const = 0;
    virtual void Import(const Tdst &dst) = 0;

    private:
    IBinding() {} // Implicit interface, do NOT inherit
};

class TestWindow: public IWindow {
    public:
    TestWindow(bool show = false) {
        std::string id = IWindow::uuid(5);
        std::snprintf(_title, IM_ARRAYSIZE(_title), "Test Window [%s]", id.c_str());
        this->_is_shown = show;
    }

    virtual ~TestWindow() {}

    virtual bool Draw() override {
        if (!this->_is_shown) return false;
        ImGui::Begin(this->_title, &this->_is_shown);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            this->_is_shown = false;
        ImGui::End();
        return true;
    }

    virtual IWindow* Show() override {
        this->_is_shown = true;
        return dynamic_cast<IWindow*>(this);
    }

    protected:
    bool _is_shown = false;
    char _title[32];
};

// Provides a RAII data binding
template <typename Tsrc, typename Tdst>
struct Binding {
    Tsrc &source;
    Tdst binding;

    Binding(Tsrc &src): source(src) {
        this->binding = src.Export();
    }

    ~Binding() {
        this->source.Import(this->binding);
    }
};

GLuint LoadShaders(const char *fname_vert, const char *fname_frag);

#endif
