// Andrew Davies

#if !defined( PHYSICS_OBJECT_H )
#define PHYSICS_OBJECT_H

#include "Physics/Object/physicsObjectFwd.h"
#include "Physics/Feature/physicsFeatureFwd.h"
#include "Dav/container/container.hpp"
#include <DirectXMath.h>
#include "Physics/Dynamics/physicsDynamics.h"
#include "Physics/Vertex/physicsVertex.h"
#include "Physics/Edge/physicsEdge.h"
#include "Physics/Triangle/physicsTriangle.h"
#include "Math/Matrix4.h"
#include "Math/Vector.h"
#include "CommandContext.h" // class GraphicsContext;

namespace Physics
{

class Object
{
public:
    enum { MAX_VERTICES = 32 };
    enum { MAX_EDGES = 32 };
    enum { MAX_TRIANGLES = 32 };
    enum { MAX_INDICES = ( MAX_TRIANGLES * 3 ) };

    typedef Dav::Vector< Vertex, MAX_VERTICES > Vertices;
    typedef Dav::Vector< Edge, MAX_EDGES > Edges;
    typedef Dav::Vector< Triangle, MAX_TRIANGLES > Triangles;

    // Debug Vertices
    typedef Dav::Vector< Math::Vector4, MAX_VERTICES > DebugVertices;
    typedef Dav::Vector< uint16_t, MAX_VERTICES > DebugVertexIndicies;

private:
    int m_UID;

    //XMVECTORVectorType m_vertexPositionVector;  // Made clockwise in constructor.
    //XMVECTOR m_averageVertexPosition;

    //XMVECTORVectorType m_edgeDirectionVector;
    //XMVECTORVectorType m_edgeDirectionUnitVector;
    //XMVECTORVectorType m_edgeNormalDirectionUnitVector;
    //FloatVectorType m_edgeLengthVector;

    //float m_area;
    float m_mass;
    bool m_infiniteMass;

    DirectX::XMVECTOR m_centerOfMassLocalPosition;

    DirectX::XMVECTOR m_inertia;
    //float m_momentOfInertia;

    Dynamics m_dynamics;

    DirectX::XMVECTOR m_force;
    DirectX::XMVECTOR m_torque;

    Vertices m_vertices;
    Edges m_edges;
    Triangles m_triangles;

    // Debug Vertices
    DebugVertices m_debugVertexData;
    DebugVertexIndicies m_debugVertexIndicies;
    StructuredBuffer m_debugVertexBuffer;
    ByteAddressBuffer m_debugVertexIndexBuffer;

    // Debug Edges
    DebugVertices m_debugEdgeVertexData;
    DebugVertexIndicies m_debugEdgeIndicies;
    StructuredBuffer m_debugEdgeVertexBuffer;
    ByteAddressBuffer m_debugEdgeIndexBuffer;

    // Debug Triangles
    DebugVertices m_debugTriangleVertexData;
    DebugVertexIndicies m_debugTriangleIndicies;
    StructuredBuffer m_debugTriangleVertexBuffer;
    ByteAddressBuffer m_debugTriangleIndexBuffer;

public:
    Object();

    void create(
        int UID,
        float mass,
        bool infiniteMass,
        const DirectX::XMVECTOR& centerOfMassLocalPosition,
        const DirectX::XMVECTOR& inertia,
        const Dynamics& dynamics,
        const DirectX::XMVECTOR& force,
        const DirectX::XMVECTOR& torque,
        const Vertices& vertices,
        const Edges& edges,
        const Triangles& triangles );

    void destroy();
    
    void draw(
        GraphicsContext& gfxContext,
        const Math::Matrix4& ViewProjMat ) const;

    void setDynamics(
        const DirectX::XMVECTOR& position,
        const DirectX::XMVECTOR& orientation,
        const DirectX::XMVECTOR& velocity,
        const DirectX::XMVECTOR& angularVelocity );

    void setDynamics( Dynamics const & dynamics );

    void stepDynamics( float deltaTime );

    //int numberOfVertices() const;

    //int numberOfEdges() const;

    //DirectX::XMVECTOR vertexPosition( int index ) const;

    //DirectX::XMVECTOR edgeDirection( int index ) const;

    //DirectX::XMVECTOR edgeDirectionUnit( int index ) const;

    //DirectX::XMVECTOR edgeNormalDirectionUnit( int index ) const;

    //float edgeLength( int index ) const;

    int UID() const
    {
        return m_UID;
    }

    //XMVECTORVectorType const & vertexPositionVector() const
    //{
    //    return m_vertexPositionVector;
    //}

    //XMVECTOR const & averageVertexPosition() const
    //{
    //    return m_averageVertexPosition;
    //}

    //XMVECTORVectorType const & edgeDirectionVector() const
    //{
    //    return m_edgeDirectionVector;
    //}

    //XMVECTORVectorType const & edgeDirectionUnitVector() const
    //{
    //    return m_edgeDirectionUnitVector;
    //}

    //XMVECTORVectorType const & edgeNormalDirectionUnitVector() const
    //{
    //    return m_edgeNormalDirectionUnitVector;
    //}

    //FloatVectorType const & edgeLengthVector() const
    //{
    //    return m_edgeLengthVector;
    //}

    //float area() const
    //{
    //    return m_area;
    //}

    float mass() const
    {
        return m_mass;
    }

    bool infiniteMass() const
    {
        return m_infiniteMass;
    }

    const DirectX::XMVECTOR& centerOfMassLocalPosition() const
    {
        return m_centerOfMassLocalPosition;
    }

    const DirectX::XMVECTOR& inertia() const
    {
        return m_inertia;
    }

    const Dynamics& dynamics() const 
    {
        return m_dynamics;
    }

    const DirectX::XMVECTOR& position() const
    {
        return m_dynamics.position();
    }

    const DirectX::XMVECTOR& orientation() const
    {
        return m_dynamics.orientation();
    }

    const DirectX::XMVECTOR& velocity() const
    {
        return m_dynamics.velocity();
    }

    const DirectX::XMVECTOR& angularVelocity() const
    {
        return m_dynamics.angularVelocity();
    }

    DirectX::FXMMATRIX transformation() const
    {
        return m_dynamics.transformation();
    }

    const DirectX::XMVECTOR& force() const
    {
        return m_force;
    }
    
    const DirectX::XMVECTOR& torque() const
    {
        return m_torque;
    }

    void addForceActingAtWorldPosition( const DirectX::XMVECTOR& worldForce, const DirectX::XMVECTOR& worldPosition );
    void addForceActingAtLocalPosition( const DirectX::XMVECTOR& localForce, const DirectX::XMVECTOR& localPosition );

    void addTorqueActingAtWorldPosition( const DirectX::XMVECTOR& worldForce, const DirectX::XMVECTOR& worldPosition );
    void addTorqueActingAtLocalPosition( const DirectX::XMVECTOR& localForce, const DirectX::XMVECTOR& localPosition );

    void setForce( const DirectX::XMVECTOR& force );
    void clearForce();
    void addForce( const DirectX::XMVECTOR& force );

    void setTorque( const DirectX::XMVECTOR& torque );
    void clearTorque();
    void addTorque( const DirectX::XMVECTOR& torque );

    //DirectX::XMVECTOR transformedAverageVertexPosition() const;

    DirectX::XMVECTOR centerOfMassWorldPosition() const;

    //DirectX::XMVECTOR transformedVertexPosition( int index ) const;

    //DirectX::XMVECTOR transformedEdgeDirection( int index ) const;

    //DirectX::XMVECTOR transformedEdgeDirectionUnit( int index ) const;

    //DirectX::XMVECTOR transformedEdgeNormalDirectionUnit( int index ) const;

    const Vertices& vertices() const
    {
        return m_vertices;
    }

    const Edges& edges() const
    {
        return m_edges;
    }

    const Triangles& triangles() const
    {
        return m_triangles;
    }

    //const D3D9Vertices& d3d9Vertices() const
    //{
    //    return m_d3d9Vertices;
    //}

    //const D3D9Indices& d3d9Indices() const
    //{
    //    return m_d3d9Indices;
    //}

    //const IDirect3DVertexBuffer9* d3d9VertexBufferPtr() const
    //{
    //    return m_d3d9VertexBufferPtr;
    //}

    //const IDirect3DIndexBuffer9* d3d9IndexBufferPtr() const
    //{
    //    return m_d3d9IndexBufferPtr;
    //}

    //const IDirect3DTexture9* d3d9TexturePtr() const
    //{
    //    return m_d3d9TexturePtr;
    //}
    
    const DebugVertices& debugVertexData() const
    {
        return m_debugVertexData;
    }

    const DebugVertexIndicies& debugVertexIndicies() const
    {
        return m_debugVertexIndicies;
    }

    const StructuredBuffer& debugVertexBuffer() const
    {
        return m_debugVertexBuffer;
    }

    const ByteAddressBuffer& debugVertexIndexBuffer() const
    {
        return m_debugVertexIndexBuffer;
    }
};

//float objectSignedDistanceFromPlaneToPoint(
//    DirectX::XMVECTOR const & planePosition,
//    DirectX::XMVECTOR const & planeUnitNormal,
//    DirectX::XMVECTOR const & point );
//
//bool objectBHasSeperatingEdge( Object const & objectA, Object const & objectB );
//
//bool objectCollision( Object const & objectA, Object const & objectB );
//
//void objectClosestFeatures(
//    float & smallestDistanceSquaredSoFar,
//    Object const & objectA, FeatureClass & objectAFeature, float & objectADistanceAlongEdge,
//    Object const & objectB, FeatureClass & objectBFeature, float & objectBDistanceAlongEdge );
//
//bool objectIntersect(
//    Object const & object,
//    DirectX::XMVECTOR const & orig, 
//    DirectX::XMVECTOR const & dir,
//    float & t, float & u, float & v );
//
//DirectX::XMVECTOR objectVelocityAtPoint( Object const & object, DirectX::XMVECTOR const & pointPosition );
//DirectX::XMVECTOR objectVertexVelocity( Object const & object, int const vertIndex );


}

#endif