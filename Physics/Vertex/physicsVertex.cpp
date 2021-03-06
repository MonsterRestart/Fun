// Andrew Davies

#include "pch.h"
#include "Physics/Vertex/physicsVertex.h"

//using namespace std;
using namespace DirectX;

namespace Physics
{

Vertex::Vertex( 
    const XMVECTOR& position,
    const XMVECTOR& normal )
    //const D3DCOLORVALUE& colour,
    //const float tu,
    //const float tv )
    : m_position( position )
    , m_normal( normal )
    //, m_colour( colour )
    //, m_tu( tu )
    //, m_tv( tv )
{
}

void Vertex::setNormal( const XMVECTOR& normal )
{
    m_normal = normal;
}

void Vertex::addEdgeIndex( const int edgeIndex )
{
    assert( !m_edgeIndices.full() );
    m_edgeIndices.push_back( edgeIndex );
}

void Vertex::addTriangleIndex( const int triangleIndex )
{
    assert( !m_triangleIndices.full() );
    m_triangleIndices.push_back( triangleIndex );
}

} // namespace Physics

