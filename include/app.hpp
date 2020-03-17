#ifndef __APP_HPP__
#define __APP_HPP__
#include <list>
#include <memory>
#include <functional>
#include <cstring>
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
    extern char shared_buffer[4096];
#if FEATURE_DirectX
    extern WNDCLASSEX wc;
    extern HWND hwnd;
#else
    extern GLFWwindow* window;
#endif

    struct AppInitResult {
        int ok;
        const char *version_string;

        inline static AppInitResult Ok(const char *version_string = nullptr) {
            return {1, version_string};
        }

        inline static AppInitResult Err() {
            return {0};
        }
    };

    struct App {
    public:
        App(App &&) = default;

        App &operator=(App &&) = default;

        App(const App &) = delete;

        App &operator=(const App &) = delete;

        ~App() { App::Shutdown(); }

        static AppInitResult Initialize();

        static bool EventLoop();

        static void Render(const ImVec4 &clear_color);

#if FEATURE_DirectX
        static ID3D11Device* GetDXDevice();
		static ID3D11DeviceContext* GetDXDeviceContext();
#endif

    private:
        static void Shutdown();
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

    class IWindow {
    public:
        virtual IWindow* Show() = 0;
        virtual bool Draw() = 0;
        virtual ~IWindow() = default;

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

    class CreatorIWindow {
    protected:
        std::function<void(std::unique_ptr<IWindow>&&)> CreateIWindow = nullptr;
    public:
        void GrantCreateWindow(std::function<void(std::unique_ptr<IWindow>&&)>);
    };

//    class IStaticWindow: public IWindow {
//    public:
//        virtual IWindow* Show() override {
//            return dynamic_cast<IWindow*>(this);
//        }
//        virtual bool Draw() override = 0;
//        virtual ~IStaticWindow() {}
//    };

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
}


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

namespace spt::Math {
//    ImVec2 FitWidth(const ImVec2 &src, float fit_width);
//    ImVec2 FitHeight(const ImVec2 &src, float fit_height);
//    ImVec2 FitBox(const ImVec2 &src, const ImVec2 &fit_limit);

    template<typename V>
    V FitWidth(const V &src, float fit_width) {
        if (fit_width <= 0)
            return V(0, 0);
        if (src.x == 0)
            return V(0, 0);
        float scale_factor = fit_width / src.x;
        return V(fit_width, src.y * scale_factor);
    }

    template<typename V>
    V FitHeight(const V &src, float fit_height) {
        if (fit_height <= 0)
            return V(0, 0);
        if (src.y == 0)
            return V(0, 0);
        float scale_factor = fit_height / src.y;
        return V(src.x * scale_factor, fit_height);
    }

    template<typename V>
    V FitBox(const V &src, const V &fit_limit) {
        if (fit_limit.x <= 0 || fit_limit.y <= 0)
            return V(0, 0);
        if (src.x == 0 || src.y == 0)
            return V(0, 0);
        float aspect_input = src.x / src.y, aspect_output = fit_limit.x / fit_limit.y;
        float scale_factor = aspect_input > aspect_output ? fit_limit.x / src.x : fit_limit.y / src.y;
        return V(src.x * scale_factor, src.y * scale_factor);
    }

    template<typename S, typename T>
    S FitWidth(const S &src, T fit_width) {
        if (fit_width <= 0)
            return S(0, 0);
        float scale_factor = ((float)fit_width) / src.width;
        return S(fit_width, static_cast<T>(src.height * scale_factor));
    }

    template<typename S, typename T>
    S FitHeight(const S &src, T fit_height) {
        if (fit_height <= 0)
            return S(0, 0);
        float scale_factor = ((float)fit_height) / src.height;
        return S(static_cast<T>(src.width * scale_factor), fit_height);
    }

    template<typename S>
    inline float AspectRatio(const S &src) {
        return static_cast<float>(src.width)/src.height;
    }

    const char *AspectRatioSS(float ratio);

    template<typename S>
    const char *AspectRatioSS(const S &src) {
        float ratio = AspectRatio(src);
        return AspectRatioSS(ratio);
    }
}

#endif
