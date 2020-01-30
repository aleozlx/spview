#include "..\include\app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "app.hpp"

#if FEATURE_DirectX

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

namespace spt::AppEngine {
#if FEATURE_DirectX
    WNDCLASSEX wc;
    HWND hwnd;
#endif

#if FEATURE_OpenGL
    GLFWwindow* window;
#endif


#if FEATURE_DirectX
    static ID3D11Device *g_pd3dDevice = NULL;
    static ID3D11DeviceContext *g_pd3dDeviceContext = NULL;
    static IDXGISwapChain *g_pSwapChain = NULL;
    static ID3D11RenderTargetView *g_mainRenderTargetView = NULL;

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
            if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
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
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
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

    App App::Initialize() {
        // Create application window
        wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
              _T("ImGui Example"), NULL};
        ::RegisterClassEx(&wc);
        hwnd = ::CreateWindow(wc.lpszClassName, _T("Superpixel Analyzer"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800,
                              NULL, NULL, wc.hInstance, NULL);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd)) {
            CleanupDeviceD3D();
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);
            return App::Err();
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

        return App::Ok();
    }

#endif

#if FEATURE_OpenGL
    App App::Initialize() {
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
            return App::Err();
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

        return App::Ok(glsl_version);
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
        if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
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
            g_pSwapChain = NULL;
        }
        if (g_pd3dDeviceContext) {
            g_pd3dDeviceContext->Release();
            g_pd3dDeviceContext = NULL;
        }
        if (g_pd3dDevice) {
            g_pd3dDevice->Release();
            g_pd3dDevice = NULL;
        }
    }

    void CreateRenderTarget() {
        ID3D11Texture2D *pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    void CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = NULL;
        }
    }

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg) {
            case WM_SIZE:
                if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
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
        hChildStd_IN_Rd = NULL;
        hChildStd_IN_Wr = NULL;

        hChildStd_OUT_Rd = NULL;
        hChildStd_OUT_Wr = NULL;

        hChildStd_ERR_Rd = NULL;
        hChildStd_ERR_Wr = NULL;
    }

    int Pipe::Open(const char *cmdLine, bool write) {
        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited.

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

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
        bSuccess = CreateProcessA(NULL,
                                  (char *) cmdLine,     // command line
                                  NULL,          // process security attributes
                                  NULL,          // primary thread security attributes
                                  TRUE,          // handles are inherited
                                  0,             // creation flags
                                  NULL,          // use parent's environment
                                  NULL,          // use parent's current directory
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
            hChildStd_OUT_Rd = NULL;
            CloseHandle(hChildStd_OUT_Wr);
            hChildStd_OUT_Wr = NULL;
            CloseHandle(hChildStd_ERR_Rd);
            hChildStd_ERR_Rd = NULL;
            CloseHandle(hChildStd_ERR_Wr);
            hChildStd_ERR_Wr = NULL;
            CloseHandle(hChildStd_IN_Rd);
            hChildStd_IN_Rd = NULL;
        } else {
            CloseHandle(hChildStd_IN_Rd);
            hChildStd_IN_Rd = NULL;
            CloseHandle(hChildStd_IN_Wr);
            hChildStd_IN_Wr = NULL;
            //		CloseHandle(hChildStd_OUT_Rd);
            //		hChildStd_OUT_Rd = NULL;
            CloseHandle(hChildStd_OUT_Wr);
            hChildStd_OUT_Wr = NULL;
            CloseHandle(hChildStd_ERR_Rd);
            hChildStd_ERR_Rd = NULL;
            CloseHandle(hChildStd_ERR_Wr);
            hChildStd_ERR_Wr = NULL;
        }
        return 0;
    }

    int Pipe::Close() {
        if (hChildStd_IN_Rd) {
            CloseHandle(hChildStd_IN_Rd);
            hChildStd_IN_Rd = NULL;
        }

        if (hChildStd_IN_Wr) {
            CloseHandle(hChildStd_IN_Wr);
            hChildStd_IN_Wr = NULL;
        }

        if (hChildStd_OUT_Rd) {
            CloseHandle(hChildStd_OUT_Rd);
            hChildStd_OUT_Rd = NULL;
        }

        if (hChildStd_OUT_Wr) {
            CloseHandle(hChildStd_OUT_Wr);
            hChildStd_OUT_Wr = NULL;
        }

        if (hChildStd_ERR_Rd) {
            CloseHandle(hChildStd_ERR_Rd);
            hChildStd_ERR_Rd = NULL;
        }

        if (hChildStd_ERR_Wr) {
            CloseHandle(hChildStd_ERR_Wr);
            hChildStd_ERR_Wr = NULL;
        }

        return 0;
    }

    size_t Pipe::Read(char *buffer, size_t size) {
        // check if the process terminated
        DWORD dwRead;
        BOOL bSuccess = ReadFile(hChildStd_OUT_Rd, buffer, size, &dwRead, NULL);
        return bSuccess ? dwRead : 0;
    }

    size_t Pipe::Write(char *buffer, size_t size) {
        DWORD dwWritten;
        BOOL bSuccess = WriteFile(hChildStd_IN_Wr, buffer, size, &dwWritten, NULL);
        return bSuccess ? dwWritten : 0;
    }
#endif

}
