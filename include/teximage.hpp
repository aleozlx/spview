#ifndef __TEXIMAGE_HPP__
#define __TEXIMAGE_HPP__

// *Note* https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

#include "imgui.h"

#if FEATURE_DirectX
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

//namespace spview::AppEngine {
//    extern ID3D11Device* g_pd3dDevice;
//}
#endif

#if FEATURE_OpenGL
#include <GL/glew.h>
#include <SOIL/SOIL.h>
#endif

namespace spview {
/// Management of image data with DirectX/OpenGL texture
    class TexImage {
    private:
#if FEATURE_DirectX
        ID3D11Device *g_pd3dDevice;
        ID3D11ShaderResourceView *texid;
#elif FEATURE_OpenGL
        GLuint texid;
#endif

    public:
        unsigned int width, height, channels;
        float f32width, f32height;

        TexImage(unsigned int width=0, unsigned int height=0, unsigned int channels=0) {
            this->texid = 0; // The value zero is reserved to represent the default texture for each texture target - Khronos
            this->width = width;
            this->f32width = (float) width;
            this->height = height;
            this->f32height = (float) height;
            this->channels = channels;
        }

        inline ImTextureID id() {
            return reinterpret_cast<void *>(texid);
        }

        void Load(const unsigned char *data) {
#if FEATURE_DirectX
            g_pd3dDevice = AppEngine::App::GetDXDevice();
            // Create texture
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            ID3D11Texture2D *pTexture = NULL;
            D3D11_SUBRESOURCE_DATA subResource;
            subResource.pSysMem = data;
            subResource.SysMemPitch = desc.Width * 4;
            subResource.SysMemSlicePitch = 0;
            g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

            // Create texture view
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &texid);
            pTexture->Release();
#elif FEATURE_OpenGL
            this->texid = SOIL_create_OGL_texture(
                data, width, height, channels,
                this->texid, SOIL_FLAG_MIPMAPS | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
            );
#endif
        }

        inline ImVec2 uv(float px = 0.0f, float py = 0.0f) {
            return ImVec2(px / f32width, py / f32height);
        }

        inline ImVec2 size() {
            return ImVec2(f32width, f32height);
        }
    };
}
#endif
