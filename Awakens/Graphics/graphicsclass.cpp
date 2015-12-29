////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "graphicsclass.h"
#include <new>


GraphicsClass::GraphicsClass()
{
    m_D3D = 0;
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
    void* ptr = _aligned_malloc( sizeof( D3DClass ), 16 );
    if( !ptr )
    {
        return false;
    }
    m_D3D = new ( ptr ) D3DClass;
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

    return true;
}


void GraphicsClass::Shutdown()
{
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
    // Clear the buffers to begin the scene.
    m_D3D->BeginScene(0.5f, 0.5f, 0.5f, 1.0f);


    // Present the rendered scene to the screen.
    m_D3D->EndScene();

    return true;
}