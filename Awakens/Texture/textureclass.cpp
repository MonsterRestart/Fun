////////////////////////////////////////////////////////////////////////////////
// Filename: textureclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "textureclass.h"
#include "BlobResource/blobResource.h"
#include <D3Dcompiler.h>
#include <fstream>
#include <sstream>
#include <comdef.h>
#include <assert.h>
#include "DDS/DDSTextureLoader.h"

using namespace std;

TextureClass::TextureClass()
    : m_texture( nullptr )
{
}

void textureInitializeError( const HRESULT result, const char* const file, const int line, const wstring& textureFilename )
{
    const _com_error error( result );
    wstringstream errorString;
    errorString << file << "(" << line << "): error: TextureClass::Initialize() [ " << result << " ] " << error.ErrorMessage();
    errorString << " - " << textureFilename << endl;
    OutputDebugString( errorString.str().c_str() );
}

bool TextureClass::Initialize( ID3D10Device* device, const wstring& textureFilename )
{
    const HRESULT readTextureResult = CreateDDSTextureFromFile( device, textureFilename.c_str(), &m_texture );
    if( FAILED( readTextureResult ) )
    {
        textureInitializeError( readTextureResult, __FILE__, __LINE__, textureFilename );
        return false;
    }

    return true;
}


void TextureClass::Shutdown()
{
    // Release the texture resource.
    if(m_texture)
    {
        m_texture->Release();
        m_texture = 0;
    }

    return;
}


ID3D10ShaderResourceView* TextureClass::GetTexture()
{
    return m_texture;
}