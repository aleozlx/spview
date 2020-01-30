#ifndef __TEXIMAGE_HPP__
#define __TEXIMAGE_HPP__

// *Note* https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

#include "imgui.h"

#if FEATURE_DirectX
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#endif

#if FEATURE_OpenGL
#include <GL/glew.h>
#include <SOIL/SOIL.h>
#endif

namespace spt {
/// Management of image data with DirectX/OpenGL texture
    class TexImage {
    private:
#if FEATURE_DirectX
        ID3D11Device *g_pd3dDevice;
		ID3D11DeviceContext* g_pd3dDeviceContext;
        ID3D11ShaderResourceView *texid;
		D3D11_TEXTURE2D_DESC tex_desc;
		ID3D11Texture2D *pTexture;
		//D3D11_MAPPED_SUBRESOURCE mappedResource;
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
#if FEATURE_DirectX
			g_pd3dDevice = AppEngine::App::GetDXDevice();
			g_pd3dDeviceContext = AppEngine::App::GetDXDeviceContext();
			ZeroMemory(&tex_desc, sizeof(tex_desc));
			tex_desc.Width = width;
			tex_desc.Height = height;
			tex_desc.MipLevels = 1;
			tex_desc.ArraySize = 1;
			// TODO add support for `channels`
			// ref: https://docs.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
			tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			tex_desc.SampleDesc.Count = 1;
			tex_desc.Usage = D3D11_USAGE_DYNAMIC;
			tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			pTexture = NULL;
			//ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
#endif
        }

		virtual ~TexImage() {
			if (pTexture != NULL) pTexture->Release();
		}

        inline ImTextureID id() {
            return reinterpret_cast<void *>(texid);
        }

        void Load(const unsigned char *data) {
#if FEATURE_DirectX
			if (pTexture == NULL) {
				D3D11_SUBRESOURCE_DATA subResource;
				subResource.pSysMem = data;
				subResource.SysMemPitch = tex_desc.Width * 4;
				subResource.SysMemSlicePitch = 0;
				g_pd3dDevice->CreateTexture2D(&tex_desc, &subResource, &pTexture);
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				ZeroMemory(&srvDesc, sizeof(srvDesc));
				srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = tex_desc.MipLevels;
				srvDesc.Texture2D.MostDetailedMip = 0;
				g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &texid);
			}
			else {
				// Cannot figure out the issue with texture format
				// ref: https://docs.microsoft.com/en-us/windows/win32/direct3d11/how-to--use-dynamic-resources
				//g_pd3dDeviceContext->Map(pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
				//memcpy(mappedResource.pData, data, width*height*4); // TODO hard-coded size
				//g_pd3dDeviceContext->Unmap(pTexture, 0);

				// UpdateSubresource is more efficient anyway 
				g_pd3dDeviceContext->UpdateSubresource(pTexture, 0, NULL, data, tex_desc.Width * 4, 0);
			}
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
