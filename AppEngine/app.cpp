#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#define NOMINMAX

#include "app.hpp"

#if FEATURE_DirectX

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

namespace spt::AppEngine {
    char shared_buffer[4096];

#if FEATURE_DirectX
    WNDCLASSEX wc;
    HWND hwnd;
#endif

#if FEATURE_OpenGL
    GLFWwindow* window;
#endif


#if FEATURE_DirectX
    static ID3D11Device *g_pd3dDevice = nullptr;
    static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
    static IDXGISwapChain *g_pSwapChain = nullptr;
    static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
    bool CreateDeviceD3D(HWND hWnd);

    void CleanupDeviceD3D();

    void CreateRenderTarget();

    void CleanupRenderTarget();

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ID3D11Device *App::GetDXDevice() {
        return g_pd3dDevice;
    }

    ID3D11DeviceContext *App::GetDXDeviceContext() {
        return g_pd3dDeviceContext;
    }

#endif

#if FEATURE_OpenGL
    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "Glfw Error " << error << description << std::endl;
    }

    GLuint LoadShaders(const char *fname_vert, const char *fname_frag);
#endif

#if FEATURE_DirectX

    bool App::EventLoop() {
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (msg.message != WM_QUIT) {
            // Poll and handle messages (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            if (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                continue;
            }

            // Start the Dear ImGui frame
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            return true;
        }
        return false;
    }

#endif

#if FEATURE_OpenGL
    /// Polls events and returns whether to continue
    bool App::EventLoop() {
        if(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            return true;
        }
        else return false;
    }
#endif

#if FEATURE_DirectX

/// ImGui App boiler plates
    void App::Render(const ImVec4 &clear_color) {
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float *) &clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

#endif

#if FEATURE_OpenGL
    /// ImGui App boiler plates
    void App::Render(const ImVec4 &clear_color) {
        ImGui::Render();
        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }
#endif

#if FEATURE_DirectX

    void App::Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    }

#endif

#if FEATURE_OpenGL
    void App::Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }
#endif

#if FEATURE_DirectX

    AppInitResult App::Initialize() {
        // Create application window
        wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr,
              nullptr,
              _T("ImGui Example"), nullptr};
        ::RegisterClassEx(&wc);
        hwnd = ::CreateWindow(wc.lpszClassName, _T("Superpixel Analyzer"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800,
                              nullptr, nullptr, wc.hInstance, nullptr);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd)) {
            CleanupDeviceD3D();
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);
            return AppInitResult::Err();
        }

        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        return AppInitResult::Ok();
    }

#endif

#if FEATURE_OpenGL
    AppInitResult App::Initialize() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) return App::Err();

        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

        // Create window with graphics context
        GLFWwindow* window = glfwCreateWindow(1280, 720, "Superpixel Analyzer", NULL, NULL);
        if (window == NULL) return App::Err();
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // ? "Enable vsync" - what is this?

        bool err = glewInit() != GLEW_OK;
        if (err) {
            std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
            return AppInitResult::Err();
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        return AppInitResult::Ok(glsl_version);
    }

#endif

#if FEATURE_DirectX
// Helper functions

    bool CreateDeviceD3D(HWND hWnd) {
        // Setup swap chain
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};
        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
                                          D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
                                          &g_pd3dDeviceContext) != S_OK)
            return false;

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D() {
        CleanupRenderTarget();
        if (g_pSwapChain) {
            g_pSwapChain->Release();
            g_pSwapChain = nullptr;
        }
        if (g_pd3dDeviceContext) {
            g_pd3dDeviceContext->Release();
            g_pd3dDeviceContext = nullptr;
        }
        if (g_pd3dDevice) {
            g_pd3dDevice->Release();
            g_pd3dDevice = nullptr;
        }
    }

    void CreateRenderTarget() {
        ID3D11Texture2D *pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    void CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
    }

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg) {
            case WM_SIZE:
                if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                    CleanupRenderTarget();
                    g_pSwapChain->ResizeBuffers(0, (UINT) LOWORD(lParam), (UINT) HIWORD(lParam), DXGI_FORMAT_UNKNOWN,
                                                0);
                    CreateRenderTarget();
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)
                    return 0;
                break;
            case WM_DESTROY:
                ::PostQuitMessage(0);
                return 0;
        }
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

#endif

#if FEATURE_OpenGL
    GLuint LoadShaders(const char *fname_vert, const char *fname_frag) {
        // Create the shaders
        GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

        // Read the Vertex Shader code from the file
        std::string VertexShaderCode;
        std::ifstream VertexShaderStream(fname_vert, std::ios::in);
        if(VertexShaderStream.is_open()){
            std::stringstream sstr;
            sstr << VertexShaderStream.rdbuf();
            VertexShaderCode = sstr.str();
            VertexShaderStream.close();
        }else{
            printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fname_vert);
            getchar();
            return 0;
        }

        // Read the Fragment Shader code from the file
        std::string FragmentShaderCode;
        std::ifstream FragmentShaderStream(fname_frag, std::ios::in);
        if(FragmentShaderStream.is_open()){
            std::stringstream sstr;
            sstr << FragmentShaderStream.rdbuf();
            FragmentShaderCode = sstr.str();
            FragmentShaderStream.close();
        }

        GLint Result = GL_FALSE;
        int InfoLogLength;

        // Compile Vertex Shader
        printf("Compiling shader : %s\n", fname_vert);
        char const * VertexSourcePointer = VertexShaderCode.c_str();
        glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
        glCompileShader(VertexShaderID);

        // Check Vertex Shader
        glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if ( InfoLogLength > 0 ){
            std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
            glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
            printf("%s\n", &VertexShaderErrorMessage[0]);
        }

        // Compile Fragment Shader
        printf("Compiling shader : %s\n", fname_frag);
        char const * FragmentSourcePointer = FragmentShaderCode.c_str();
        glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
        glCompileShader(FragmentShaderID);

        // Check Fragment Shader
        glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if ( InfoLogLength > 0 ){
            std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
            glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
            printf("%s\n", &FragmentShaderErrorMessage[0]);
        }

        // Link the program
        printf("Linking program\n");
        GLuint ProgramID = glCreateProgram();
        glAttachShader(ProgramID, VertexShaderID);
        glAttachShader(ProgramID, FragmentShaderID);
        glLinkProgram(ProgramID);

        // Check the program
        glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
        glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if ( InfoLogLength > 0 ){
            std::vector<char> ProgramErrorMessage(InfoLogLength+1);
            glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
            printf("%s\n", &ProgramErrorMessage[0]);
        }

        glDetachShader(ProgramID, VertexShaderID);
        glDetachShader(ProgramID, FragmentShaderID);

        glDeleteShader(VertexShaderID);
        glDeleteShader(FragmentShaderID);

        return ProgramID;
    }
#endif


#if _MSC_VER
#define popen _popen
#define pclose _pclose
#endif
#if PIPE_USE_POPEN
    Pipe::Pipe() {
        fp = nullptr;
    }

    int Pipe::Open(const char *cmdLine, bool write) {
        fp = popen(cmdLine, write ? "wb" : "rb");
        if (!fp)
            return -1;
        return 0;
    }

    int Pipe::Close() {
        if (fp) {
            pclose(fp);
            fp = nullptr;
        }
        return 0;
    }

    size_t Pipe::Read(char *buffer, size_t size) {
        return fread(buffer, 1, size, fp);
    }

    size_t Pipe::Write(char *buffer, size_t size) {
        return fwrite(buffer, 1, size, fp);
    }
#else

    Pipe::Pipe() {
        hChildStd_IN_Rd = nullptr;
        hChildStd_IN_Wr = nullptr;

        hChildStd_OUT_Rd = nullptr;
        hChildStd_OUT_Wr = nullptr;

        hChildStd_ERR_Rd = nullptr;
        hChildStd_ERR_Wr = nullptr;
    }

    int Pipe::Open(const char *cmdLine, bool write) {
        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited.

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = nullptr;

        // Create a pipe for the child process's STDOUT.

        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
            return -1;

        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if (!write) {
            if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
                return -1;
        } else {
            if (!SetHandleInformation(hChildStd_OUT_Wr, HANDLE_FLAG_INHERIT, 0))
                return -1;
        }

        // Create a pipe for the child process's STDERR.

        if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0))
            return -1;

        // Ensure the read handle to the pipe for STDERR is not inherited.

        if (!write) {
            if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
                return -1;
        } else {
            if (!SetHandleInformation(hChildStd_ERR_Wr, HANDLE_FLAG_INHERIT, 0))
                return -1;
        }

        // Create a pipe for the child process's STDIN.

        if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0))
            return -1;

        // Ensure the write handle to the pipe for STDIN is not inherited.

        if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
            return -1;


        STARTUPINFOA siStartInfo;
        BOOL bSuccess = FALSE;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = hChildStd_ERR_Wr;
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.hStdInput = hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
        bSuccess = CreateProcessA(nullptr,
                                  (char *) cmdLine,     // command line
                                  nullptr,          // process security attributes
                                  nullptr,          // primary thread security attributes
                                  TRUE,          // handles are inherited
                                  0,             // creation flags
                                  nullptr,          // use parent's environment
                                  nullptr,          // use parent's current directory
                                  &siStartInfo,  // STARTUPINFO pointer
                                  &piProcInfo);  // receives PROCESS_INFORMATION

        // If an error occurs, exit the application.
        if (!bSuccess)
            return -1;
        else {
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example.

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
        }

        if (write) {
            CloseHandle(hChildStd_OUT_Rd);
            hChildStd_OUT_Rd = nullptr;
            CloseHandle(hChildStd_OUT_Wr);
            hChildStd_OUT_Wr = nullptr;
            CloseHandle(hChildStd_ERR_Rd);
            hChildStd_ERR_Rd = nullptr;
            CloseHandle(hChildStd_ERR_Wr);
            hChildStd_ERR_Wr = nullptr;
            CloseHandle(hChildStd_IN_Rd);
            hChildStd_IN_Rd = nullptr;
        } else {
            CloseHandle(hChildStd_IN_Rd);
            hChildStd_IN_Rd = nullptr;
            CloseHandle(hChildStd_IN_Wr);
            hChildStd_IN_Wr = nullptr;
            //		CloseHandle(hChildStd_OUT_Rd);
            //		hChildStd_OUT_Rd = NULL;
            CloseHandle(hChildStd_OUT_Wr);
            hChildStd_OUT_Wr = nullptr;
            CloseHandle(hChildStd_ERR_Rd);
            hChildStd_ERR_Rd = nullptr;
            CloseHandle(hChildStd_ERR_Wr);
            hChildStd_ERR_Wr = nullptr;
        }
        return 0;
    }

    int Pipe::Close() {
        if (hChildStd_IN_Rd) {
            CloseHandle(hChildStd_IN_Rd);
            hChildStd_IN_Rd = nullptr;
        }

        if (hChildStd_IN_Wr) {
            CloseHandle(hChildStd_IN_Wr);
            hChildStd_IN_Wr = nullptr;
        }

        if (hChildStd_OUT_Rd) {
            CloseHandle(hChildStd_OUT_Rd);
            hChildStd_OUT_Rd = nullptr;
        }

        if (hChildStd_OUT_Wr) {
            CloseHandle(hChildStd_OUT_Wr);
            hChildStd_OUT_Wr = nullptr;
        }

        if (hChildStd_ERR_Rd) {
            CloseHandle(hChildStd_ERR_Rd);
            hChildStd_ERR_Rd = nullptr;
        }

        if (hChildStd_ERR_Wr) {
            CloseHandle(hChildStd_ERR_Wr);
            hChildStd_ERR_Wr = nullptr;
        }

        return 0;
    }

    size_t Pipe::Read(char *buffer, size_t size) {
        // check if the process terminated
        DWORD dwRead;
        BOOL bSuccess = ReadFile(hChildStd_OUT_Rd, buffer, size, &dwRead, nullptr);
        return bSuccess ? dwRead : 0;
    }

    size_t Pipe::Write(char *buffer, size_t size) {
        DWORD dwWritten;
        BOOL bSuccess = WriteFile(hChildStd_IN_Wr, buffer, size, &dwWritten, nullptr);
        return bSuccess ? dwWritten : 0;
    }

#endif

#define FEATURE_LINEBUFFER_AUTOBYPASS

    LineBuffer::LineBuffer(size_t buffer_size) : buffer_size(buffer_size) {
        buffer = new char[buffer_size + 1]; // one extra byte for \0
        capacity = buffer_size;
        cursor_buf = buffer;
    }

    LineBuffer::~LineBuffer() {
#ifdef FEATURE_LINEBUFFER_AUTOBYPASS
        if (bypass != 0)
#endif
            delete[] buffer;
    }

    bool LineBuffer::GetLine(const char *src, size_t max_count) {
        const char *newline;
#ifdef FEATURE_LINEBUFFER_AUTOBYPASS
        if (bypass == 0) {
            buffer = const_cast<char *>(src); // input buffer could be reallocated
            return max_count > 0;
        }
#endif
        if (cursor_src && cursor_src > src) { // continuation
            max_count -= cursor_src - src;
            src = cursor_src;
#ifdef FEATURE_LINEBUFFER_AUTOBYPASS
            bypass = -1; // permanently disable bypass
#endif
        }
        while ((newline = static_cast<const char *>(std::memchr(src, '\n', max_count)))) {
#ifdef FEATURE_LINEBUFFER_AUTOBYPASS
            if (newline == src + max_count - 1) // input was a whole line
                DecrBypass(src);
#endif
            size_t len = newline - src + 1;
            if (discard_line) {
                discard_line = false;
                goto discard;
            }
            if (Copy(src, len, true)) {
                cursor_src = newline + 1;
                return true;
            }
            discard:
            max_count -= len;
            src += len;
        }
        if (max_count != 0) { // save incomplete line
            if (!Copy(src, max_count, false)) { // discard entire over-length incomplete line
                discard_line = true;
            }
        }
        cursor_src = nullptr;
        return false;
    }

    bool LineBuffer::Copy(const char *src, size_t len, bool complete) {
        if (len <= capacity) {
            std::memcpy(cursor_buf, src, len);
            cursor_buf[len] = '\0';
            if (complete) { // release buffer for the next write
                capacity = buffer_size;
                cursor_buf = buffer;
            } else {
                cursor_buf += len;
                capacity -= len;
            }
            return true;
        } else { // reject entire line
            capacity = buffer_size;
            cursor_buf = buffer;
            return false;
        }
    }

    void LineBuffer::DecrBypass(const char *src) {
#ifdef FEATURE_LINEBUFFER_AUTOBYPASS
        if (bypass > 0)
            --bypass;
        if (bypass == 0) {
            delete[] buffer;
            buffer = const_cast<char *>(src);
        }
#endif
    }

    FlatlandObject::FlatlandObject() : FlatlandObject('?', '\x20') {

    }

    FlatlandObject::FlatlandObject(char type, char sep) : type(type), separator(sep) {

    }

    FlatlandObject::FlatlandObject(char type, char sep, const char *src, size_t buffer_size) :
            FlatlandObject(type, sep) {
        buffer_size = ValidBufferSize(src, buffer_size);
        this->PushValues(src + 2, buffer_size - 2);
    }

    void FlatlandObject::PushValues(const char *src, size_t buffer_size) {
        const char *p;
        while ((p = static_cast<const char *>(std::memchr(src, this->separator, buffer_size)))) {
            size_t len = p - src;
            this->values.emplace_back(src, len);
            src = p + 1;
            buffer_size -= len + 1;
        }
        if (buffer_size > 0)
            this->values.emplace_back(src, buffer_size);
    }

    FlatlandObject FlatlandObject::Parse(const char *src, size_t buffer_size) {
        buffer_size = ValidBufferSize(src, buffer_size);
        if (buffer_size == 0)
            return FlatlandObject();
        FlatlandObject obj(src[0], src[1]);
        obj.PushValues(src + 2, buffer_size - 2);
        return obj;
    }

    size_t FlatlandObject::ValidBufferSize(const char *src, size_t buffer_size) {
        const char *p = static_cast<const char *>(std::memchr(src, '\n', buffer_size));
        return p ? p - src : buffer_size;
    }

    void CreatorIWindow::GrantCreateWindow(std::function<void(std::unique_ptr<IWindow> &&)> cw) {
        this->CreateIWindow = std::move(cw);
    }

}

namespace spt::Math {
    const char *AspectRatioSS(float ratio) {
        if(std::abs(ratio-1.33f)<0.01f) {
            strcpy(spt::AppEngine::shared_buffer, "4:3"); // NOLINT
        }
        else if (std::abs(ratio-1.77f)<0.01f) {
            strcpy(spt::AppEngine::shared_buffer, "16:9"); // NOLINT
        }
        else {
            sprintf(spt::AppEngine::shared_buffer, "%.2f:1", ratio);
        }
        return spt::AppEngine::shared_buffer;
    }
}
