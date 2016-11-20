#if !defined( PHYSICS_EDGE_H )
#define PHYSICS_EDGE_H

// Andrew Davies

#include "Physics/Edge/physicsEdgeFwd.h"
#include "Dav/container/container.hpp"
#include <DirectXMath.h>

namespace Physics
{

class Edge
{
public:
    typedef Dav::Vector< int, 2 > Indices;

private:
    DirectX::XMVECTOR m_normal;

    Indices m_vertexIndices;
    Indices m_triangleIndices;

public:
    Edge( const DirectX::XMVECTOR& normal );

    const DirectX::XMVECTOR& normal() const
    {
        return m_normal;
    }

    void setNormal( const DirectX::XMVECTOR& normal );
    
    const Indices& vertexIndices() const
    {
        return m_vertexIndices;
    }

    void addVertexIndex( int vertexIndex );

    const Indices& triangleIndices() const
    {
        return m_triangleIndices;
    }

    void addTriangleIndex( int triangleIndex );

};

}

#endif