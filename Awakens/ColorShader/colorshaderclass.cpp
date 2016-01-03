////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "colorshaderclass.h"
#include <D3Dcompiler.h>
#include <fstream>
#include <sstream>
//#include <ios>
//#include <ostream> // <iostream>
#include <comdef.h>

using namespace DirectX;
using namespace std;

ColorShaderClass::ColorShaderClass()
    : m_pConstantBuffer( nullptr )
    , m_pVertexLayout( nullptr )
    , m_pPixelShader( nullptr )
    , m_pVertexShader( nullptr )
{
    //m_effect = 0;
    //m_technique = 0;
    //m_layout = 0;

    //m_worldMatrixPtr = 0;
    //m_viewMatrixPtr = 0;
    //m_projectionMatrixPtr = 0;
}


//ColorShaderClass::ColorShaderClass(const ColorShaderClass& other)
//{
//}


//ColorShaderClass::~ColorShaderClass()
//{
//}


bool ColorShaderClass::Initialize(ID3D10Device* device, HWND hwnd)
{
#if defined( _WIN64 )
#if defined( DEBUG ) || defined( _DEBUG )
    const wstring pixelShaderFilename( L"x64/Debug/colorPS.cso" );
    const wstring vertexShaderFilename( L"x64/Debug/colorVS.cso" );
#else
    const wstring pixelShaderFilename( L"x64/Release/colorPS.cso" );
    const wstring vertexShaderFilename( L"x64/Release/colorVS.cso" );
#endif
#else
#if defined( DEBUG ) || defined( _DEBUG )
    const wstring pixelShaderFilename( L"Debug/colorPS.cso" );
    const wstring vertexShaderFilename( L"Debug/colorVS.cso" );
#else
    const wstring pixelShaderFilename( L"Release/colorPS.cso" );
    const wstring vertexShaderFilename( L"Release/colorVS.cso" );
#endif
#endif

    return InitializeShader( device, hwnd, pixelShaderFilename, vertexShaderFilename );
}


void ColorShaderClass::Shutdown()
{
    // Shutdown the shader effect.
    ShutdownShader();

    return;
}


void ColorShaderClass::Render(ID3D10Device* device, int indexCount, const DirectX::XMMATRIX& worldMatrix, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projectionMatrix)
{
    // Set the shader parameters that it will use for rendering.
    SetShaderParameters(worldMatrix, viewMatrix, projectionMatrix);

    // Now render the prepared buffers with the shader.
    RenderShader(device, indexCount);

    return;
}

void colorShaderInitializeShaderError( const HRESULT result, const char* const file, const int line, const wstring& shaderFilename, const HWND hwnd )
{
    const _com_error error( result );
    wstringstream errorString;
    errorString << file << "(" << line << "): error: ColorShaderClass::InitializeShader() [ " << result << " ] " << error.ErrorMessage();
    errorString << " - " << shaderFilename << endl;
    OutputDebugString( errorString.str().c_str() );
    MessageBox( hwnd, errorString.str().c_str(), L"ColorShaderClass::InitializeShader Error",  MB_OK );
}

struct VS_CONSTANT_BUFFER
{
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

bool ColorShaderClass::InitializeShader( ID3D10Device* device, HWND hwnd, const wstring& pixelShaderFilename, const wstring& vertexShaderFilename )
{
    assert( device );

    struct BlobResource
    {
        ID3DBlob* m_pBlob;

    public:
        BlobResource()
            : m_pBlob( nullptr )
        {
        }

        ~BlobResource()
        {
            if( m_pBlob )
            {
                m_pBlob->Release();
            }
        }
    };

    // Read compiled Pixel shader file in to blob.
    BlobResource pixelShaderBlobResource;
    const HRESULT readPixelShaderResult = D3DReadFileToBlob( pixelShaderFilename.c_str(), &pixelShaderBlobResource.m_pBlob );
    if( FAILED( readPixelShaderResult ) )
    {
        colorShaderInitializeShaderError( readPixelShaderResult, __FILE__, __LINE__, pixelShaderFilename, hwnd );
        return false;
    }
    assert( pixelShaderBlobResource.m_pBlob );

    const HRESULT createPixelShaderResult = device->CreatePixelShader( pixelShaderBlobResource.m_pBlob->GetBufferPointer(), pixelShaderBlobResource.m_pBlob->GetBufferSize(), &m_pPixelShader );
    if( FAILED( createPixelShaderResult ) )
    {
        colorShaderInitializeShaderError( createPixelShaderResult, __FILE__, __LINE__, pixelShaderFilename, hwnd );
        return false;
    }
    assert( m_pPixelShader );

    // Read compiled vertex shader file in to blob.
    BlobResource vertexShaderBlobResource;
    const HRESULT readVertexShaderResult = D3DReadFileToBlob( vertexShaderFilename.c_str(), &vertexShaderBlobResource.m_pBlob );
    if( FAILED( readVertexShaderResult ) )
    {
        colorShaderInitializeShaderError( readVertexShaderResult, __FILE__, __LINE__, vertexShaderFilename, hwnd );
        return false;
    }
    assert( vertexShaderBlobResource.m_pBlob );

    const HRESULT createVertexShaderResult = device->CreateVertexShader( vertexShaderBlobResource.m_pBlob->GetBufferPointer(), vertexShaderBlobResource.m_pBlob->GetBufferSize(), &m_pVertexShader );
    if( FAILED( createVertexShaderResult ) )
    {
        colorShaderInitializeShaderError( createVertexShaderResult, __FILE__, __LINE__, vertexShaderFilename, hwnd );
        return false;
    }
    assert( m_pVertexShader );
    
    // Create a constant buffer
    D3D10_BUFFER_DESC cbDesc;
    cbDesc.ByteWidth = sizeof( VS_CONSTANT_BUFFER );
    cbDesc.Usage = D3D10_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    const HRESULT createBufferResult = device->CreateBuffer( &cbDesc, NULL, &m_pConstantBuffer );
    if( FAILED( createBufferResult ) )
    {
        colorShaderInitializeShaderError( createBufferResult, __FILE__, __LINE__, vertexShaderFilename, hwnd );
        return false;
    }
    assert( m_pConstantBuffer );

    
    // Create our vertex input layout
    const D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D10_APPEND_ALIGNED_ELEMENT,  D3D10_INPUT_PER_VERTEX_DATA,    0 },
        { "COLOR",      0,  DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D10_APPEND_ALIGNED_ELEMENT,  D3D10_INPUT_PER_VERTEX_DATA,    0 },
    };
    const HRESULT createInputLayerResult = device->CreateInputLayout( 
        layout,
        _countof( layout ), // sizeof( layout ) / sizeof( D3D10_INPUT_ELEMENT_DESC ),
        vertexShaderBlobResource.m_pBlob->GetBufferPointer(),
        vertexShaderBlobResource.m_pBlob->GetBufferSize(),
        &m_pVertexLayout );
    if( FAILED( createInputLayerResult ) )
    {
        colorShaderInitializeShaderError( createInputLayerResult, __FILE__, __LINE__, vertexShaderFilename, hwnd );
        return false;
    }
    assert( m_pVertexLayout );

    // Get pointers to the three matrices inside the shader so we can update them from this class.
    //m_worldMatrixPtr = m_pVertexShader->GetVariableByName("worldMatrix")->AsMatrix();
    //m_viewMatrixPtr = m_effect->GetVariableByName("viewMatrix")->AsMatrix();
    //m_projectionMatrixPtr = m_effect->GetVariableByName("projectionMatrix")->AsMatrix();

    return true;
}

void ColorShaderClass::ShutdownShader()
{
    //// Release the pointers to the matrices inside the shader.
    //m_worldMatrixPtr = 0;
    //m_viewMatrixPtr = 0;
    //m_projectionMatrixPtr = 0;

    if( m_pConstantBuffer )
    {
        m_pConstantBuffer->Release();
        m_pConstantBuffer = nullptr;
    }

    if( m_pVertexLayout )
    {
        m_pVertexLayout->Release();
        m_pVertexLayout = nullptr;
    }

    if( m_pPixelShader )
    {
        m_pPixelShader->Release();
        m_pPixelShader = nullptr;
    }

    if( m_pVertexShader )
    {
        m_pVertexShader->Release();
        m_pVertexShader = nullptr;
    }

    //// Release the pointer to the shader layout.
    //if(m_layout)
    //{
    //    m_layout->Release();
    //    m_layout = 0;
    //}

    //// Release the pointer to the shader technique.
    //m_technique = 0;

    //// Release the pointer to the shader.
    //if(m_effect)
    //{
    //    m_effect->Release();
    //    m_effect = 0;
    //}

    return;
}

void ColorShaderClass::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
    char* compileErrors;
    size_t bufferSize, i;
    ofstream fout;


    // Get a pointer to the error message text buffer.
    compileErrors = (char*)(errorMessage->GetBufferPointer());

    // Get the length of the message.
    bufferSize = errorMessage->GetBufferSize();

    // Open a file to write the error message to.
    fout.open("shader-error.txt");

    // Write out the error message.
    for(i=0; i<bufferSize; i++)
    {
        fout << compileErrors[i];
    }

    // Close the file.
    fout.close();

    // Release the error message.
    errorMessage->Release();
    errorMessage = 0;

    // Pop a message up on the screen to notify the user to check the text file for compile errors.
    MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

    return;
}

void ColorShaderClass::SetShaderParameters(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    assert( m_pConstantBuffer );
    assert( m_pPixelShader );
    assert( m_pVertexShader );

    // Update the Constant Buffer
    VS_CONSTANT_BUFFER* pConstData;
    m_pConstantBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&pConstData );
    pConstData->worldMatrix = worldMatrix;
    pConstData->viewMatrix = viewMatrix;
    pConstData->projectionMatrix = projectionMatrix;
    m_pConstantBuffer->Unmap();

    //// Set the world matrix variable inside the shader.
    //m_worldMatrixPtr->SetMatrix((float*)&worldMatrix);

    //// Set the view matrix variable inside the shader.
    //m_viewMatrixPtr->SetMatrix((float*)&viewMatrix);

    //// Set the projection matrix variable inside the shader.
    //m_projectionMatrixPtr->SetMatrix((float*)&projectionMatrix);
}


void ColorShaderClass::RenderShader(ID3D10Device* device, int indexCount)
{
    device->IASetInputLayout( m_pVertexLayout );

    ID3D10Buffer* pBuffers[1] = { m_pConstantBuffer };
    device->VSSetConstantBuffers( 0, 1, pBuffers );
    //device->OMSetBlendState( g_pBlendStateNoBlend, 0, 0xffffffff );
    //device->RSSetState( g_pRasterizerStateNoCull );
    //device->OMSetDepthStencilState( g_pLessEqualDepth, 0 );
    device->VSSetShader( m_pVertexShader );
    device->GSSetShader( nullptr );
    device->PSSetShader( m_pPixelShader );
    device->DrawIndexed(indexCount, 0, 0);

    



    //D3D10_TECHNIQUE_DESC techniqueDesc;
    //unsigned int i;
    

    // Set the input layout.
    //device->IASetInputLayout(m_layout);

    // Get the description structure of the technique from inside the shader so it can be used for rendering.
    //m_technique->GetDesc(&techniqueDesc);

    // Go through each pass in the technique (should be just one currently) and render the triangles.
    //for(i=0; i<techniqueDesc.Passes; ++i)
    //{
    //    m_technique->GetPassByIndex(i)->Apply(0);
    //    device->DrawIndexed(indexCount, 0, 0);
    //}
}