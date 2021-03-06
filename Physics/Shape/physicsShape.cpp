// Andrew Davies

#include "pch.h"
#include "Physics/Shape/physicsShape.h"
#include "Physics/Feature/physicsFeature.h"
#include "Misc/misc.h"
#include <float.h>
//#include <limits.h>

//using namespace std;
using namespace DirectX;

namespace Physics
{

Shape::Shape()
    : m_UID(0)
    , m_averageVertexPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_area(0.0f)
    , m_mass(0.0f)
    , m_infiniteMass(false)
    , m_centerOfMassLocalPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_momentOfInertia(0.0f)
    , m_dynamics()
    , m_force{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_torque(0.0f)
{
}

Shape::Shape(
    int const UID,
    XMVECTORVectorType const & vertexPositionVector,
    XMVECTOR const & position,
    float const orientation,
    XMVECTOR const & velocity,
    float const angularVelocity,
    XMVECTOR const & force,
    float const torque,	
    bool const infiniteMass )	
    : m_UID( UID )
    , m_vertexPositionVector( vertexPositionVector )
    , m_averageVertexPosition{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_edgeDirectionVector( vertexPositionVector ) // Mainly to dimension it.
    , m_edgeDirectionUnitVector( vertexPositionVector ) // Mainly to dimension it.
    , m_edgeNormalDirectionUnitVector( vertexPositionVector ) // Mainly to dimension it.
    , m_edgeLengthVector()
    , m_area( 0.0f )
    , m_mass( 0.0f )
    , m_infiniteMass( infiniteMass )
    , m_centerOfMassLocalPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_momentOfInertia( 0.0f )
    , m_dynamics()
    , m_force( force )
    , m_torque( torque )
{
    const int numberOfVertices = ( int )vertexPositionVector.size();
    for( int vertexPositionIndex = 0; vertexPositionIndex != numberOfVertices; ++vertexPositionIndex )
    {
        int nextVertexPositionIndex = vertexPositionIndex + 1;
        if( nextVertexPositionIndex == numberOfVertices )
        {
            nextVertexPositionIndex = 0;
        }

        //  http://local.wasp.uwa.edu.au/~pbourke/geometry/polyarea/
        float const temp = 
            ( XMVectorGetX( vertexPositionVector[ vertexPositionIndex ] ) * XMVectorGetY( vertexPositionVector[ nextVertexPositionIndex ] ) ) -
            ( XMVectorGetX( vertexPositionVector[ nextVertexPositionIndex ] ) * XMVectorGetY( vertexPositionVector[ vertexPositionIndex ] ) );

        m_area += temp;

        const XMVECTOR vertPosPlusNext = XMVectorAdd( vertexPositionVector[ vertexPositionIndex ], vertexPositionVector[ nextVertexPositionIndex ] );
        const XMVECTOR vertPosPlusNextMultTemp = XMVectorScale( vertPosPlusNext, temp );
        m_centerOfMassLocalPosition = XMVectorAdd( m_centerOfMassLocalPosition, vertPosPlusNextMultTemp );
        XMVectorSetZ( m_centerOfMassLocalPosition, 0.0f );
        //m_centerOfMassLocalPosition.x += ( vertexPositionVector[ vertexPositionIndex ].x + vertexPositionVector[ nextVertexPositionIndex ].x ) * temp;
        //m_centerOfMassLocalPosition.y += ( vertexPositionVector[ vertexPositionIndex ].y + vertexPositionVector[ nextVertexPositionIndex ].y ) * temp;

        m_averageVertexPosition += vertexPositionVector[ vertexPositionIndex ];
    }
    if( numberOfVertices > 0 )
    {
        m_averageVertexPosition /= ( float )numberOfVertices;
    }
    m_area /= 2.0f;

    if( fabsf( m_area ) > 0.0f )
    {
        m_centerOfMassLocalPosition *= ( 1.0f / ( 6.0f * m_area ) );
    }
    else
    {
        XMVectorSetX( m_centerOfMassLocalPosition, 0.0f );
        XMVectorSetY( m_centerOfMassLocalPosition, 0.0f );
    }
    XMVectorSetW( m_centerOfMassLocalPosition, 1.0f );

    // if area is positive at this point then the vertices are wound anti-clockwise and need to be flipped.
    if( m_area > 0.0f )
    {
        XMVECTORVectorType::const_reverse_iterator const beginInputVertexPositionReverseItr = vertexPositionVector.rbegin();
        XMVECTORVectorType::const_reverse_iterator const endInputVertexPositionReverseItr = vertexPositionVector.rend();
        XMVECTORVectorType::const_reverse_iterator inputVertexPositionReverseItr = beginInputVertexPositionReverseItr;
        
        XMVECTORVectorType::iterator const beginVertexPositionItr = m_vertexPositionVector.begin();
        XMVECTORVectorType::iterator const endVertexPositionItr = m_vertexPositionVector.end();
        XMVECTORVectorType::iterator vertexPositionItr = beginVertexPositionItr;
        
        for( ; vertexPositionItr != endVertexPositionItr; ++vertexPositionItr, ++inputVertexPositionReverseItr )
        {
            *vertexPositionItr = *inputVertexPositionReverseItr;
        }
    }
    else
    {		
        m_area = -m_area;
    }

    m_mass = ( infiniteMass ? ( FLT_MAX ) : m_area );

    for( int vertexPositionIndex = 0; vertexPositionIndex != numberOfVertices; ++vertexPositionIndex )
    {
        int nextVertexPositionIndex = vertexPositionIndex + 1;
        if( nextVertexPositionIndex == numberOfVertices )
        {
            nextVertexPositionIndex = 0;
        }
    
        XMVECTOR const edgeDirection = m_vertexPositionVector[ nextVertexPositionIndex ] - m_vertexPositionVector[ vertexPositionIndex ];
        float const edgeLength = XMVectorGetX( XMVector4Length( edgeDirection ) );
        XMVECTOR edgeDirectionUnit = edgeDirection;
        edgeDirectionUnit = XMVector4Normalize( edgeDirectionUnit );
        XMVECTOR edgeNormalDirectionUnit{ -XMVectorGetY( edgeDirection ), XMVectorGetX( edgeDirection ), XMVectorGetZ( edgeDirection ), XMVectorGetW( edgeDirection ) };
        edgeNormalDirectionUnit = XMVector4Normalize( edgeNormalDirectionUnit );

        m_edgeDirectionVector[ vertexPositionIndex ] = edgeDirection;
        m_edgeDirectionUnitVector[ vertexPositionIndex ] = edgeDirectionUnit;
        m_edgeNormalDirectionUnitVector[ vertexPositionIndex ] = edgeNormalDirectionUnit;
        m_edgeLengthVector.push_back( edgeLength );
    }

    // Now we have center of mass we can translate vertices to calculate moment of inertia	
    float momentOfInertiaDenominator = 0.0f;
    float momentOfInertiaDevisor = 0.0f;
    for( int vertexPositionIndex = 0; vertexPositionIndex != numberOfVertices; ++vertexPositionIndex )
    {
        int nextVertexPositionIndex = vertexPositionIndex + 1;
        if( nextVertexPositionIndex == numberOfVertices )
        {
            nextVertexPositionIndex = 0;
        }

        XMVECTOR const vertexPosition = vertexPositionVector[ vertexPositionIndex ] - m_centerOfMassLocalPosition;
        XMVECTOR const nextVertexPosition = vertexPositionVector[ nextVertexPositionIndex ] - m_centerOfMassLocalPosition;
        
        const XMVECTOR nextVertexPositionCrossVertexPosition = XMVector4Cross( nextVertexPosition, vertexPosition, Misc::origin() );
        //D3DXVec4Cross( &nextVertexPositionCrossVertexPosition, &nextVertexPosition, &vertexPosition, &Misc::origin() );
        float const nextVertexPositionCrossVertexPositionLength = XMVectorGetX( XMVector4Length( nextVertexPositionCrossVertexPosition ) );
        float const vertexPositionLengthSquared = XMVectorGetX( XMVector4LengthSq( vertexPosition ) );
        float const nextVertexPositionLengthSquared = XMVectorGetX( XMVector4LengthSq( nextVertexPosition ) );
        float const nextVertexPositionDotVertexPosition = XMVectorGetX( XMVector4Dot( nextVertexPosition, vertexPosition ) );

        momentOfInertiaDenominator += nextVertexPositionCrossVertexPositionLength * ( nextVertexPositionLengthSquared + nextVertexPositionDotVertexPosition + vertexPositionLengthSquared );
        momentOfInertiaDevisor += nextVertexPositionCrossVertexPositionLength;
    }

    if( fabsf( momentOfInertiaDevisor ) > 0.0f )
    {
        m_momentOfInertia = ( m_mass / 6.0f ) * ( momentOfInertiaDenominator / momentOfInertiaDevisor );
    }

    if( infiniteMass )
    {	
        m_momentOfInertia = FLT_MAX;
    }

    m_dynamics.set( position, orientation, velocity, angularVelocity, m_centerOfMassLocalPosition );
}

void Shape::setDynamics(	
    XMVECTOR const & position,
    float const orientation,
    XMVECTOR const & velocity,
    float const angularVelocity )
{		
    m_dynamics.set( position, orientation, velocity, angularVelocity, m_centerOfMassLocalPosition );
}

void Shape::setDynamics( const Dynamics2D& dynamics )
{
    m_dynamics = dynamics;
}

void Shape::stepDynamics( float const deltaTime )
{		
    m_dynamics.step( m_force, m_torque, deltaTime, m_infiniteMass, m_mass, m_momentOfInertia, m_centerOfMassLocalPosition );
}

//Shape * shapeInCollision(	
//	Shape const & shape,
//	EngineShapeMapType & engineShapeMap )
//{
//	for( EngineShapeMapType::iterator shapeItr = engineShapeMap.begin(); shapeItr != engineShapeMap.end(); ++shapeItr )
//	{
//		if( &shape != &shapeItr->second )
//		{
//			if( shapeCollision( shape, shapeItr->second ) )
//			{
//				return &shapeItr->second;
//			}
//		}
//	}
//
//	return 0;
//}

//void Shape::step(
//	float const deltaTime,
//	EngineShapeMapType & engineShapeMap )
//{
//	const Dynamics2D initialDynamics = m_dynamics;
//
//	m_dynamics.step( m_force, m_torque, deltaTime, m_mass, m_momentOfInertia, m_centerOfMassLocalPosition );
//
//	Shape * collidedShapePtr = shapeInCollision( *this, engineShapeMap );
//	if( collidedShapePtr != 0 )
//	{
//		if( !m_infiniteMass || !collidedShapePtr->infiniteMass() )
//		{
//			m_dynamics = initialDynamics;
//
//			//assert( shapeInCollision( *this, engineShapeMap ) == 0 );
//
//			FeatureClass shapeFeature;
//			FeatureClass collidedShapeFeature;
//			
//			float shapeDistanceAlongEdge = 0.0f;
//			float collidedShapeDistanceAlongEdge = 0.0f;
//			
//			shapeClosestFeatures(
//				*this, shapeFeature, shapeDistanceAlongEdge,
//				*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
//			assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );
//
//			Shape * shapeAPtr = 0;
//			Shape * shapeBPtr = 0;
//			XMVECTOR collisionPosition( 0.0f, 0.0f, 0.0f, 1.0f );
//			XMVECTOR collisionUnitNormal( 0.0f, 0.0f, 1.0f, 0.0f );
//
//			if( shapeFeature.type() == VertexFeature )
//			{
//				shapeAPtr = this;
//				shapeBPtr = collidedShapePtr;
//				collisionPosition = this->transformedVertexPosition( shapeFeature.index() );
//				collisionUnitNormal = collidedShapePtr->transformedEdgeNormalDirectionUnit( collidedShapeFeature.index() );
//			}
//			else // if( collidedShapeFeature.type() == VertexFeature )
//			{
//				shapeAPtr = collidedShapePtr;
//				shapeBPtr = this;
//				collisionPosition = collidedShapePtr->transformedVertexPosition( collidedShapeFeature.index() );
//				collisionUnitNormal = this->transformedEdgeNormalDirectionUnit( shapeFeature.index() );
//			}		
//			//else
//			//{
//			//	// Edge edge whitnesses.
//
//			//	shapeAPtr = this;
//			//	shapeBPtr = collidedShapePtr;
//			//	collisionPosition = 
//			//		this->transformedVertexPosition( shapeFeature.index() ) +
//			//		( this->transformedEdgeDirection( shapeFeature.index() ) * 0.5f );
//			//	collisionUnitNormal = collidedShapePtr->transformedEdgeNormalDirectionUnit( collidedShapeFeature.index() );
//			//}
//
//			XMVECTOR const radiusA = collisionPosition - shapeAPtr->centerOfMassWorldPosition();
//			XMVECTOR const radiusB = collisionPosition - shapeBPtr->centerOfMassWorldPosition();
//				
//			XMVECTOR const radiusTangentA( -radiusA.y, radiusA.x, radiusA.z, radiusA.w );
//			XMVECTOR const radiusTangentB( -radiusB.y, radiusB.x, radiusB.z, radiusB.w );
//
//			XMVECTOR const velAP = shapeAPtr->dynamics().velocity() + ( radiusTangentA * shapeAPtr->dynamics().angularVelocity() );
//			XMVECTOR const velBP = shapeBPtr->dynamics().velocity() + ( radiusTangentB * shapeBPtr->dynamics().angularVelocity() );
//			XMVECTOR const relVel = velAP - velBP;
//
//			float const relVelDotNormal = D3DXVec4Dot( &relVel, &collisionUnitNormal );
//			float const radiusTangentADotNormal = D3DXVec4Dot( &radiusTangentA, &collisionUnitNormal );
//			float const radiusTangentBDotNormal = D3DXVec4Dot( &radiusTangentB, &collisionUnitNormal );
//
//			float reciprocalMassSum = 0.0f;		
//			float reciprocalInertiaSum = 0.0f;
//			if( !shapeAPtr->infiniteMass() )
//			{
//				reciprocalMassSum += ( 1.0f / shapeAPtr->mass() );
//				reciprocalInertiaSum += ( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
//			}
//			if( !shapeBPtr->infiniteMass() )
//			{
//				reciprocalMassSum += ( 1.0f / shapeBPtr->mass() );
//				reciprocalInertiaSum += ( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
//			}
//			//float const reciprocalMassSum = ( 1.0f / shapeAPtr->mass() ) + ( 1.0f / shapeBPtr->mass() );	
//
//			
//			//float const reciprocalInertiaSum = 
//			//	( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() ) + 
//			//	( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
//
//			float const e = 0.2f;
//			float const j = ( -( 1.0f + e ) * relVelDotNormal ) / ( reciprocalMassSum + reciprocalInertiaSum );
//			
//			XMVECTOR const velA2 = shapeAPtr->dynamics().velocity() + ( ( j / shapeAPtr->mass() ) * collisionUnitNormal );
//			XMVECTOR const velB2 = shapeBPtr->dynamics().velocity() - ( ( j / shapeBPtr->mass() ) * collisionUnitNormal );
//
//			float const angVelA2 = shapeAPtr->dynamics().angularVelocity() + ( ( j * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
//			float const angVelB2 = shapeBPtr->dynamics().angularVelocity() - ( ( j * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
//
//			if( !shapeAPtr->infiniteMass() )
//			{
//				shapeAPtr->setDynamics(
//					shapeAPtr->dynamics().position(),
//					shapeAPtr->dynamics().orientation(),
//					velA2,
//					angVelA2 );
//				
//				//assert( shapeInCollision( *this, engineShapeMap ) == 0 );
//			}
//
//			if( !shapeBPtr->infiniteMass() )
//			{
//				shapeBPtr->setDynamics(
//					shapeBPtr->dynamics().position(),
//					shapeBPtr->dynamics().orientation(),
//					velB2,
//					angVelB2 );
//				
//				//assert( shapeInCollision( *this, engineShapeMap ) == 0 );
//			}
//		}
//	}
//
//}

struct ShapeVertexStruct
{
    FLOAT x, y, z; //, rhw; // The transformed position for the vertex
    XMVECTOR colour;        // The vertex colour
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE) // (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

void Shape::draw( /*IDirect3DDevice9 & d3dDevice*/ ) const
{
    const XMVECTOR colour = XMVECTOR{ 0.0f, 1.0f, 0.0f, 1.0f };

    const int numberOfVertices = ( int )m_vertexPositionVector.size();
    if( numberOfVertices > 1 )
    { 
        const int vertexArraySize = ( int )( PHYSICS_SHAPE_MAX_VERTS * 2 );
        ShapeVertexStruct frontVertexArray[ vertexArraySize ];
        ShapeVertexStruct rearVertexArray[ vertexArraySize ];
        assert( ( numberOfVertices * 2 ) <= vertexArraySize );
        
        frontVertexArray[ 0 ].x = XMVectorGetX( m_averageVertexPosition );
        frontVertexArray[ 0 ].y = XMVectorGetY( m_averageVertexPosition );
        frontVertexArray[ 0 ].z = XMVectorGetZ( m_averageVertexPosition );
        frontVertexArray[ 0 ].colour = XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f };
        rearVertexArray[ 0 ] = frontVertexArray[ 0 ];

        for( int vertexIndex = 0; vertexIndex != numberOfVertices; ++vertexIndex )
        {
            frontVertexArray[ vertexIndex + 1 ].x = XMVectorGetX( m_vertexPositionVector[ vertexIndex ] );
            frontVertexArray[ vertexIndex + 1 ].y = XMVectorGetY( m_vertexPositionVector[ vertexIndex ] );
            frontVertexArray[ vertexIndex + 1 ].z = 0.0f;
            frontVertexArray[ vertexIndex + 1 ].colour = colour;
            rearVertexArray[ vertexIndex + 1 ].x = XMVectorGetX( m_vertexPositionVector[ numberOfVertices - ( vertexIndex + 1 ) ] );
            rearVertexArray[ vertexIndex + 1 ].y = XMVectorGetY( m_vertexPositionVector[ numberOfVertices - ( vertexIndex + 1 ) ] );
            rearVertexArray[ vertexIndex + 1 ].z = 0.0f;
            rearVertexArray[ vertexIndex + 1 ].colour = colour;
        }

        frontVertexArray[ numberOfVertices + 1 ].x = XMVectorGetX( m_vertexPositionVector[ 0 ] );
        frontVertexArray[ numberOfVertices + 1 ].y = XMVectorGetY( m_vertexPositionVector[ 0 ] );
        frontVertexArray[ numberOfVertices + 1 ].z = 0.0f;
        frontVertexArray[ numberOfVertices + 1 ].colour = colour;
        rearVertexArray[ numberOfVertices + 1 ].x = XMVectorGetX( m_vertexPositionVector[ numberOfVertices - 1 ] );
        rearVertexArray[ numberOfVertices + 1 ].y = XMVectorGetY( m_vertexPositionVector[ numberOfVertices - 1 ] );
        rearVertexArray[ numberOfVertices + 1 ].z = 0.0f;
        rearVertexArray[ numberOfVertices + 1 ].colour = colour;
                    
        //d3dDevice.SetTransform( D3DTS_WORLD, &( m_dynamics.transformation() ) );
        //d3dDevice.SetFVF( D3DFVF_CUSTOMVERTEX );
        //d3dDevice.DrawPrimitiveUP( 
        //    D3DPT_TRIANGLEFAN, // D3DPRIMITIVETYPE PrimitiveType,
        //    numberOfVertices, // UINT PrimitiveCount,
        //    frontVertexArray, // CONST void* pVertexStreamZeroData,
        //    sizeof( ShapeVertexStruct ) ); // UINT VertexStreamZeroStride
        //d3dDevice.DrawPrimitiveUP( 
        //    D3DPT_TRIANGLEFAN, // D3DPRIMITIVETYPE PrimitiveType,
        //    numberOfVertices, // UINT PrimitiveCount,
        //    rearVertexArray, // CONST void* pVertexStreamZeroData,
        //    sizeof( ShapeVertexStruct ) ); // UINT VertexStreamZeroStride
    }

    //#if 1
    //// Feature
    //{
    //	CUSTOMVERTEX vertexArray[ 2 ];
    //	float const edgeLineLength = 3.0f;	
    //	
    //	D3DXMATRIXA16 matWorld;
    //	D3DXMatrixIdentity( &matWorld );
    //	d3dDevice.SetTransform( D3DTS_WORLD, &matWorld );
    //	d3dDevice.SetFVF( D3DFVF_CUSTOMVERTEX );
    //	
    //	DWORD const colourArray[ ] =
    //	{
    //		D3DCOLOR_COLORVALUE( 0.0f, 0.0f, 1.0f, 1.0f ), // VClipDone,
    //		D3DCOLOR_COLORVALUE( 0.0f, 1.0f, 0.0f, 1.0f ), // VClipContinue,
    //		D3DCOLOR_COLORVALUE( 1.0f, 0.0f, 0.0f, 1.0f ), // VClipPenetration
    //	};

    //	switch( feature.type() )
    //	{
    //	case VertexFeature :
    //		{
    //			unsigned int const vertexIndex = feature.index();
    //			unsigned int const edgeIndex = feature.index();
    //			unsigned int const previousEdgeIndex = ( ( feature.index() == 0 ) ? ( shape.vertexVector().size() - 1 ) : ( feature.index() - 1 ) );
    //			
    //			XMVECTOR const shapeTransformedVertex = shape.transformedVertexPosition( vertexIndex );
    //			XMVECTOR const shapeTransformedEdgeNormalDirectionUnit = shape.transformedEdgeNormalDirectionUnit( edgeIndex );
    //			XMVECTOR const shapeTransformedPreviousEdgeNormalDirectionUnit = shape.transformedEdgeNormalDirectionUnit( previousEdgeIndex );

    //			vertexArray[ 0 ].x = shapeTransformedVertex.x;
    //			vertexArray[ 0 ].y = shapeTransformedVertex.y;
    //			vertexArray[ 0 ].z = shapeTransformedVertex.z;
    //			vertexArray[ 0 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //			vertexArray[ 1 ].x = shapeTransformedVertex.x + ( shapeTransformedEdgeNormalDirectionUnit.x * edgeLineLength );
    //			vertexArray[ 1 ].y = shapeTransformedVertex.y + ( shapeTransformedEdgeNormalDirectionUnit.y * edgeLineLength );
    //			vertexArray[ 1 ].z = shapeTransformedVertex.z + ( shapeTransformedEdgeNormalDirectionUnit.z * edgeLineLength );
    //			vertexArray[ 1 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //				
    //			d3dDevice.DrawPrimitiveUP( 
    //				D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
    //				1, // UINT PrimitiveCount,
    //				vertexArray, // CONST void* pVertexStreamZeroData,
    //				sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride
    //				
    //			vertexArray[ 0 ].x = shapeTransformedVertex.x;
    //			vertexArray[ 0 ].y = shapeTransformedVertex.y;
    //			vertexArray[ 0 ].z = shapeTransformedVertex.z;
    //			vertexArray[ 0 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //			vertexArray[ 1 ].x = shapeTransformedVertex.x + ( shapeTransformedPreviousEdgeNormalDirectionUnit.x * edgeLineLength );
    //			vertexArray[ 1 ].y = shapeTransformedVertex.y + ( shapeTransformedPreviousEdgeNormalDirectionUnit.y * edgeLineLength );
    //			vertexArray[ 1 ].z = shapeTransformedVertex.z + ( shapeTransformedPreviousEdgeNormalDirectionUnit.z * edgeLineLength );
    //			vertexArray[ 1 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //				
    //			d3dDevice.DrawPrimitiveUP( 
    //				D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
    //				1, // UINT PrimitiveCount,
    //				vertexArray, // CONST void* pVertexStreamZeroData,
    //				sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride				
    //		}
    //		break;

    //	case EdgeFeature :
    //	case FaceFeature :
    //	case numberOfFeatures :
    //	default :
    //		{
    //			unsigned int const edgeIndex = feature.index();
    //			unsigned int const tailVertexIndex = feature.index();
    //			unsigned int const headVertexIndex = ( ( feature.index() == ( shape.vertexVector().size() - 1 ) ) ? 0 : ( feature.index() + 1 ) );
    //			
    //			XMVECTOR const shapeTransformedTailVertex = shape.transformedVertexPosition( tailVertexIndex );
    //			XMVECTOR const shapeTransformedHeadVertex = shape.transformedVertexPosition( headVertexIndex );
    //			XMVECTOR const shapeTransformedEdgeNormalDirectionUnit = shape.transformedEdgeNormalDirectionUnit( edgeIndex );

    //			vertexArray[ 0 ].x = shapeTransformedTailVertex.x;
    //			vertexArray[ 0 ].y = shapeTransformedTailVertex.y;
    //			vertexArray[ 0 ].z = shapeTransformedTailVertex.z;
    //			vertexArray[ 0 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //			vertexArray[ 1 ].x = shapeTransformedTailVertex.x + ( shapeTransformedEdgeNormalDirectionUnit.x * edgeLineLength );
    //			vertexArray[ 1 ].y = shapeTransformedTailVertex.y + ( shapeTransformedEdgeNormalDirectionUnit.y * edgeLineLength );
    //			vertexArray[ 1 ].z = shapeTransformedTailVertex.z + ( shapeTransformedEdgeNormalDirectionUnit.z * edgeLineLength );
    //			vertexArray[ 1 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //				
    //			d3dDevice.DrawPrimitiveUP( 
    //				D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
    //				1, // UINT PrimitiveCount,
    //				vertexArray, // CONST void* pVertexStreamZeroData,
    //				sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride
    //				
    //			vertexArray[ 0 ].x = shapeTransformedHeadVertex.x;
    //			vertexArray[ 0 ].y = shapeTransformedHeadVertex.y;
    //			vertexArray[ 0 ].z = shapeTransformedHeadVertex.z;
    //			vertexArray[ 0 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //			vertexArray[ 1 ].x = shapeTransformedHeadVertex.x + ( shapeTransformedEdgeNormalDirectionUnit.x * edgeLineLength );
    //			vertexArray[ 1 ].y = shapeTransformedHeadVertex.y + ( shapeTransformedEdgeNormalDirectionUnit.y * edgeLineLength );
    //			vertexArray[ 1 ].z = shapeTransformedHeadVertex.z + ( shapeTransformedEdgeNormalDirectionUnit.z * edgeLineLength );
    //			vertexArray[ 1 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //				
    //			d3dDevice.DrawPrimitiveUP( 
    //				D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
    //				1, // UINT PrimitiveCount,
    //				vertexArray, // CONST void* pVertexStreamZeroData,
    //				sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride				
    //				
    //			vertexArray[ 0 ].x = shapeTransformedTailVertex.x;
    //			vertexArray[ 0 ].y = shapeTransformedTailVertex.y;
    //			vertexArray[ 0 ].z = shapeTransformedTailVertex.z;
    //			vertexArray[ 0 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //			vertexArray[ 1 ].x = shapeTransformedHeadVertex.x;
    //			vertexArray[ 1 ].y = shapeTransformedHeadVertex.y;
    //			vertexArray[ 1 ].z = shapeTransformedHeadVertex.z;
    //			vertexArray[ 1 ].colour = colourArray[ g_VClipState ]; // D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //				
    //			d3dDevice.DrawPrimitiveUP( 
    //				D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
    //				1, // UINT PrimitiveCount,
    //				vertexArray, // CONST void* pVertexStreamZeroData,
    //				sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride				
    //		}
    //		break;
    //	}
    //}
    //#endif

}

//void renderFeatureConnection(
//	IDirect3DDevice9* pd3dDevice,
//	Shape const & shapeA,
//	FeatureClass const & shapeAFeature,
//	float const shapeADistanceAlongEdge,
//	Shape const & shapeB,
//	FeatureClass const & shapeBFeature,
//	float const shapeBDistanceAlongEdge )
//{
//	XMVECTOR fromPosition = shapeA.dynamics().position();	
//	switch( shapeAFeature.type() )
//	{
//	case VertexFeature :
//		{
//			fromPosition = shapeA.transformedVertexPosition( shapeAFeature.index() );
//		}
//		break;
//
//	case EdgeFeature :
//	case FaceFeature :
//	case numberOfFeatures :
//	default :
//		{
//			fromPosition = 
//				shapeA.transformedVertexPosition( shapeAFeature.index() ) +
//				( shapeA.transformedEdgeDirectionUnit( shapeAFeature.index() ) * shapeADistanceAlongEdge );
//				//( shapeA.transformedEdgeDirection( shapeAFeature.index() ) * 0.5f );
//		}
//		break;
//	}
//
//	XMVECTOR toPosition = shapeB.dynamics().position();
//	switch( shapeBFeature.type() )
//	{
//	case VertexFeature :
//		{
//			toPosition = shapeB.transformedVertexPosition( shapeBFeature.index() );
//		}
//		break;
//
//	case EdgeFeature :
//	case FaceFeature :
//	case numberOfFeatures :
//	default :
//		{
//			toPosition = 
//				shapeB.transformedVertexPosition( shapeBFeature.index() ) +
//				( shapeB.transformedEdgeDirectionUnit( shapeBFeature.index() ) * shapeBDistanceAlongEdge );
//				//( shapeB.transformedEdgeDirection( shapeBFeature.index() ) * 0.5f );
//		}
//		break;
//	}
//
//	CUSTOMVERTEX vertexArray[ 2 ];
//	vertexArray[ 0 ].x = fromPosition.x;
//	vertexArray[ 0 ].y = fromPosition.y;
//	vertexArray[ 0 ].z = fromPosition.z;
//	vertexArray[ 0 ].colour = D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
//	vertexArray[ 1 ].x = toPosition.x;
//	vertexArray[ 1 ].y = toPosition.y;
//	vertexArray[ 1 ].z = toPosition.z;
//	vertexArray[ 1 ].colour = D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
//		
//	D3DXMATRIXA16 matWorld;
//	D3DXMatrixIdentity( &matWorld );
//	pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
//	pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
//	pd3dDevice->DrawPrimitiveUP( 
//		D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
//		1, // UINT PrimitiveCount,
//		vertexArray, // CONST void* pVertexStreamZeroData,
//		sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride
//}

int Shape::numberOfVertices() const
{
    return ( int )m_vertexPositionVector.size();
}

int Shape::numberOfEdges() const
{
    return ( int )m_edgeDirectionVector.size();
}

XMVECTOR Shape::vertexPosition( const int index ) const
{
    assert( index < numberOfVertices() );
    return m_vertexPositionVector[ index ];
}

XMVECTOR Shape::edgeDirection( const int index ) const
{
    assert( index < numberOfEdges() );
    return m_edgeDirectionVector[ index ];
}

XMVECTOR Shape::edgeDirectionUnit( const int index ) const
{
    assert( index < numberOfEdges() );
    return m_edgeDirectionUnitVector[ index ];
}

XMVECTOR Shape::edgeNormalDirectionUnit( const int index ) const
{
    assert( index < numberOfEdges() );
    return m_edgeNormalDirectionUnitVector[ index ];
}

float Shape::edgeLength( const int index ) const
{
    assert( index < numberOfEdges() );
    return m_edgeLengthVector[ index ];
}

const XMVECTOR& Shape::position() const
{
    return m_dynamics.position();
}

float Shape::orientation() const
{
    return m_dynamics.orientation();
}

const XMVECTOR& Shape::velocity() const
{
    return m_dynamics.velocity();
}

float Shape::angularVelocity() const
{
    return m_dynamics.angularVelocity();
}

FXMMATRIX Shape::transformation() const
{
    return m_dynamics.transformation();
}

void Shape::addForceActingAtWorldPosition( 
    const XMVECTOR& force,
    const XMVECTOR& worldPos )
{
    m_force += force;
    
    XMVECTOR const COMPos = centerOfMassWorldPosition();
    XMVECTOR const localPos = worldPos - COMPos;
    
    const XMVECTOR perpLocalPos{ -XMVectorGetY( localPos ), XMVectorGetX( localPos ), XMVectorGetZ( localPos ), XMVectorGetW( localPos ) };
    
    m_torque += ( XMVectorGetX( perpLocalPos ) * XMVectorGetX( force ) ) + ( XMVectorGetY( perpLocalPos ) * XMVectorGetY( force ) );
    //m_torque += ( perpLocalPos[ 0 ] * force[ 0 ] ) + ( perpLocalPos[ 1 ] * force[ 1 ] );
}

void Shape::setForce( const XMVECTOR& force )
{
    m_force = force;
}

void Shape::clearForce()
{
    m_force = Misc::zero();
}

void Shape::addForce( const XMVECTOR& force )
{
    m_force += force;
}

void Shape::setTorque( const float torque )
{
    m_torque = torque;
}

void Shape::clearTorque()
{
    m_torque = 0.0f;
}

void Shape::addTorque( const float torque )
{
    m_torque += torque;
}

XMVECTOR Shape::transformedAverageVertexPosition() const
{
    return XMVector4Transform( m_averageVertexPosition, m_dynamics.transformation() );
}

//    return XMVector4Transform( m_edgeNormalDirectionUnitVector[ index ], m_dynamics.transformation() );

XMVECTOR Shape::centerOfMassWorldPosition() const
{
    return XMVector4Transform( m_centerOfMassLocalPosition, m_dynamics.transformation() );
}

XMVECTOR Shape::transformedVertexPosition( const int index ) const
{
    assert( index < numberOfVertices() );
    return XMVector4Transform( m_vertexPositionVector[ index ], m_dynamics.transformation() );
}

XMVECTOR Shape::transformedEdgeDirection( const int index ) const
{
    assert( index < numberOfEdges() );
    return XMVector4Transform( m_edgeDirectionVector[ index ], m_dynamics.transformation() );
}

XMVECTOR Shape::transformedEdgeDirectionUnit( const int index ) const
{
    assert( index < numberOfEdges() );
    return XMVector4Transform( m_edgeDirectionUnitVector[ index ], m_dynamics.transformation() );
}

XMVECTOR Shape::transformedEdgeNormalDirectionUnit( const int index ) const
{
    assert( index < numberOfEdges() );
    return XMVector4Transform( m_edgeNormalDirectionUnitVector[ index ], m_dynamics.transformation() );
}

float shapeSignedDistanceFromPlaneToPoint(
    XMVECTOR const & planePosition,
    XMVECTOR const & planeUnitNormal,
    XMVECTOR const & point )
{
    XMVECTOR const planePositionToPointDirection =  point - planePosition;
    return XMVectorGetX( XMVector4Dot( planeUnitNormal, planePositionToPointDirection ) );
}

bool shapeBHasSeperatingEdge( Shape const & shapeA, Shape const & shapeB )
{	
    const int shapeANumberOfVertices = shapeA.numberOfVertices();
    const int shapeBNumberOfEdges = shapeB.numberOfEdges();

    int shapeBEdgeIndex = 0;
    while( shapeBEdgeIndex != shapeBNumberOfEdges )
    {
        XMVECTOR const shapeBEdgePosition = shapeB.transformedVertexPosition( shapeBEdgeIndex );
        XMVECTOR const shapeBEdgeNormalDirectionUnit = shapeB.transformedEdgeNormalDirectionUnit( shapeBEdgeIndex );

        int shapeAVertexIndex = 0;
        while( shapeAVertexIndex != shapeANumberOfVertices )
        {		
            XMVECTOR const shapeAVertexPosition = shapeA.transformedVertexPosition( shapeAVertexIndex );
            
            float const perpDot = shapeSignedDistanceFromPlaneToPoint( shapeBEdgePosition, shapeBEdgeNormalDirectionUnit, shapeAVertexPosition );

            if( perpDot > 0.0f ) 
            {
                ++shapeAVertexIndex;
                
                if( shapeAVertexIndex == shapeANumberOfVertices )
                {
                    // found an edge that all a's vertices are on the correct side of so we must not be colliding.
                    return true;
                }
            }
            else
            {
                // We've found a vertex that's on the wrong side of this edge.
                // That doesn't mean we're definatley intersecting but it does mean we don't need to test this edge anymore.
                // Move on to the next edge by breaking out of the a vertex loop
                shapeAVertexIndex = shapeANumberOfVertices;
            }			
        }

        ++shapeBEdgeIndex;
    }
    
    return false;
}

bool shapeCollision( Shape const & shapeA, Shape const & shapeB )
{	
    if( shapeBHasSeperatingEdge( shapeA, shapeB ) )
    {
        return false;
    }
        
    if( shapeBHasSeperatingEdge( shapeB, shapeA ) )
    {
        return false;
    }

    return true;
}

bool shapeIntersect( 
    Shape const & shape, 
    XMVECTOR const & orig, 
    XMVECTOR const & dir,
    float & t, float & u, float & v )
{
    const int numberOfVertices = shape.numberOfVertices();
    int vertexIndex = 0;
    while( vertexIndex != numberOfVertices )
    {
        int nextVertexIndex = vertexIndex + 1;
        if( nextVertexIndex == numberOfVertices )
        {
            nextVertexIndex = 0;
        }		

        // Find vectors for two edges sharing vert0
        XMVECTOR const edge1 = shape.transformedVertexPosition( vertexIndex ) - shape.transformedAverageVertexPosition();
        XMVECTOR const edge2 = shape.transformedVertexPosition( nextVertexIndex ) - shape.transformedAverageVertexPosition();
        //XMVECTOR const edge1 = shape.transformedVertexPosition( vertexIndex ) - shape.centerOfMassWorldPosition();
        //XMVECTOR const edge2 = shape.transformedVertexPosition( nextVertexIndex ) - shape.centerOfMassWorldPosition();

        // Begin calculating determinant - also used to calculate U parameter
        const XMVECTOR pvec = XMVector4Cross( dir, edge2, Misc::origin() );
        //D3DXVec4Cross( &pvec, &dir, &edge2, &Misc::origin() );
        //D3DXVec3Cross( ( D3DXVECTOR3 * )&pvec, ( D3DXVECTOR3 const * )&dir, ( D3DXVECTOR3 const * )&edge2 );

        // If determinant is near zero, ray lies in plane of triangle
        float det = XMVectorGetX( XMVector4Dot( edge1, pvec ) );
        //float det = D3DXVec3Dot( ( D3DXVECTOR3 const * )&edge1, ( D3DXVECTOR3 const * )&pvec );

        XMVECTOR tvec;
        if( det > 0.0f )
        {
            tvec = orig - shape.transformedAverageVertexPosition();
            //tvec = orig - shape.centerOfMassWorldPosition();
        }
        else
        {
            tvec = shape.transformedAverageVertexPosition() - orig;
            //tvec = shape.centerOfMassWorldPosition() - orig;
            det = -det;
        }

        if( det > 0.0001f )
        {
            // Calculate U parameter and test bounds
            u = XMVectorGetX( XMVector4Dot( tvec, pvec ) );
            //u = D3DXVec3Dot( ( D3DXVECTOR3 const * )&tvec, ( D3DXVECTOR3 const * )&pvec );

            if( ( u >= 0.0f ) && ( u <= det ) )
            {
                // Prepare to test V parameter
                const XMVECTOR qvec = XMVector4Cross( tvec, edge1, Misc::origin() );
                //D3DXVec4Cross( &qvec, &tvec, &edge1, &Misc::origin() );
                //D3DXVec3Cross( ( D3DXVECTOR3 * )&qvec, ( D3DXVECTOR3 const * )&tvec, ( D3DXVECTOR3 const * )&edge1 );

                // Calculate V parameter and test bounds
                v = XMVectorGetX( XMVector4Dot( dir, qvec ) );
                //v = D3DXVec3Dot( ( D3DXVECTOR3 const * )&dir, ( D3DXVECTOR3 const * )&qvec );
                if( ( v >= 0.0f ) && ( ( u + v ) <= det ) )
                {
                    // Calculate t, scale parameters, ray intersects triangle
                    t = XMVectorGetX( XMVector4Dot( edge2, qvec ) );
                    //t = D3DXVec3Dot( ( D3DXVECTOR3 const * )&edge2, ( D3DXVECTOR3 const * )&qvec );
                    float const fInvDet = 1.0f / det;
                    t *= fInvDet;
                    u *= fInvDet;
                    v *= fInvDet;

                    return true;
                }
            }
        }

        ++vertexIndex;
    }

    return false;
}

void shapeClosestFeatures(						  
    float & smallestDistanceSquaredSoFar,
    Shape const & shapeA, FeatureClass & shapeAFeature, //float & shapeADistanceAlongEdge,
    Shape const & shapeB, FeatureClass & shapeBFeature, float & shapeBDistanceAlongEdge )
{
    const int shapeANumberOfVertices = shapeA.numberOfVertices();
    for( int shapeAVertexIndex = 0; shapeAVertexIndex != shapeANumberOfVertices; ++shapeAVertexIndex )
    {
        XMVECTOR const shapeAVertexPosition = shapeA.transformedVertexPosition( shapeAVertexIndex );
        
        const int shapeBNumberOfEdges = shapeB.numberOfEdges();
        const int shapeBNumberOfVertices = shapeB.numberOfVertices();
        for( int shapeBEdgeIndex = 0; shapeBEdgeIndex != shapeBNumberOfEdges; ++shapeBEdgeIndex )
        {
            XMVECTOR const shapeBEdgePosition = shapeB.transformedVertexPosition( shapeBEdgeIndex );
            XMVECTOR const shapeBEdgeUnitDirection = shapeB.transformedEdgeDirectionUnit( shapeBEdgeIndex );
            float const shapeBEdgeLength = shapeB.edgeLength( shapeBEdgeIndex );
            
            float const positionDotLineDirection = XMVectorGetX( XMVector4Dot( shapeAVertexPosition, shapeBEdgeUnitDirection ) );
            float const lineStartPositionDotLineDirection = XMVectorGetX( XMVector4Dot( shapeBEdgePosition, shapeBEdgeUnitDirection ) );

            
            float distanceAlongLine = positionDotLineDirection - lineStartPositionDotLineDirection;
            FeatureClass shapeFeature( EdgeFeature, shapeBEdgeIndex );
            if( distanceAlongLine <= 0.0f )
            {
                shapeFeature = FeatureClass( VertexFeature, shapeBEdgeIndex );
                distanceAlongLine = 0.0f;
            }
            else if( distanceAlongLine >= shapeBEdgeLength )
            {
                shapeFeature = FeatureClass( VertexFeature, shapeBEdgeIndex + 1 );
                if( shapeFeature.index() == shapeBNumberOfVertices )
                {
                    shapeFeature = FeatureClass( VertexFeature, 0 );
                }
                distanceAlongLine = shapeBEdgeLength;
            }	
            //return clamp( positionDotLineDirection - lineStartPositionDotLineDirection, 0.0f, lineLength );
            //float const distanceAlongLine = closestDistanceAlongLine( shapeBEdgePosition, shapeBEdgeUnitDirection, shapeBEdgeLength, shapeAVertexPosition );

            XMVECTOR const shapeBClosestPosition = shapeBEdgePosition + ( shapeBEdgeUnitDirection * distanceAlongLine );
            XMVECTOR const toShapeBClosestDirection = shapeBClosestPosition - shapeAVertexPosition;
            float const toShapeBClosestDistanceSquared = XMVectorGetX( XMVector4LengthSq( toShapeBClosestDirection ) );
            //float const toShapeBClosestDistanceSquared = D3DXVec4Dot( &toShapeBClosestDirection, &toShapeBClosestDirection );
            
            if( toShapeBClosestDistanceSquared < smallestDistanceSquaredSoFar )
            {
                smallestDistanceSquaredSoFar = toShapeBClosestDistanceSquared;
                shapeAFeature = FeatureClass( VertexFeature, shapeAVertexIndex );
                shapeBFeature = shapeFeature; // FeatureClass( EdgeFeature, shapeBEdgeIndex );
                shapeBDistanceAlongEdge = distanceAlongLine;
            }
        }		
    }
}

void shapeClosestFeatures(				  
    float & smallestDistanceSquaredSoFar,
    Shape const & shapeA, FeatureClass & shapeAFeature, float & shapeADistanceAlongEdge,
    Shape const & shapeB, FeatureClass & shapeBFeature, float & shapeBDistanceAlongEdge )
{
    smallestDistanceSquaredSoFar = FLT_MAX;
    shapeAFeature = FeatureClass();
    shapeBFeature = FeatureClass();
    
    shapeClosestFeatures( smallestDistanceSquaredSoFar, shapeA, shapeAFeature, shapeB, shapeBFeature, shapeBDistanceAlongEdge );
    shapeClosestFeatures( smallestDistanceSquaredSoFar, shapeB, shapeBFeature, shapeA, shapeAFeature, shapeADistanceAlongEdge );
}

XMVECTOR shapeVelocityAtPoint( Shape const & shape, XMVECTOR const & pointPos )
{
    XMVECTOR const COMPos = shape.centerOfMassWorldPosition();
    XMVECTOR const radiusDir = pointPos - COMPos;
    
    const XMVECTOR pointVelDir{ -XMVectorGetY( radiusDir ), XMVectorGetX( radiusDir ), XMVectorGetZ( radiusDir ), XMVectorGetW( radiusDir ) };
    //XMVECTOR const pointVelDir( -radiusDir.y, radiusDir.x, radiusDir.z, radiusDir.w );

    return shape.dynamics().velocity() + ( pointVelDir * shape.dynamics().angularVelocity() );
}

XMVECTOR shapeVertexVelocity( Shape const & shape, int const vertIndex )
{
    assert( ( vertIndex >= 0 ) && ( vertIndex < shape.numberOfVertices() ) );
    XMVECTOR const vertPos = shape.transformedVertexPosition( vertIndex );

    return shapeVelocityAtPoint( shape, vertPos );
}

} // namespace Physics
