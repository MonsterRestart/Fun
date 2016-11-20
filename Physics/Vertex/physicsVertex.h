#if !defined( PHYSICS_VERTEX_H )
#define PHYSICS_VERTEX_H

// Andrew Davies

#include "Physics/Vertex/physicsVertexFwd.h"
#include "Dav/container/container.hpp"
#include <DirectXMath.h>

namespace Physics
{

class Vertex
{
public:
    typedef Dav::Vector< int, 8 > Indices;

private:
    DirectX::XMVECTOR m_position;
    DirectX::XMVECTOR m_normal;
    //D3DCOLORVALUE m_colour;
    //float m_tu, m_tv;

    Indices m_edgeIndices;
    Indices m_triangleIndices;

public:
    Vertex( 
        const DirectX::XMVECTOR& position,
        const DirectX::XMVECTOR& normal );
        //const D3DCOLORVALUE& colour,
        //float tu, float tv );
    
    const DirectX::XMVECTOR& position() const
    {
        return m_position;
    }

    const DirectX::XMVECTOR& normal() const
    {
        return m_normal;
    }

    void setNormal( const DirectX::XMVECTOR& normal );
    
    //const D3DCOLORVALUE& colour() const
    //{
    //    return m_colour;
    //}

    //float tu() const
    //{
    //    return m_tu;
    //}
    //
    //float tv() const
    //{
    //    return m_tv;
    //}

    const Indices& edgeIndices() const
    {
        return m_edgeIndices;
    }

    void addEdgeIndex( int edgeIndex );

    const Indices& triangleIndices() const
    {
        return m_triangleIndices;
    }

    void addTriangleIndex( int triangleIndex );

};

}

#endif