// Andrew Davies

#if !defined( DEV_GRAPHICS_H )
#define DEV_GRAPHICS_H

#include "DEvGraphics/devGraphicsFwd.h"
#include <DirectXMath.h>

namespace DevGraphics
{

void line(
    int UID,
    DirectX::XMVECTOR const & fromPosition,
    DirectX::XMVECTOR const & toPosition,
    DirectX::XMVECTOR const & fromColour = { 1.0f, 1.0f, 1.0f, 1.0f },
    DirectX::XMVECTOR const & toColour = { 1.0f, 1.0f, 1.0f, 1.0f },
    bool infiniteLife = false,
    float lifeTime = 0.1f );

void removeLine( int UID );

#define DEV_GRAPHICS_DEFAULT_POINT_SIZE ( 8.0f )

void point(
    int UID,
    DirectX::XMVECTOR const & position,
    float size = DEV_GRAPHICS_DEFAULT_POINT_SIZE,
    DirectX::XMVECTOR const & colour = { 1.0f, 1.0f, 1.0f, 1.0f },
    bool infiniteLife = false,
    float lifeTime = 0.1f );

void removePoint( int UID );

void step( float deltaTime );
void draw(); // IDirect3DDevice9 & d3dDevice );

}

#endif