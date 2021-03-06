// Andrew Davies

#include "pch.h"
#include "Physics/Triangle/physicsTriangle.h"

//using namespace std;

namespace Physics
{

Triangle::Triangle( const DirectX::XMVECTOR& normal )
    : m_normal( normal )
{
}

void Triangle::addVertexIndex( const int vertexIndex )
{
    assert( !m_vertexIndices.full() );
    m_vertexIndices.push_back( vertexIndex );
}

void Triangle::addEdgeIndex( const int edgeIndex )
{
    assert( !m_edgeIndices.full() );
    m_edgeIndices.push_back( edgeIndex );
}

} // namespace Physics

