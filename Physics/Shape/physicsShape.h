// Andrew Davies

#if !defined( PHYSICS_SHAPE_H )
#define PHYSICS_SHAPE_H

#include "Physics/Shape/physicsShapeFwd.h"
#include "Physics/Feature/physicsFeatureFwd.h"
#include "Dav/container/container.hpp"
#include <DirectXMath.h>
#include "Physics/Dynamics/physicsDynamics.h"

namespace Physics
{

#define PHYSICS_SHAPE_MAX_VERTS ( ( int )16 )
    
typedef Dav::Vector< DirectX::XMVECTOR, PHYSICS_SHAPE_MAX_VERTS > XMVECTORVectorType;
//typedef Dav::Vector< D3DXVECTOR4, PHYSICS_SHAPE_MAX_VERTS > D3DXVECTOR4VectorType;
typedef Dav::Vector< float, PHYSICS_SHAPE_MAX_VERTS > FloatVectorType;

class Shape
{
private:
    int m_UID;

    XMVECTORVectorType m_vertexPositionVector;  // Made clockwise in constructor.
    DirectX::XMVECTOR m_averageVertexPosition;
    
    XMVECTORVectorType m_edgeDirectionVector;
    XMVECTORVectorType m_edgeDirectionUnitVector;
    XMVECTORVectorType m_edgeNormalDirectionUnitVector;
    FloatVectorType m_edgeLengthVector;

    float m_area;
    float m_mass;	
    bool m_infiniteMass;

    DirectX::XMVECTOR m_centerOfMassLocalPosition;

    float m_momentOfInertia;
    
    Dynamics2D m_dynamics;

    DirectX::XMVECTOR m_force;
    float m_torque;

public:
    Shape();
    //Shape( const Shape& rhs )
    //    : m_UID( rhs.m_UID )
    //    , m_vertexPositionVector( rhs.m_vertexPositionVector )
    //    , m_averageVertexPosition( rhs.m_averageVertexPosition )
    //    , m_edgeDirectionVector( rhs.m_edgeDirectionVector )
    //    , m_edgeDirectionUnitVector( rhs.m_edgeDirectionUnitVector )
    //    , m_edgeNormalDirectionUnitVector( rhs.m_edgeNormalDirectionUnitVector )
    //    , m_edgeLengthVector( rhs.m_edgeLengthVector )
    //    , m_area( rhs.m_area )
    //    , m_mass( rhs.m_mass )
    //    , m_infiniteMass( rhs.m_infiniteMass )
    //    , m_centerOfMassLocalPosition( rhs.m_centerOfMassLocalPosition )
    //    , m_momentOfInertia( rhs.m_momentOfInertia )
    //    , m_dynamics( rhs.m_dynamics )
    //    , m_force( rhs.m_force )
    //    , m_torque( rhs.m_torque ) 
    //{
    //    //m_vertexPositionVector = rhs.m_vertexPositionVector;
    //    //m_averageVertexPosition = rhs.m_averageVertexPosition;
    //    //m_edgeDirectionVector = rhs.m_edgeDirectionVector;
    //    //m_edgeDirectionUnitVector = rhs.m_edgeDirectionUnitVector;
    //    //m_edgeNormalDirectionUnitVector = rhs.m_edgeNormalDirectionUnitVector;
    //    //m_edgeLengthVector = rhs.m_edgeLengthVector;
    //}

    Shape(
        int UID,
        XMVECTORVectorType const & vertexPositionVector,
        DirectX::XMVECTOR const & position,
        float orientation,
        DirectX::XMVECTOR const & velocity,
        float angularVelocity,
        DirectX::XMVECTOR const & force,
        float torque,
        bool infiniteMass );

    void setDynamics(	
        DirectX::XMVECTOR const & position,
        float orientation,
        DirectX::XMVECTOR const & velocity,
        float angularVelocity );
    
    void setDynamics( const Dynamics2D& dynamics );

    void stepDynamics( float deltaTime );
    
    //void step(
    //	float deltaTime,
    //	EngineShapeMapType & engineShapeMap );
    
    void draw( /*IDirect3DDevice9 & d3dDevice*/ ) const;

    int numberOfVertices() const;

    int numberOfEdges() const;
    
    DirectX::XMVECTOR vertexPosition( int index ) const;

    DirectX::XMVECTOR edgeDirection( int index ) const;

    DirectX::XMVECTOR edgeDirectionUnit( int index ) const;

    DirectX::XMVECTOR edgeNormalDirectionUnit( int index ) const;

    float edgeLength( int index ) const;
    
    int UID() const
    {
        return m_UID;
    }

    XMVECTORVectorType const & vertexPositionVector() const
    {
        return m_vertexPositionVector;
    }

    DirectX::XMVECTOR const & averageVertexPosition() const
    {
        return m_averageVertexPosition;
    }

    XMVECTORVectorType const & edgeDirectionVector() const
    {
        return m_edgeDirectionVector;
    }

    XMVECTORVectorType const & edgeDirectionUnitVector() const
    {
        return m_edgeDirectionUnitVector;
    }

    XMVECTORVectorType const & edgeNormalDirectionUnitVector() const
    {
        return m_edgeNormalDirectionUnitVector;
    }

    FloatVectorType const & edgeLengthVector() const
    {
        return m_edgeLengthVector;
    }

    float area() const
    {
        return m_area;
    }

    float mass() const
    {
        return m_mass;
    }

    bool infiniteMass() const
    {
        return m_infiniteMass;
    }

    DirectX::XMVECTOR const & centerOfMassLocalPosition() const
    {
        return m_centerOfMassLocalPosition;
    }

    float momentOfInertia() const
    {
        return m_momentOfInertia;
    }
    
    Dynamics2D const & dynamics() const 
    {
        return m_dynamics;
    }

    const DirectX::XMVECTOR& position() const;
    float orientation() const;
    const DirectX::XMVECTOR& velocity() const;
    float angularVelocity() const;
    DirectX::FXMMATRIX transformation() const;
    
    DirectX::XMVECTOR const & force() const
    {
        return m_force;
    }

    float torque() const
    {
        return m_torque;
    }	

    void addForceActingAtWorldPosition(
        const DirectX::XMVECTOR& force,
        const DirectX::XMVECTOR& worldPosition );

    void setForce( const DirectX::XMVECTOR& force );
    void clearForce();
    void addForce( const DirectX::XMVECTOR& force );

    void setTorque( float torque );
    void clearTorque();
    void addTorque( float torque );

    DirectX::XMVECTOR transformedAverageVertexPosition() const;

    DirectX::XMVECTOR centerOfMassWorldPosition() const;

    DirectX::XMVECTOR transformedVertexPosition( int index ) const;

    DirectX::XMVECTOR transformedEdgeDirection( int index ) const;

    DirectX::XMVECTOR transformedEdgeDirectionUnit( int index ) const;

    DirectX::XMVECTOR transformedEdgeNormalDirectionUnit( int index ) const;
};

//Shape * shapeInCollision(	
//	Shape const & shape,
//	EngineShapeMapType & engineShapeMap );

float shapeSignedDistanceFromPlaneToPoint(
    DirectX::XMVECTOR const & planePosition,
    DirectX::XMVECTOR const & planeUnitNormal,
    DirectX::XMVECTOR const & point );

bool shapeBHasSeperatingEdge( Shape const & shapeA, Shape const & shapeB );

bool shapeCollision( Shape const & shapeA, Shape const & shapeB );

void shapeClosestFeatures(
    float & smallestDistanceSquaredSoFar,
    Shape const & shapeA, FeatureClass & shapeAFeature, float & shapeADistanceAlongEdge,
    Shape const & shapeB, FeatureClass & shapeBFeature, float & shapeBDistanceAlongEdge );

bool shapeIntersect(
    Shape const & shape,
    DirectX::XMVECTOR const & orig, 
    DirectX::XMVECTOR const & dir,
    float & t, float & u, float & v );

DirectX::XMVECTOR shapeVelocityAtPoint( Shape const & shape, DirectX::XMVECTOR const & pointPosition );
DirectX::XMVECTOR shapeVertexVelocity( Shape const & shape, int const vertIndex );


}

#endif