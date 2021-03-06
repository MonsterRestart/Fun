// Andrew Davies

#include "pch.h"
#include "Physics/Edge/physicsEdge.h"

//using namespace std;
using namespace DirectX;

namespace Physics
{

Edge::Edge( const XMVECTOR& normal )
    : m_normal( normal )
{
}

void Edge::setNormal( const XMVECTOR& normal )
{
    m_normal = normal;
}

void Edge::addVertexIndex( const int vertexIndex )
{
    assert( !m_vertexIndices.full() );
    m_vertexIndices.push_back( vertexIndex );
}

void Edge::addTriangleIndex( const int triangleIndex )
{
    assert( !m_triangleIndices.full() );
    m_triangleIndices.push_back( triangleIndex );
}

} // namespace Physics

