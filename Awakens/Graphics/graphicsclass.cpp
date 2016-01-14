////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "graphicsclass.h"
#include <new>

using namespace DirectX;


GraphicsClass::GraphicsClass()
{
    m_D3D = 0;
    m_Camera = 0;
    m_Model = 0;
    m_ColorShader = 0;
    m_TextureShader = 0;
}


GraphicsClass::GraphicsClass(const GraphicsClass& other)
{
}


GraphicsClass::~GraphicsClass()
{
}


bool GraphicsClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
    bool result;

    // Create the Direct3D object.
    // Aligned because it contains XMMATRIX members that are declared '__declspec(align(16)) struct XMMATRIX' in C:\Program Files (x86)\Windows Kits\10\Include\10.0.10240.0\um\DirectXMath.h
    void* alignedD3DPtr = _aligned_malloc( sizeof( D3DClass ), 16 );
    if( !alignedD3DPtr )
    {
        return false;
    }
    m_D3D = new ( alignedD3DPtr ) D3DClass;
    if(!m_D3D)
    {
        return false;
    }

    // Initialize the Direct3D object.
    result = m_D3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
        return false;
    }

    // Create the camera object.
    // Aligned because it contains XMMATRIX members that are declared '__declspec(align(16)) struct XMMATRIX' in C:\Program Files (x86)\Windows Kits\10\Include\10.0.10240.0\um\DirectXMath.h  
    void* alignedCameraPtr = _aligned_malloc( sizeof( CameraClass ), 16 );
    if( !alignedCameraPtr )
    {
        return false;
    }
    m_Camera = new ( alignedCameraPtr ) CameraClass;
    if(!m_Camera)
    {
        return false;
    }

    // Set the initial position of the camera.
    m_Camera->SetPosition(0.0f, 0.0f, -10.0f);
    
    // Create the model object.
    m_Model = new ModelClass;
    if(!m_Model)
    {
        return false;
    }

    // Initialize the model object.
    result = m_Model->Initialize(m_D3D->GetDevice(), L"data/seafloor.dds");
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
        return false;
    }

    // Create the color shader object.
    m_ColorShader = new ColorShaderClass;
    if(!m_ColorShader)
    {
        return false;
    }

    // Initialize the color shader object.
    result = m_ColorShader->Initialize(m_D3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the color shader object.", L"Error", MB_OK);
        return false;
    }

    // Create the texture shader object.
    m_TextureShader = new TextureShaderClass;
    if(!m_TextureShader)
    {
        return false;
    }

    // Initialize the texture shader object.
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if(!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }

    return true;
}


void GraphicsClass::Shutdown()
{
    // Release the texture shader object.
    if(m_TextureShader)
    {
        m_TextureShader->Shutdown();
        delete m_TextureShader;
        m_TextureShader = 0;
    }

    // Release the color shader object.
    if(m_ColorShader)
    {
        m_ColorShader->Shutdown();
        delete m_ColorShader;
        m_ColorShader = 0;
    }

    // Release the model object.
    if(m_Model)
    {
        m_Model->Shutdown();
        delete m_Model;
        m_Model = 0;
    }

    // Release the camera object.
    if(m_Camera)
    {
        _aligned_free( reinterpret_cast< void* >( m_Camera ) );
        m_Camera = 0;
    }

    // Release the D3D object.
    if(m_D3D)
    {
        m_D3D->Shutdown();
        _aligned_free( reinterpret_cast< void* >( m_D3D ) );
        m_D3D = 0;
    }

    return;
}


bool GraphicsClass::Frame()
{
    bool result;


    // Render the graphics scene.
    result = Render();
    if(!result)
    {
        return false;
    }

    return true;
}


bool GraphicsClass::Render()
{
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix;

    // Clear the buffers to begin the scene.
    m_D3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

    // Generate the view matrix based on the camera's position.
    m_Camera->Render();

    // Get the world, view, and projection matrices from the camera and d3d objects.
    m_Camera->GetViewMatrix(viewMatrix);
    m_D3D->GetWorldMatrix(worldMatrix);
    m_D3D->GetProjectionMatrix(projectionMatrix);

    // Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
    m_Model->Render(m_D3D->GetDevice());

    //// Render the model using the color shader.
    //m_ColorShader->Render(m_D3D->GetDevice(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix);

    // Render the model using the texture shader.
    m_TextureShader->Render(m_D3D->GetDevice(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture());


    // Present the rendered scene to the screen.
    m_D3D->EndScene();

    return true;
}