#ifndef __APP_HPP__
#define __APP_HPP__
#include <list>
#include <memory>
#include "imgui.h"

#if FEATURE_DirectX
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#endif

#if FEATURE_OpenGL
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

namespace spt::AppEngine {
#if FEATURE_DirectX
    extern WNDCLASSEX wc;
    extern HWND hwnd;
#else
    extern GLFWwindow* window;
#endif

    class App final {
    public:
        int ok;
        const char *version_string;

        App(App &&) = default;

        App &operator=(App &&) = default;

        App(const App &) = delete;

        App &operator=(const App &) = delete;

        ~App() { this->Shutdown(); }

        inline static App Ok(const char *version_string = nullptr) {
            return {1};
        }

        inline static App Err() {
            return {0};
        }

        static App Initialize();

        bool EventLoop();

        void Render(const ImVec4 &clear_color);

#if FEATURE_DirectX
        static ID3D11Device* GetDXDevice();
		static ID3D11DeviceContext* GetDXDeviceContext();
#endif

    private:
        void Shutdown();
    };

#if _MSC_VER // Must use Windows API to hide console window
#define PIPE_USE_POPEN 0
#else
#define PIPE_USE_POPEN 1
#endif
    class Pipe {
    private:
#if PIPE_USE_POPEN
        FILE *fp;
#else
        HANDLE hChildStd_IN_Rd;
        HANDLE hChildStd_IN_Wr;
        HANDLE hChildStd_OUT_Rd;
        HANDLE hChildStd_OUT_Wr;
        HANDLE hChildStd_ERR_Rd;
        HANDLE hChildStd_ERR_Wr;
        PROCESS_INFORMATION piProcInfo;
#endif

    public:
        Pipe();

        ~Pipe() {
            Close();
        }

        int Open(const char *cmdline, bool write = false);

        int Close();

        size_t Read(char *buffer, size_t size);

        size_t Write(char *buffer, size_t size);
    };

    struct LineBuffer {
        char *buffer = nullptr;
        const size_t buffer_size; // total buffer size
        const char *cursor_src = nullptr; // valid read location
        char *cursor_buf = nullptr; // valid write location
        size_t capacity; // available buffer size
        bool discard_line = false;
		char bypass = 32; // consecutive whole line inputs cause bypass
		//   bypass: -1 disable  0 enable  >0 observing
        LineBuffer(size_t buffer_size = 512);
        ~LineBuffer();
        bool GetLine(const char *src, size_t max_count);
        bool Copy(const char *src, size_t len, bool complete);
		void DecrBypass(const char *src);
    };

    class FlatlandObject {
    protected:
        char type, separator;
        std::list<std::string> values;
        void PushValues(const char *src, size_t buffer_size);
    private:
        FlatlandObject();
    public:
        FlatlandObject(char type, char sep);
		FlatlandObject(char type, char sep, const char *src, size_t buffer_size);
        virtual ~FlatlandObject() = default;
        static FlatlandObject Parse(const char *src, size_t buffer_size);
        static size_t ValidBufferSize(const char *src, size_t buffer_size);
    };
}

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
        char *buf = new char[len+1];
        buf[len] = '\0';
        for (int i = 0; i < len; ++i)
            buf[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        delete[] buf;
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
//template <typename Tdst>
//class IBinding {
//    public:
//    virtual Tdst Export() const = 0;
//    virtual void Import(const Tdst &dst) = 0;
//
//    private:
//    IBinding() {} // Implicit interface, do NOT inherit
//};

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

#endif
