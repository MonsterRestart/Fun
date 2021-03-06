// Andrew Davies

#include "pch.h"
#include "DevGraphics/devGraphics.h"
#pragma warning (disable: 4530)
#include <map>
#pragma warning (default: 4530)

//using namespace std;
using namespace DirectX;

namespace DevGraphics
{

#define D3DFVF_LINEVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)
struct LineVertexStruct
{
    float x, y, z;
    float r, g, b, a; //DWORD colour;
};

#define D3DFVF_POINTVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_PSIZE)
struct PointVertexStruct
{
    float x, y, z;
    float size;
    float r, g, b, a; //DWORD colour;
};

class BaseClass
{
private:
    bool m_infiniteLife;
    float m_lifeTime;

public:
    BaseClass( )
        : m_infiniteLife( true )
        , m_lifeTime( 0.0f )
    {
    }

    BaseClass(
        bool const infiniteLife,
        float const lifeTime )
        : m_infiniteLife( infiniteLife )
        , m_lifeTime( lifeTime )
    {
    }

    bool infiniteLife() const
    {
        return m_infiniteLife;
    }

    float lifeTime() const
    {
        return m_lifeTime;
    }

    void step( float const deltaTime )
    {
        if( !m_infiniteLife )
        {
            m_lifeTime -= deltaTime;
        }
    }
};

class LineClass : public BaseClass
{
private:
    XMVECTOR m_fromPosition;
    XMVECTOR m_toPosition;

    XMVECTOR m_fromColour;
    XMVECTOR m_toColour;
    
    LineVertexStruct m_vertexArray[ 2 ];

public:
    LineClass( )
        : BaseClass( true, 0.0f )
        , m_fromPosition( XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ) )
        , m_toPosition( XMVectorSet( 0.0f, 50.0f, 0.0f, 1.0f ) )
        , m_fromColour( XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f ) )
        , m_toColour( XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f ) )
    {
        m_vertexArray[ 0 ].x = XMVectorGetX( m_fromPosition );
        m_vertexArray[ 0 ].y = XMVectorGetY( m_fromPosition );
        m_vertexArray[ 0 ].z = XMVectorGetZ( m_fromPosition );
        m_vertexArray[ 0 ].r = XMVectorGetX( m_fromColour );
        m_vertexArray[ 0 ].g = XMVectorGetY( m_fromColour );
        m_vertexArray[ 0 ].b = XMVectorGetZ( m_fromColour );
        m_vertexArray[ 0 ].a = XMVectorGetW( m_fromColour );
        //m_vertexArray[ 0 ].colour = D3DCOLOR_COLORVALUE( 
        //    XMVectorGetX( m_fromColour ),
        //    XMVectorGetY( m_fromColour ), 
        //    XMVectorGetZ( m_fromColour ), 
        //    XMVectorGetW( m_fromColour ) );
        m_vertexArray[ 1 ].x = XMVectorGetX( m_toPosition );
        m_vertexArray[ 1 ].y = XMVectorGetY( m_toPosition );
        m_vertexArray[ 1 ].z = XMVectorGetZ( m_toPosition );
        m_vertexArray[ 1 ].r = XMVectorGetX( m_toColour );
        m_vertexArray[ 1 ].g = XMVectorGetY( m_toColour );
        m_vertexArray[ 1 ].b = XMVectorGetZ( m_toColour );
        m_vertexArray[ 1 ].a = XMVectorGetW( m_toColour );
        //m_vertexArray[ 1 ].colour = D3DCOLOR_COLORVALUE( 
        //    XMVectorGetX( m_toColour ),
        //    XMVectorGetY( m_toColour ), 
        //    XMVectorGetZ( m_toColour ), 
        //    XMVectorGetW( m_toColour ) );
    }

    LineClass(
        bool const infiniteLife,
        float const lifeTime,
        XMVECTOR const & fromPosition,
        XMVECTOR const & toPosition,
        XMVECTOR const & fromColour,
        XMVECTOR const & toColour )
        : BaseClass( infiniteLife, lifeTime )
        , m_fromPosition( fromPosition )
        , m_toPosition( toPosition )
        , m_fromColour( fromColour )
        , m_toColour( toColour )
    {
        m_vertexArray[ 0 ].x = XMVectorGetX( fromPosition );
        m_vertexArray[ 0 ].y = XMVectorGetY( fromPosition );
        m_vertexArray[ 0 ].z = XMVectorGetZ( fromPosition );
        m_vertexArray[ 0 ].r = XMVectorGetX( fromColour );
        m_vertexArray[ 0 ].g = XMVectorGetY( fromColour );
        m_vertexArray[ 0 ].b = XMVectorGetZ( fromColour );
        m_vertexArray[ 0 ].a = XMVectorGetW( fromColour );
        //m_vertexArray[ 0 ].colour = D3DCOLOR_COLORVALUE( 
        //    fromColour.x,
        //    fromColour.y, 
        //    fromColour.z, 
        //    fromColour.w );
        m_vertexArray[ 1 ].x = XMVectorGetX( toPosition );
        m_vertexArray[ 1 ].y = XMVectorGetY( toPosition );
        m_vertexArray[ 1 ].z = XMVectorGetZ( toPosition );
        m_vertexArray[ 1 ].r = XMVectorGetX( toColour );
        m_vertexArray[ 1 ].g = XMVectorGetY( toColour );
        m_vertexArray[ 1 ].b = XMVectorGetZ( toColour );
        m_vertexArray[ 1 ].a = XMVectorGetW( toColour );
        //m_vertexArray[ 1 ].colour = D3DCOLOR_COLORVALUE( 
        //    toColour.x,
        //    toColour.y, 
        //    toColour.z, 
        //    toColour.w );
    }

    XMVECTOR const & fromPosition() const
    {
        return m_fromPosition;
    }

    XMVECTOR const & toPosition() const
    {
        return m_toPosition;
    }

    XMVECTOR const & fromColour() const
    {
        return m_fromColour;
    }

    XMVECTOR const & toColour() const
    {
        return m_toColour;
    }

    LineVertexStruct const * vertexArray() const
    {
        return m_vertexArray;
    }
};

class PointClass : public BaseClass
{
private:
    XMVECTOR m_position;
    float m_size;
    XMVECTOR m_colour;
    
    PointVertexStruct m_vertex;

public:
    PointClass( )
        : BaseClass( true, 0.0f )
        , m_position( XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ) )
        , m_size( 1.0f )
        , m_colour( XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f ) )
    {
        m_vertex.x = XMVectorGetX( m_position );
        m_vertex.y = XMVectorGetY( m_position );
        m_vertex.z = XMVectorGetZ( m_position );
        m_vertex.size = m_size;
        m_vertex.r = XMVectorGetX( m_colour );
        m_vertex.g = XMVectorGetY( m_colour );
        m_vertex.b = XMVectorGetZ( m_colour );
        m_vertex.a = XMVectorGetW( m_colour );
        //m_vertex.colour = D3DCOLOR_COLORVALUE( m_colour.x, m_colour.y, m_colour.z, m_colour.w );
    }

    PointClass(
        bool const infiniteLife,
        float const lifeTime,
        XMVECTOR const & position,
        float const size,
        XMVECTOR const & colour )
        : BaseClass( infiniteLife, lifeTime )
        , m_position( position )
        , m_size( size )
        , m_colour( colour )
    {
        m_vertex.x = XMVectorGetX( position );
        m_vertex.y = XMVectorGetY( position );
        m_vertex.z = XMVectorGetZ( position );
        m_vertex.size = size;
        m_vertex.r = XMVectorGetX( colour );
        m_vertex.g = XMVectorGetY( colour );
        m_vertex.b = XMVectorGetZ( colour );
        m_vertex.a = XMVectorGetW( colour );
        //m_vertex.colour = D3DCOLOR_COLORVALUE( colour.x, colour.y, colour.z, colour.w );
    }

    XMVECTOR const & position() const
    {
        return m_position;
    }

    float size() const
    {
        return m_size;
    }

    XMVECTOR const & colour() const
    {
        return m_colour;
    }

    PointVertexStruct const & vertex() const
    {
        return m_vertex;
    }
};

typedef std::map< int, LineClass > LineMapClass;
LineMapClass g_lineMap;

typedef std::map< int, PointClass > PointMapClass;
PointMapClass g_pointMap;

void line(
    const int UID,
    XMVECTOR const & fromPosition,
    XMVECTOR const & toPosition,
    XMVECTOR const & fromColour,
    XMVECTOR const & toColour,
    bool const infiniteLife,
    float const lifeTime )
{
    g_lineMap[ UID ] = LineClass( 
        infiniteLife,
        lifeTime,
        fromPosition,
        toPosition,
        fromColour,
        toColour );
}

void removeLine( const int UID )
{
    g_lineMap.erase( UID );
}

void point(
    const int UID,
    XMVECTOR const & position,
    float const size,
    XMVECTOR const & colour,
    bool const infiniteLife,
    float const lifeTime )
{
    g_pointMap[ UID ] = PointClass( infiniteLife, lifeTime, position, size, colour );
}

void removePoint( const int UID )
{
    g_pointMap.erase( UID );
}

void step( float const deltaTime )
{
    LineMapClass::iterator lineItr = g_lineMap.begin();
    while( lineItr != g_lineMap.end() )
    {
        LineClass & line = lineItr->second;

        line.step( deltaTime );
        if( !line.infiniteLife( ) && ( line.lifeTime() <= 0.0f ) )
        {
            g_lineMap.erase( lineItr++ );
        }
        else
        {
            ++lineItr;
        }
    }

    PointMapClass::iterator pointItr = g_pointMap.begin();
    while( pointItr != g_pointMap.end() )
    {
        PointClass & point = pointItr->second;

        point.step( deltaTime );
        if( !point.infiniteLife( ) && ( point.lifeTime() <= 0.0f ) )
        {
            g_pointMap.erase( pointItr++ );
        }
        else
        {
            ++pointItr;
        }
    }
}

void draw( /*IDirect3DDevice9 & d3dDevice*/ )
{
    //// Lighting
    //{
    //    d3dDevice.SetRenderState( D3DRS_LIGHTING, FALSE );
    //        
    //    //d3dDevice.SetRenderState( D3DRS_AMBIENT, D3DCOLOR_RGBA( 0, 0, 0, 255 ) );
    //    //    
    //    //D3DMATERIAL9 material;
    //    //// Set the RGBA for diffuse reflection.
    //    //material.Diffuse.r = 0.0f;
    //    //material.Diffuse.g = 0.0f;
    //    //material.Diffuse.b = 0.0f;
    //    //material.Diffuse.a = 1.0f;
    //    //// Set the RGBA for ambient reflection.
    //    //material.Ambient.r = 0.0f;
    //    //material.Ambient.g = 0.0f;
    //    //material.Ambient.b = 0.0f;
    //    //material.Ambient.a = 1.0f;
    //    //// Set the color and sharpness of specular highlights.
    //    //material.Specular.r = 0.0f;
    //    //material.Specular.g = 0.0f;
    //    //material.Specular.b = 0.0f;
    //    //material.Specular.a = 0.0f;
    //    //material.Power = 50.0f;
    //    //// Set the RGBA for emissive color.
    //    //material.Emissive.r = 0.0f;
    //    //material.Emissive.g = 0.0f;
    //    //material.Emissive.b = 0.0f;
    //    //material.Emissive.a = 0.0f;
    //    //d3dDevice.SetMaterial( &material );

    //    //D3DLIGHT9 light;
    //    ////light.Type = D3DLIGHT_POINT;
    //    ////light.Type = D3DLIGHT_SPOT;
    //    //light.Type = D3DLIGHT_DIRECTIONAL;
    //    //light.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
    //    //light.Specular = { 1.0f, 1.0f, 1.0f, 1.0f };
    //    //light.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
    //    //light.Position = { 0.0f, 0.0f, 0.0f };
    //    //light.Direction = { 1.0f, -1.0f, 0.0f };
    //    //light.Range = 100.0f;
    //    //light.Falloff = 50.0f;
    //    //light.Attenuation0 = 1.0f;
    //    //light.Attenuation1 = 1.0f;
    //    //light.Attenuation2 = 1.0f;
    //    //light.Theta = 0.0f;
    //    //light.Phi = 0.0f;
    //    //d3dDevice.SetLight( 0, &light );
    //    //d3dDevice.LightEnable(0, TRUE);
    //}

    //D3DXMATRIXA16 matWorld;
    //D3DXMatrixIdentity( &matWorld );
    //d3dDevice.SetTransform( D3DTS_WORLD, &matWorld );

    //d3dDevice.SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );

    //d3dDevice.SetFVF( D3DFVF_LINEVERTEX );
    //for( LineMapClass::const_iterator lineItr = g_lineMap.begin(); lineItr != g_lineMap.end(); ++lineItr )
    //{
    //    LineClass const & line = lineItr->second;

    //    d3dDevice.DrawPrimitiveUP(
    //        D3DPT_LINELIST, // d3dprimitivetype primitivetype,
    //        1, // uint primitivecount,
    //        line.vertexArray(), // const void* pvertexstreamzerodata,
    //        sizeof( LineVertexStruct ) ); // uint vertexstreamzerostride
    //}

    //d3dDevice.SetFVF( D3DFVF_POINTVERTEX );
    //for( PointMapClass::const_iterator pointItr = g_pointMap.begin(); pointItr != g_pointMap.end(); ++pointItr )
    //{   
    //    PointClass const & point = pointItr->second;

    //    d3dDevice.DrawPrimitiveUP(
    //        D3DPT_POINTLIST, // d3dprimitivetype primitivetype,
    //        1, // uint primitivecount,
    //        &( point.vertex() ), // const void* pvertexstreamzerodata,
    //        sizeof( PointVertexStruct ) ); // uint vertexstreamzerostride
    //}
}

}
