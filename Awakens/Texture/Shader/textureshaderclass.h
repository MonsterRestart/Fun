////////////////////////////////////////////////////////////////////////////////
// Filename: textureshaderclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _TEXTURESHADERCLASS_H_
#define _TEXTURESHADERCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <d3d10.h>
#include <DirectXMath.h>
#include <xstring>

////////////////////////////////////////////////////////////////////////////////
// Class name: TextureShaderClass
////////////////////////////////////////////////////////////////////////////////
class TextureShaderClass
{
public:
    TextureShaderClass();

    bool Initialize(ID3D10Device*, HWND);
    void Shutdown();
    void Render(ID3D10Device*, int, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&, ID3D10ShaderResourceView*);

private:
    bool InitializeShader( ID3D10Device*, HWND, const std::wstring&, const std::wstring& );
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

    void SetShaderParameters(ID3D10Device* device, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&, ID3D10ShaderResourceView*);
    void RenderShader(ID3D10Device*, int);

private:
    ID3D10Buffer* m_pConstantBuffer;
    ID3D10InputLayout* m_pVertexLayout;
    ID3D10PixelShader* m_pPixelShader;
    ID3D10VertexShader* m_pVertexShader;
};

#endif