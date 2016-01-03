////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _COLORSHADERCLASS_H_
#define _COLORSHADERCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <d3d10.h>
#include <DirectXMath.h>
//#include <fstream>
#include <xstring>

////////////////////////////////////////////////////////////////////////////////
// Class name: ColorShaderClass
////////////////////////////////////////////////////////////////////////////////
class ColorShaderClass
{
public:
    ColorShaderClass();
    //ColorShaderClass(const ColorShaderClass&);
    //~ColorShaderClass();

    bool Initialize(ID3D10Device*, HWND);
    void Shutdown();
    void Render(ID3D10Device*, int, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&);

private:
    bool InitializeShader( ID3D10Device*, HWND, const std::wstring&, const std::wstring& );
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

    void SetShaderParameters(const DirectX::XMMATRIX&, const DirectX::XMMATRIX&, const DirectX::XMMATRIX&);
    void RenderShader(ID3D10Device*, int);

private:
    ID3D10Buffer* m_pConstantBuffer;
    ID3D10InputLayout* m_pVertexLayout;
    ID3D10PixelShader* m_pPixelShader;
    ID3D10VertexShader* m_pVertexShader;
    //ID3D10Effect* m_effect;
    //ID3D10EffectTechnique* m_technique;
    //ID3D10InputLayout* m_layout;

    //ID3D10EffectMatrixVariable* m_worldMatrixPtr;
    //ID3D10EffectMatrixVariable* m_viewMatrixPtr;
    //ID3D10EffectMatrixVariable* m_projectionMatrixPtr;
};

#endif