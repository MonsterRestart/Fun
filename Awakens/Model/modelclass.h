////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <d3d10.h>
#include <DirectXMath.h>
#include "Texture/textureclass.h"


////////////////////////////////////////////////////////////////////////////////
// Class name: ModelClass
////////////////////////////////////////////////////////////////////////////////
class ModelClass
{
private:
    struct VertexType
    {
        DirectX::XMVECTOR position;
        //DirectX::XMVECTOR color;
        DirectX::XMFLOAT2 texture;
    };

public:
    ModelClass();

    bool Initialize(ID3D10Device*, WCHAR*);
    void Shutdown();
    void Render(ID3D10Device*);

    int GetIndexCount();
    ID3D10ShaderResourceView* GetTexture();

private:
    bool InitializeBuffers(ID3D10Device*);
    void ShutdownBuffers();
    void RenderBuffers(ID3D10Device*);

    bool LoadTexture(ID3D10Device*, WCHAR*);
    void ReleaseTexture();

private:
    ID3D10Buffer *m_vertexBuffer, *m_indexBuffer;
    int m_vertexCount, m_indexCount;
    TextureClass* m_Texture;
};

#endif