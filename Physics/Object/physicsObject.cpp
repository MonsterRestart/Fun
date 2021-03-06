// Andrew Davies

#include "pch.h"
#include "Physics/Object/physicsObject.h"
#include "Physics/Feature/physicsFeature.h"
#include "Misc/misc.h"
#include <float.h>

//using namespace std;
using namespace DirectX;

#define PHYSICS_OBJECT_DEBUG_NORMAL_LENGTH ( 50.0f )

namespace Physics
{

Object::Object()
    : m_UID( 0 )
    //, m_averageVertexPosition( 0.0f, 0.0f, 0.0f, 1.0f )
    //, m_area( 0.0f )
    , m_mass( 0.0f )
    , m_infiniteMass( false )
    , m_centerOfMassLocalPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_inertia{ 1.0f, 1.0f, 1.0f, 0.0f }
    , m_dynamics()
    , m_force{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_torque{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_debugVertexData()
    , m_debugVertexIndices()
    , m_debugVertexBuffer()
    , m_debugVertexIndexBuffer()
    , m_debugEdgeVertexData()
    , m_debugEdgeIndices()
    , m_debugEdgeVertexBuffer()
    , m_debugEdgeIndexBuffer()
    , m_debugTriangleVertexData()
    , m_debugTriangleIndices()
    , m_debugTriangleVertexBuffer()
    , m_debugTriangleIndexBuffer()
{
}

void Object::create(
    const int UID,
    const float mass,
    const bool infiniteMass,
    const XMVECTOR& centerOfMassLocalPosition,
    const XMVECTOR& inertia,
    const Dynamics& dynamics,
    const XMVECTOR& force,
    const XMVECTOR& torque,
    const Vertices& vertices,
    const Edges& edges,
    const Triangles& triangles )
{
    assert( m_vertices.empty() );
    assert( m_edges.empty() );
    assert( m_triangles.empty() );

    m_UID = UID;
    m_mass = mass;
    m_infiniteMass = infiniteMass;
    m_centerOfMassLocalPosition = centerOfMassLocalPosition;
    m_inertia = inertia;
    m_dynamics = dynamics;
    m_force = force;
    m_torque = torque;
    m_vertices = vertices;
    m_edges = edges;
    m_triangles = triangles;

    // Debug Vertices
    {
        assert( m_debugVertexData.empty() );
        assert( m_debugVertexIndices.empty() );

        for( const Vertex& vertex : vertices )
        {
            m_debugVertexData.push_back( Math::Vector4( vertex.position() ) );
            m_debugVertexData.push_back( Math::Vector4( vertex.position() + vertex.normal() * PHYSICS_OBJECT_DEBUG_NORMAL_LENGTH ) );
            m_debugVertexIndices.push_back( ( uint16_t )( m_debugVertexData.size() - 2 ) );
            m_debugVertexIndices.push_back( ( uint16_t )( m_debugVertexData.size() - 1 ) );
        }

        m_debugVertexBuffer.Create( L"DebugVertexBuffer", ( uint32_t )m_debugVertexData.size(), sizeof( Math::Vector4 ), &m_debugVertexData[ 0 ] );
        m_debugVertexIndexBuffer.Create( L"DebugVertexIndexBuffer", ( uint32_t )m_debugVertexIndices.size(), sizeof( uint16_t ), &m_debugVertexIndices[ 0 ] );
    }

    // Debug Edges
    {
        assert( m_debugEdgeVertexData.empty() );
        assert( m_debugEdgeIndices.empty() );

        for( const Edge& edge : edges )
        {
            assert( edge.vertexIndices().size() == 2 );
            const int fromVertexIndex = edge.vertexIndices()[ 0 ];
            const int toVertexIndex = edge.vertexIndices()[ 1 ];
            assert( fromVertexIndex < vertices.size() );
            assert( toVertexIndex < vertices.size() );
            
            const Math::Vector4 from = Math::Vector4( vertices[ fromVertexIndex ].position() );
            const Math::Vector4 to = Math::Vector4( vertices[ toVertexIndex ].position() );

            m_debugEdgeVertexData.push_back( from );
            m_debugEdgeVertexData.push_back( to );
            m_debugEdgeIndices.push_back( ( uint16_t )( m_debugEdgeVertexData.size() - 2 ) );
            m_debugEdgeIndices.push_back( ( uint16_t )( m_debugEdgeVertexData.size() - 1 ) );

            const Math::Vector4 normStart = from + ( ( to - from ) / 2.0f );
            const Math::Vector4 normEnd = normStart + ( Math::Vector4( edge.normal() ) * PHYSICS_OBJECT_DEBUG_NORMAL_LENGTH );

            m_debugEdgeVertexData.push_back( normStart );
            m_debugEdgeVertexData.push_back( normEnd );
            m_debugEdgeIndices.push_back( ( uint16_t )( m_debugEdgeVertexData.size() - 2 ) );
            m_debugEdgeIndices.push_back( ( uint16_t )( m_debugEdgeVertexData.size() - 1 ) );
        }

        m_debugEdgeVertexBuffer.Create( L"DebugEdgeVertexBuffer", ( uint32_t )m_debugEdgeVertexData.size(), sizeof( Math::Vector4 ), &m_debugEdgeVertexData[ 0 ] );
        m_debugEdgeIndexBuffer.Create( L"DebugEdgeVertexIndexBuffer", ( uint32_t )m_debugEdgeIndices.size(), sizeof( uint16_t ), &m_debugEdgeIndices[ 0 ] );
    }

    // Debug Triangles
    {
        assert( m_debugTriangleVertexData.empty() );
        assert( m_debugTriangleIndices.empty() );

        for( const Triangle& triangle : triangles )
        {
            assert( triangle.vertexIndices().size() == 3 );
            const int vertexIndex0 = triangle.vertexIndices()[ 0 ];
            const int vertexIndex1 = triangle.vertexIndices()[ 1 ];
            const int vertexIndex2 = triangle.vertexIndices()[ 2 ];
            assert( vertexIndex0 < vertices.size() );
            assert( vertexIndex1 < vertices.size() );
            assert( vertexIndex2 < vertices.size() );

            const Math::Vector4 pos0 = Math::Vector4( vertices[ vertexIndex0 ].position() );
            const Math::Vector4 pos1 = Math::Vector4( vertices[ vertexIndex1 ].position() );
            const Math::Vector4 pos2 = Math::Vector4( vertices[ vertexIndex2 ].position() );
            
            m_debugTriangleVertexData.push_back( pos0 );
            m_debugTriangleVertexData.push_back( pos1 );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 2 ) );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 1 ) );

            m_debugTriangleVertexData.push_back( pos1 );
            m_debugTriangleVertexData.push_back( pos2 );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 2 ) );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 1 ) );

            m_debugTriangleVertexData.push_back( pos2 );
            m_debugTriangleVertexData.push_back( pos0 );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 2 ) );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 1 ) );

            const Math::Vector4 normStart = ( pos0 + pos1 + pos2 ) / 3.0f;
            const Math::Vector4 normEnd = normStart + ( Math::Vector4( triangle.normal() ) * PHYSICS_OBJECT_DEBUG_NORMAL_LENGTH );

            m_debugTriangleVertexData.push_back( normStart );
            m_debugTriangleVertexData.push_back( normEnd );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 2 ) );
            m_debugTriangleIndices.push_back( ( uint16_t )( m_debugTriangleVertexData.size() - 1 ) );
        }

        m_debugTriangleVertexBuffer.Create( L"DebugTriangleVertexBuffer", ( uint32_t )m_debugTriangleVertexData.size(), sizeof( Math::Vector4 ), &m_debugTriangleVertexData[ 0 ] );
        m_debugTriangleIndexBuffer.Create( L"DebugTriangleVertexIndexBuffer", ( uint32_t )m_debugTriangleIndices.size(), sizeof( uint16_t ), &m_debugTriangleIndices[ 0 ] );
    }
    


    //if( SUCCEEDED( vhr ) && m_d3d9VertexBufferPtr &&
    //    SUCCEEDED( ihr ) && m_d3d9IndexBufferPtr &&
    //    SUCCEEDED( thr ) && m_d3d9TexturePtr )
    //{
    //    D3D9Vertex* d3d9Vertices = nullptr;
    //    m_d3d9VertexBufferPtr->Lock(0, 0, (void**)&d3d9Vertices, 0);

    //    for( int vertexIndex = 0; vertexIndex != numVertices; ++vertexIndex )
    //    {
    //        d3d9Vertices[ vertexIndex ].x = vertices[ vertexIndex ].position().x;
    //        d3d9Vertices[ vertexIndex ].y = vertices[ vertexIndex ].position().y;
    //        d3d9Vertices[ vertexIndex ].z = vertices[ vertexIndex ].position().z;
    //        //d3d9Vertices[ vertexIndex ].w = 0.0f; // vertices[ vertexIndex ].position().w;
    //        d3d9Vertices[ vertexIndex ].n.x = vertices[ vertexIndex ].normal().x;
    //        d3d9Vertices[ vertexIndex ].n.y = vertices[ vertexIndex ].normal().y;
    //        d3d9Vertices[ vertexIndex ].n.z = vertices[ vertexIndex ].normal().z;
    //        d3d9Vertices[ vertexIndex ].colour = D3DCOLOR_COLORVALUE(
    //            vertices[ vertexIndex ].colour().r,
    //            vertices[ vertexIndex ].colour().g,
    //            vertices[ vertexIndex ].colour().b,
    //            vertices[ vertexIndex ].colour().a );
    //        d3d9Vertices[ vertexIndex ].tu = vertices[ vertexIndex ].tu();
    //        d3d9Vertices[ vertexIndex ].tv = vertices[ vertexIndex ].tv();

    //    }

    //    m_d3d9VertexBufferPtr->Unlock();

    //    D3D9Index* d3d9Indices = nullptr;
    //    m_d3d9IndexBufferPtr->Lock(0, 0, (void**)&d3d9Indices, 0);

    //    for( int triangleIndex = 0; triangleIndex != numTriangles; ++triangleIndex )
    //    {
    //        for( int triangleVertexIndex = 0; triangleVertexIndex != 3; ++triangleVertexIndex )
    //        {
    //            const int indexIndex = ( triangleIndex * 3 ) + triangleVertexIndex;
    //            assert( indexIndex < numIndices );
    //            assert( triangleVertexIndex < ( int )triangles[ triangleIndex ].vertexIndices().size() );

    //            d3d9Indices[ indexIndex ] = ( D3D9Index )triangles[ triangleIndex ].vertexIndices()[ triangleVertexIndex ];
    //        }
    //    }

    //    m_d3d9IndexBufferPtr->Unlock();
    //}

//	//const int numberOfVertices = ( int )vertexPositionVector.size();
//	//for( int vertexPositionIndex = 0; vertexPositionIndex != numberOfVertices; ++vertexPositionIndex )
//	//{
//	//	int nextVertexPositionIndex = vertexPositionIndex + 1;
//	//	if( nextVertexPositionIndex == numberOfVertices )
//	//	{
//	//		nextVertexPositionIndex = 0;
//	//	}
//
//	//	//  http://local.wasp.uwa.edu.au/~pbourke/geometry/polyarea/
//	//	float const temp = 
//	//		( vertexPositionVector[ vertexPositionIndex ].x * vertexPositionVector[ nextVertexPositionIndex ].y ) -
//	//		( vertexPositionVector[ nextVertexPositionIndex ].x * vertexPositionVector[ vertexPositionIndex ].y );
//
//	//	m_area += temp;
//	//	m_centerOfMassLocalPosition.x += ( vertexPositionVector[ vertexPositionIndex ].x + vertexPositionVector[ nextVertexPositionIndex ].x ) * temp;
//	//	m_centerOfMassLocalPosition.y += ( vertexPositionVector[ vertexPositionIndex ].y + vertexPositionVector[ nextVertexPositionIndex ].y ) * temp;
//
//	//	m_averageVertexPosition += vertexPositionVector[ vertexPositionIndex ];
//	//}
//	//if( numberOfVertices > 0 )
//	//{
//	//	m_averageVertexPosition /= ( float )numberOfVertices;
//	//}
//	//m_area /= 2.0f;
//
//	//if( fabsf( m_area ) > 0.0f )
//	//{
//	//	m_centerOfMassLocalPosition *= ( 1.0f / ( 6.0f * m_area ) );
//	//}
//	//else
//	//{
//	//	m_centerOfMassLocalPosition.x = 0.0f;
//	//	m_centerOfMassLocalPosition.y = 0.0f;
//	//}
//	//m_centerOfMassLocalPosition.w = 1.0f;
//
//	//// if area is positive at this point then the vertices are wound anti-clockwise and need to be flipped.
//	//if( m_area > 0.0f )
//	//{
//	//	D3DXVECTOR4VectorType::const_reverse_iterator const beginInputVertexPositionReverseItr = vertexPositionVector.rbegin();
//	//	D3DXVECTOR4VectorType::const_reverse_iterator const endInputVertexPositionReverseItr = vertexPositionVector.rend();
//	//	D3DXVECTOR4VectorType::const_reverse_iterator inputVertexPositionReverseItr = beginInputVertexPositionReverseItr;
//	//	
//	//	D3DXVECTOR4VectorType::iterator const beginVertexPositionItr = m_vertexPositionVector.begin();
//	//	D3DXVECTOR4VectorType::iterator const endVertexPositionItr = m_vertexPositionVector.end();
//	//	D3DXVECTOR4VectorType::iterator vertexPositionItr = beginVertexPositionItr;
//	//	
//	//	for( ; vertexPositionItr != endVertexPositionItr; ++vertexPositionItr, ++inputVertexPositionReverseItr )
//	//	{
//	//		*vertexPositionItr = *inputVertexPositionReverseItr;
//	//	}
//	//}
//	//else
//	//{		
//	//	m_area = -m_area;
//	//}
//
//	//m_mass = ( infiniteMass ? ( FLT_MAX ) : m_area );
//
//	//for( int vertexPositionIndex = 0; vertexPositionIndex != numberOfVertices; ++vertexPositionIndex )
//	//{
//	//	int nextVertexPositionIndex = vertexPositionIndex + 1;
//	//	if( nextVertexPositionIndex == numberOfVertices )
//	//	{
//	//		nextVertexPositionIndex = 0;
//	//	}
//	//
//	//	XMVECTOR const edgeDirection = m_vertexPositionVector[ nextVertexPositionIndex ] - m_vertexPositionVector[ vertexPositionIndex ];
//	//	float const edgeLength = D3DXVec4Length( &edgeDirection );
//	//	XMVECTOR edgeDirectionUnit = edgeDirection;
//	//	D3DXVec4Normalize( &edgeDirectionUnit, &edgeDirectionUnit );
//	//	XMVECTOR edgeNormalDirectionUnit( -edgeDirection.y, edgeDirection.x, edgeDirection.z, edgeDirection.w );
//	//	D3DXVec4Normalize( &edgeNormalDirectionUnit, &edgeNormalDirectionUnit );
//
//	//	m_edgeDirectionVector[ vertexPositionIndex ] = edgeDirection;
//	//	m_edgeDirectionUnitVector[ vertexPositionIndex ] = edgeDirectionUnit;
//	//	m_edgeNormalDirectionUnitVector[ vertexPositionIndex ] = edgeNormalDirectionUnit;
//	//	m_edgeLengthVector.push_back( edgeLength );
//	//}
//
//	//// Now we have center of mass we can translate vertices to calculate moment of inertia	
//	//float momentOfInertiaDenominator = 0.0f;
//	//float momentOfInertiaDevisor = 0.0f;
//	//for( int vertexPositionIndex = 0; vertexPositionIndex != numberOfVertices; ++vertexPositionIndex )
//	//{
//	//	int nextVertexPositionIndex = vertexPositionIndex + 1;
//	//	if( nextVertexPositionIndex == numberOfVertices )
//	//	{
//	//		nextVertexPositionIndex = 0;
//	//	}
//
//	//	XMVECTOR const vertexPosition = vertexPositionVector[ vertexPositionIndex ] - m_centerOfMassLocalPosition;
//	//	XMVECTOR const nextVertexPosition = vertexPositionVector[ nextVertexPositionIndex ] - m_centerOfMassLocalPosition;
//	//	
//	//	XMVECTOR nextVertexPositionCrossVertexPosition;
//	//	D3DXVec4Cross( &nextVertexPositionCrossVertexPosition, &nextVertexPosition, &vertexPosition, &Misc::origin() );
//	//	float const nextVertexPositionCrossVertexPositionLength = D3DXVec4Length( &nextVertexPositionCrossVertexPosition );
//	//	float const vertexPositionLengthSquared = D3DXVec4LengthSq( &vertexPosition );
//	//	float const nextVertexPositionLengthSquared = D3DXVec4LengthSq( &nextVertexPosition );
//	//	float const nextVertexPositionDotVertexPosition = D3DXVec4Dot( &nextVertexPosition, &vertexPosition );
//
//	//	momentOfInertiaDenominator += nextVertexPositionCrossVertexPositionLength * ( nextVertexPositionLengthSquared + nextVertexPositionDotVertexPosition + vertexPositionLengthSquared );
//	//	momentOfInertiaDevisor += nextVertexPositionCrossVertexPositionLength;
//	//}
//
//	//if( fabsf( momentOfInertiaDevisor ) > 0.0f )
//	//{
//	//	m_momentOfInertia = ( m_mass / 6.0f ) * ( momentOfInertiaDenominator / momentOfInertiaDevisor );
//	//}
//	//
//	//if( infiniteMass )
//	//{	
//	//	m_momentOfInertia = FLT_MAX;
//	//}
//
//	m_dynamics.set( position, orientation, velocity, angularVelocity, m_centerOfMassLocalPosition );

}

void Object::destroy()
{
}

void Object::drawDebugVertices(
    GraphicsContext& gfxContext, 
    const Math::Matrix4& ViewProjMat ) const
{
    struct VSConstants
    {
        Math::Matrix4 modelToProjection;
    } vsConstants;
    
    const Math::Matrix4 transformation = Math::Matrix4( m_dynamics.transformation() );
    vsConstants.modelToProjection = ViewProjMat * transformation;

    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);
    
    gfxContext.SetIndexBuffer(m_debugVertexIndexBuffer.IndexBufferView());
    gfxContext.SetVertexBuffer(0, m_debugVertexBuffer.VertexBufferView());
    uint32_t VertexStride = sizeof( float ) * 3;    
    uint32_t indexCount = ( uint32_t )m_debugVertexIndices.size();
    uint32_t startIndex = 0;
    uint32_t baseVertex = 0;

    gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);
}

void Object::drawDebugEdges(
    GraphicsContext& gfxContext, 
    const Math::Matrix4& ViewProjMat ) const
{
    struct VSConstants
    {
        Math::Matrix4 modelToProjection;
    } vsConstants;
    
    const Math::Matrix4 transformation = Math::Matrix4( m_dynamics.transformation() );
    vsConstants.modelToProjection = ViewProjMat * transformation;

    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);
    
    gfxContext.SetIndexBuffer(m_debugEdgeIndexBuffer.IndexBufferView());
    gfxContext.SetVertexBuffer(0, m_debugEdgeVertexBuffer.VertexBufferView());
    uint32_t VertexStride = sizeof( float ) * 3;    
    uint32_t indexCount = ( uint32_t )m_debugEdgeIndices.size();
    uint32_t startIndex = 0;
    uint32_t baseVertex = 0;

    gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);
}

void Object::drawDebugTriangles(
    GraphicsContext& gfxContext, 
    const Math::Matrix4& ViewProjMat ) const
{
    struct VSConstants
    {
        Math::Matrix4 modelToProjection;
    } vsConstants;
    
    const Math::Matrix4 transformation = Math::Matrix4( m_dynamics.transformation() );
    vsConstants.modelToProjection = ViewProjMat * transformation;

    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);
    
    gfxContext.SetIndexBuffer(m_debugTriangleIndexBuffer.IndexBufferView());
    gfxContext.SetVertexBuffer(0, m_debugTriangleVertexBuffer.VertexBufferView());
    uint32_t VertexStride = sizeof( float ) * 3;    
    uint32_t indexCount = ( uint32_t )m_debugTriangleIndices.size();
    uint32_t startIndex = 0;
    uint32_t baseVertex = 0;

    gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);
}

void Object::draw( 
    GraphicsContext& gfxContext, 
    const Math::Matrix4& ViewProjMat ) const
{
    drawDebugVertices( gfxContext, ViewProjMat );
    drawDebugEdges( gfxContext, ViewProjMat );
    drawDebugTriangles( gfxContext, ViewProjMat );
}

void Object::setDynamics(
    const XMVECTOR& position,
    const XMVECTOR& orientation,
    const XMVECTOR& velocity,
    const XMVECTOR& angularVelocity )
{
    m_dynamics.set( position, orientation, velocity, angularVelocity, m_centerOfMassLocalPosition );
}

void Object::setDynamics( const Dynamics& dynamics )
{
    m_dynamics = dynamics;
}

void Object::stepDynamics( const float deltaTime )
{
    const XMMATRIX worldTransformation = m_dynamics.transformation();

    const XMVECTOR invInertia{ 1.0f / XMVectorGetX( m_inertia ), 1.0f / XMVectorGetX( m_inertia ), 1.0f / XMVectorGetX( m_inertia ), 0.0f };

    const XMMATRIX invInertiaScale = XMMatrixScalingFromVector( invInertia );
    //D3DXMatrixScaling( &invInertiaScale, invInertia.x, invInertia.y, invInertia.z );
    
    //const XMMATRIX worldOrientation(
    XMMATRIX worldOrientation = worldTransformation;
    XMVectorSetW( worldOrientation.r[ 0 ], 0.0f );
    XMVectorSetW( worldOrientation.r[ 1 ], 0.0f );
    XMVectorSetW( worldOrientation.r[ 2 ], 0.0f );
    worldOrientation.r[ 3 ] = { 0.0f, 0.0f, 0.0f, 1.0f };
    //const XMMATRIX worldOrientation(
    //    worldTransformation._11, worldTransformation._12, worldTransformation._13, 0.0f,
    //    worldTransformation._21, worldTransformation._22, worldTransformation._23, 0.0f,
    //    worldTransformation._31, worldTransformation._32, worldTransformation._33, 0.0f,
    //    0.0f, 0.0f, 0.0f, 1.0f );
        
    const XMMATRIX worldOrientationScale = XMMatrixMultiply( worldOrientation, invInertiaScale );
    //D3DXMatrixMultiply( &worldOrientationScale, &worldOrientation, &invInertiaScale );

    const XMMATRIX worldOrientationTranspose = XMMatrixTranspose( worldOrientation );
    //D3DXMatrixTranspose( &worldOrientationTranspose, &worldOrientation );

    const XMMATRIX inverseInertiaTensorWorld = XMMatrixMultiply( worldOrientationScale, worldOrientationTranspose );
    //D3DXMatrixMultiply( &inverseInertiaTensorWorld, &worldOrientationScale, &worldOrientationTranspose );

    m_dynamics.step( m_force, m_torque, deltaTime, m_infiniteMass, m_mass, inverseInertiaTensorWorld, m_centerOfMassLocalPosition );
}

//int Object::numberOfVertices() const
//{
//	return ( int )m_vertexPositionVector.size();
//}
//
//int Object::numberOfEdges() const
//{
//	return ( int )m_edgeDirectionVector.size();
//}
//
//XMVECTOR Object::vertexPosition( const int index ) const
//{
//	assert( index < numberOfVertices() );
//	return m_vertexPositionVector[ index ];
//}
//
//XMVECTOR Object::edgeDirection( const int index ) const
//{
//	assert( index < numberOfEdges() );
//	return m_edgeDirectionVector[ index ];
//}
//
//XMVECTOR Object::edgeDirectionUnit( const int index ) const
//{
//	assert( index < numberOfEdges() );
//	return m_edgeDirectionUnitVector[ index ];
//}
//
//XMVECTOR Object::edgeNormalDirectionUnit( const int index ) const
//{
//	assert( index < numberOfEdges() );
//	return m_edgeNormalDirectionUnitVector[ index ];
//}
//
//float Object::edgeLength( const int index ) const
//{
//	assert( index < numberOfEdges() );
//	return m_edgeLengthVector[ index ];
//}

void Object::addForceActingAtWorldPosition( const XMVECTOR& worldForce, const XMVECTOR& worldPosition )
{
    m_force += worldForce;
    addTorqueActingAtWorldPosition( worldForce, worldPosition );
}

void Object::addForceActingAtLocalPosition( const XMVECTOR& localForce, const XMVECTOR& localPosition )
{
    const XMVECTOR worldForce = XMVector4Transform( localForce, m_dynamics.transformation() );
    //D3DXVec4Transform( &worldForce, &localForce, &m_dynamics.transformation() );
    const XMVECTOR worldPosition = XMVector4Transform( localPosition, m_dynamics.transformation() );
    //D3DXVec4Transform( &worldPosition, &localPosition, &m_dynamics.transformation() );
    addForceActingAtWorldPosition( worldForce, worldPosition );
}

void Object::addTorqueActingAtWorldPosition( const XMVECTOR& worldForce, const XMVECTOR& worldPosition )
{
    const XMVECTOR COMPos = centerOfMassWorldPosition();
    const XMVECTOR localPos = worldPosition - COMPos;
    
    const XMVECTOR additionalTorque = XMVector4Cross( localPos, worldForce, Misc::origin() );
    //D3DXVec4Cross( &additionalTorque, &localPos, &worldForce, &Misc::origin() );
    m_torque += additionalTorque;
}

void Object::addTorqueActingAtLocalPosition( const XMVECTOR& localForce, const XMVECTOR& localPosition )
{
    const XMVECTOR worldForce = XMVector4Transform( localForce, m_dynamics.transformation() );
    //D3DXVec4Transform( &worldForce, &localForce, &m_dynamics.transformation() );
    const XMVECTOR worldPosition = XMVector4Transform( localPosition, m_dynamics.transformation() );
    //D3DXVec4Transform( &worldPosition, &localPosition, &m_dynamics.transformation() );
    addTorqueActingAtWorldPosition( worldForce, worldPosition );
}

//void Object::addForceActingAtWorldPosition( 
//	const XMVECTOR& force,
//	const XMVECTOR& worldPos )
//{
//	m_force += force;
//	
//	XMVECTOR const COMPos = centerOfMassWorldPosition();
//	XMVECTOR const localPos = worldPos - COMPos;
//	
//	XMVECTOR const perpLocalPos( -localPos.y, localPos.x, localPos.z, localPos.w );
//
//	m_torque += ( perpLocalPos[ 0 ] * force[ 0 ] ) + ( perpLocalPos[ 1 ] * force[ 1 ] );
//}

void Object::setForce( const XMVECTOR& force )
{
    m_force = force;
}

void Object::clearForce()
{
    m_force = Misc::zero();
}

void Object::addForce( const XMVECTOR& force )
{
    m_force += force;
}

void Object::setTorque( const XMVECTOR& torque )
{
    m_torque = torque;
}

void Object::clearTorque()
{
    m_torque = Misc::zero();
}

void Object::addTorque( const XMVECTOR& torque )
{
    m_torque += torque;
}

//XMVECTOR Object::transformedAverageVertexPosition() const
//{
//    return XMVector4Transform( m_averageVertexPosition, m_dynamics.transformation() );
//}

XMVECTOR Object::centerOfMassWorldPosition() const
{
    return XMVector4Transform( m_centerOfMassLocalPosition, m_dynamics.transformation() );
}

//XMVECTOR Object::transformedVertexPosition( const int index ) const
//{
//    assert( index < numberOfVertices() );
//    return XMVector4Transform( m_vertexPositionVector[ index ], m_dynamics.transformation() );
//}
//
//XMVECTOR Object::transformedEdgeDirection( const int index ) const
//{
//    assert( index < numberOfEdges() );
//    return XMVector4Transform( m_edgeDirectionVector[ index ], m_dynamics.transformation() );
//}
//
//XMVECTOR Object::transformedEdgeDirectionUnit( const int index ) const
//{
//    assert( index < numberOfEdges() );
//    return XMVector4Transform( m_edgeDirectionUnitVector[ index ], m_dynamics.transformation() );
//    return returnValue;
//}
//
//XMVECTOR Object::transformedEdgeNormalDirectionUnit( const int index ) const
//{
//    assert( index < numberOfEdges() );
//    return XMVector4Transform( m_edgeNormalDirectionUnitVector[ index ], m_dynamics.transformation() );
//}

//float objectSignedDistanceFromPlaneToPoint(
//    XMVECTOR const & planePosition,
//    XMVECTOR const & planeUnitNormal,
//    XMVECTOR const & point )
//{
//    XMVECTOR const planePositionToPointDirection =  point - planePosition;
//    return D3DXVec4Dot( &planeUnitNormal, &planePositionToPointDirection );
//}
//
//bool objectBHasSeperatingEdge( Object const & objectA, Object const & objectB )
//{	
//	const int objectANumberOfVertices = objectA.numberOfVertices();
//	const int objectBNumberOfEdges = objectB.numberOfEdges();
//
//	int objectBEdgeIndex = 0;
//	while( objectBEdgeIndex != objectBNumberOfEdges )
//	{
//		XMVECTOR const objectBEdgePosition = objectB.transformedVertexPosition( objectBEdgeIndex );
//		XMVECTOR const objectBEdgeNormalDirectionUnit = objectB.transformedEdgeNormalDirectionUnit( objectBEdgeIndex );
//
//		int objectAVertexIndex = 0;
//		while( objectAVertexIndex != objectANumberOfVertices )
//		{		
//			XMVECTOR const objectAVertexPosition = objectA.transformedVertexPosition( objectAVertexIndex );
//			
//			float const perpDot = objectSignedDistanceFromPlaneToPoint( objectBEdgePosition, objectBEdgeNormalDirectionUnit, objectAVertexPosition );
//
//			if( perpDot > 0.0f ) 
//			{
//				++objectAVertexIndex;
//				
//				if( objectAVertexIndex == objectANumberOfVertices )
//				{
//					// found an edge that all a's vertices are on the correct side of so we must not be colliding.
//					return true;
//				}
//			}
//			else
//			{
//				// We've found a vertex that's on the wrong side of this edge.
//				// That doesn't mean we're definatley intersecting but it does mean we don't need to test this edge anymore.
//				// Move on to the next edge by breaking out of the a vertex loop
//				objectAVertexIndex = objectANumberOfVertices;
//			}			
//		}
//
//		++objectBEdgeIndex;
//	}
//	
//	return false;
//}
//
//bool objectCollision( Object const & objectA, Object const & objectB )
//{	
//	if( objectBHasSeperatingEdge( objectA, objectB ) )
//	{
//		return false;
//	}
//		
//	if( objectBHasSeperatingEdge( objectB, objectA ) )
//	{
//		return false;
//	}
//
//	return true;
//}
//
//bool objectIntersect( 
//	Object const & object, 
//	XMVECTOR const & orig, 
//	XMVECTOR const & dir,
//	float & t, float & u, float & v )
//{
//	const int numberOfVertices = object.numberOfVertices();
//	int vertexIndex = 0;
//	while( vertexIndex != numberOfVertices )
//	{
//		int nextVertexIndex = vertexIndex + 1;
//		if( nextVertexIndex == numberOfVertices )
//		{
//			nextVertexIndex = 0;
//		}		
//
//		// Find vectors for two edges sharing vert0
//		XMVECTOR const edge1 = object.transformedVertexPosition( vertexIndex ) - object.transformedAverageVertexPosition();
//		XMVECTOR const edge2 = object.transformedVertexPosition( nextVertexIndex ) - object.transformedAverageVertexPosition();
//		//XMVECTOR const edge1 = object.transformedVertexPosition( vertexIndex ) - object.centerOfMassWorldPosition();
//		//XMVECTOR const edge2 = object.transformedVertexPosition( nextVertexIndex ) - object.centerOfMassWorldPosition();
//
//		// Begin calculating determinant - also used to calculate U parameter
//		XMVECTOR pvec;
//		D3DXVec4Cross( &pvec, &dir, &edge2, &Misc::origin() );
//		//D3DXVec3Cross( ( D3DXVECTOR3 * )&pvec, ( D3DXVECTOR3 const * )&dir, ( D3DXVECTOR3 const * )&edge2 );
//
//		// If determinant is near zero, ray lies in plane of triangle
//		float det = D3DXVec4Dot( &edge1, &pvec );
//		//float det = D3DXVec3Dot( ( D3DXVECTOR3 const * )&edge1, ( D3DXVECTOR3 const * )&pvec );
//
//		XMVECTOR tvec;
//		if( det > 0.0f )
//		{
//			tvec = orig - object.transformedAverageVertexPosition();
//			//tvec = orig - object.centerOfMassWorldPosition();
//		}
//		else
//		{
//			tvec = object.transformedAverageVertexPosition() - orig;
//			//tvec = object.centerOfMassWorldPosition() - orig;
//			det = -det;
//		}
//
//		if( det > 0.0001f )
//		{
//			// Calculate U parameter and test bounds
//			u = D3DXVec4Dot( &tvec, &pvec );
//			//u = D3DXVec3Dot( ( D3DXVECTOR3 const * )&tvec, ( D3DXVECTOR3 const * )&pvec );
//
//			if( ( u >= 0.0f ) && ( u <= det ) )
//			{
//				// Prepare to test V parameter
//				XMVECTOR qvec;
//				D3DXVec4Cross( &qvec, &tvec, &edge1, &Misc::origin() );
//				//D3DXVec3Cross( ( D3DXVECTOR3 * )&qvec, ( D3DXVECTOR3 const * )&tvec, ( D3DXVECTOR3 const * )&edge1 );
//
//				// Calculate V parameter and test bounds
//				v = D3DXVec4Dot( &dir, &qvec );
//				//v = D3DXVec3Dot( ( D3DXVECTOR3 const * )&dir, ( D3DXVECTOR3 const * )&qvec );
//				if( ( v >= 0.0f ) && ( ( u + v ) <= det ) )
//				{
//					// Calculate t, scale parameters, ray intersects triangle
//					t = D3DXVec4Dot( &edge2, &qvec );
//					//t = D3DXVec3Dot( ( D3DXVECTOR3 const * )&edge2, ( D3DXVECTOR3 const * )&qvec );
//					float const fInvDet = 1.0f / det;
//					t *= fInvDet;
//					u *= fInvDet;
//					v *= fInvDet;
//
//					return true;
//				}
//			}
//		}
//
//		++vertexIndex;
//	}
//
//	return false;
//}
//
//void objectClosestFeatures(						  
//	float & smallestDistanceSquaredSoFar,
//	Object const & objectA, FeatureClass & objectAFeature, //float & objectADistanceAlongEdge,
//	Object const & objectB, FeatureClass & objectBFeature, float & objectBDistanceAlongEdge )
//{
//	const int objectANumberOfVertices = objectA.numberOfVertices();
//	for( int objectAVertexIndex = 0; objectAVertexIndex != objectANumberOfVertices; ++objectAVertexIndex )
//	{
//		XMVECTOR const objectAVertexPosition = objectA.transformedVertexPosition( objectAVertexIndex );
//		
//		const int objectBNumberOfEdges = objectB.numberOfEdges();
//		const int objectBNumberOfVertices = objectB.numberOfVertices();
//		for( int objectBEdgeIndex = 0; objectBEdgeIndex != objectBNumberOfEdges; ++objectBEdgeIndex )
//		{
//			XMVECTOR const objectBEdgePosition = objectB.transformedVertexPosition( objectBEdgeIndex );
//			XMVECTOR const objectBEdgeUnitDirection = objectB.transformedEdgeDirectionUnit( objectBEdgeIndex );
//			float const objectBEdgeLength = objectB.edgeLength( objectBEdgeIndex );
//			
//			float const positionDotLineDirection = D3DXVec4Dot( &objectAVertexPosition, &objectBEdgeUnitDirection );
//			float const lineStartPositionDotLineDirection = D3DXVec4Dot( &objectBEdgePosition, &objectBEdgeUnitDirection );
//			
//			float distanceAlongLine = positionDotLineDirection - lineStartPositionDotLineDirection;
//			FeatureClass objectFeature( EdgeFeature, objectBEdgeIndex );
//			if( distanceAlongLine <= 0.0f )
//			{
//				objectFeature = FeatureClass( VertexFeature, objectBEdgeIndex );
//				distanceAlongLine = 0.0f;
//			}
//			else if( distanceAlongLine >= objectBEdgeLength )
//			{
//				objectFeature = FeatureClass( VertexFeature, objectBEdgeIndex + 1 );
//				if( objectFeature.index() == objectBNumberOfVertices )
//				{
//					objectFeature = FeatureClass( VertexFeature, 0 );
//				}
//				distanceAlongLine = objectBEdgeLength;
//			}	
//			//return clamp( positionDotLineDirection - lineStartPositionDotLineDirection, 0.0f, lineLength );
//			//float const distanceAlongLine = closestDistanceAlongLine( objectBEdgePosition, objectBEdgeUnitDirection, objectBEdgeLength, objectAVertexPosition );
//
//			XMVECTOR const objectBClosestPosition = objectBEdgePosition + ( objectBEdgeUnitDirection * distanceAlongLine );
//			XMVECTOR const toObjectBClosestDirection = objectBClosestPosition - objectAVertexPosition;
//			float const toObjectBClosestDistanceSquared = D3DXVec4LengthSq( &toObjectBClosestDirection );
//			//float const toObjectBClosestDistanceSquared = D3DXVec4Dot( &toObjectBClosestDirection, &toObjectBClosestDirection );
//
//			if( toObjectBClosestDistanceSquared < smallestDistanceSquaredSoFar )
//			{
//				smallestDistanceSquaredSoFar = toObjectBClosestDistanceSquared;
//				objectAFeature = FeatureClass( VertexFeature, objectAVertexIndex );
//				objectBFeature = objectFeature; // FeatureClass( EdgeFeature, objectBEdgeIndex );
//				objectBDistanceAlongEdge = distanceAlongLine;
//			}
//		}		
//	}
//}
//
//void objectClosestFeatures(				  
//	float & smallestDistanceSquaredSoFar,
//	Object const & objectA, FeatureClass & objectAFeature, float & objectADistanceAlongEdge,
//	Object const & objectB, FeatureClass & objectBFeature, float & objectBDistanceAlongEdge )
//{
//	smallestDistanceSquaredSoFar = FLT_MAX;
//	objectAFeature = FeatureClass();
//	objectBFeature = FeatureClass();
//	
//	objectClosestFeatures( smallestDistanceSquaredSoFar, objectA, objectAFeature, objectB, objectBFeature, objectBDistanceAlongEdge );
//	objectClosestFeatures( smallestDistanceSquaredSoFar, objectB, objectBFeature, objectA, objectAFeature, objectADistanceAlongEdge );
//}
//
//XMVECTOR objectVelocityAtPoint( Object const & object, XMVECTOR const & pointPos )
//{
//	XMVECTOR const COMPos = object.centerOfMassWorldPosition();
//	XMVECTOR const radiusDir = pointPos - COMPos;
//	
//	XMVECTOR const pointVelDir( -radiusDir.y, radiusDir.x, radiusDir.z, radiusDir.w );
//
//	return object.dynamics().velocity() + ( pointVelDir * object.dynamics().angularVelocity() );
//}
//
//XMVECTOR objectVertexVelocity( Object const & object, int const vertIndex )
//{
//	assert( ( vertIndex >= 0 ) && ( vertIndex < object.numberOfVertices() ) );
//	XMVECTOR const vertPos = object.transformedVertexPosition( vertIndex );
//
//	return objectVelocityAtPoint( object, vertPos );
//}

} // namespace Physics
