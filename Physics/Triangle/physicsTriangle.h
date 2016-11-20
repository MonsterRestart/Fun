#if !defined( PHYSICS_TRIANGLE_H )
#define PHYSICS_TRIANGLE_H

// Andrew Davies

#include "Physics/Triangle/physicsTriangleFwd.h"
#include "Dav/container/container.hpp"
#include <DirectXMath.h>

namespace Physics
{

class Triangle
{
public:
    typedef Dav::Vector< int, 3 > Indices;

private:
    DirectX::XMVECTOR m_normal;

    Indices m_vertexIndices;
    Indices m_edgeIndices;

public:
    Triangle( const DirectX::XMVECTOR& normal );

    const DirectX::XMVECTOR& normal() const
    {
        return m_normal;
    }

    const Indices& vertexIndices() const
    {
        return m_vertexIndices;
    }

    void addVertexIndex( int vertexIndex );

    const Indices& edgeIndices() const
    {
        return m_edgeIndices;
    }

    void addEdgeIndex( int edgeIndex );

};

}

#endif