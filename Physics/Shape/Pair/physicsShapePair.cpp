// Andrew Davies

#include "pch.h"
//#include "Physics/Shape/Pair/physicsShapePair.h"
//#include "Physics/Shape/physicsShape.h"
//#include "Physics/Shape/VClip/physicsShapeVCLip.h"
//
//namespace Physics
//{
//
//ShapePairClass::ShapePairClass(		
//	ShapeClass & shapeA,
//	FeatureClass const & shapeAFeature,
//	ShapeClass & shapeB,
//	FeatureClass const & shapeBFeature )
//	: m_shapeAPtr( &shapeA )
//	, m_shapeAFeature( shapeAFeature )
//	, m_shapeBPtr( &shapeB )
//	, m_shapeBFeature( shapeBFeature )
//{
//}
//
//
//void ShapePairClass::step( )
//{
//	if( shapeVClip( *m_shapeAPtr, m_shapeAFeature, *m_shapeBPtr, m_shapeBFeature ) )
//	{
//		//m_shapeAPtr->setDynamicsToPrevious( );
//		//m_shapeBPtr->setDynamicsToPrevious( );
//
//		ShapeClass * shapeAPtr = 0;
//		ShapeClass * shapeBPtr = 0;
//		XMVECTOR collisionPosition( 0.0f, 0.0f, 0.0f, 1.0f );
//		XMVECTOR collisionUnitNormal( 0.0f, 0.0f, 1.0f, 0.0f );
//
//		if( m_shapeAFeature.type( ) == VertexFeature )
//		{
//			shapeAPtr = m_shapeAPtr;
//			shapeBPtr = m_shapeBPtr;
//			collisionPosition = m_shapeAPtr->transformedVertexPosition( m_shapeAFeature.index( ) );
//			collisionUnitNormal = m_shapeBPtr->transformedEdgeNormalDirectionUnit( m_shapeBFeature.index( ) );
//		}
//		else if( m_shapeBFeature.type( ) == VertexFeature )
//		{
//			shapeAPtr = m_shapeBPtr;
//			shapeBPtr = m_shapeAPtr;
//			collisionPosition = m_shapeBPtr->transformedVertexPosition( m_shapeBFeature.index( ) );
//			collisionUnitNormal = m_shapeAPtr->transformedEdgeNormalDirectionUnit( m_shapeAFeature.index( ) );
//		}		
//		else
//		{
//			// Edge edge whitnesses.
//
//			shapeAPtr = m_shapeAPtr;
//			shapeBPtr = m_shapeBPtr;
//			collisionPosition = 
//				m_shapeAPtr->transformedVertexPosition( m_shapeAFeature.index( ) ) +
//				( m_shapeAPtr->transformedEdgeDirection( m_shapeAFeature.index( ) ) * 0.5f );
//			collisionUnitNormal = m_shapeBPtr->transformedEdgeNormalDirectionUnit( m_shapeBFeature.index( ) );
//		}
//
//		XMVECTOR const radiusA = collisionPosition - shapeAPtr->centerOfMassWorldPosition( );
//		XMVECTOR const radiusB = collisionPosition - shapeBPtr->centerOfMassWorldPosition( );
//			
//		XMVECTOR const radiusTangentA( -radiusA.y, radiusA.x, radiusA.z, radiusA.w );
//		XMVECTOR const radiusTangentB( -radiusB.y, radiusB.x, radiusB.z, radiusB.w );
//
//		XMVECTOR const velAP = shapeAPtr->dynamics( ).velocity( ) + ( radiusTangentA * shapeAPtr->dynamics( ).angularVelocity( ) );
//		XMVECTOR const velBP = shapeBPtr->dynamics( ).velocity( ) + ( radiusTangentB * shapeBPtr->dynamics( ).angularVelocity( ) );
//		XMVECTOR const relVel = velAP - velBP;
//
//		float const relVelDotNormal = D3DXVec4Dot( &relVel, &collisionUnitNormal );
//		float const reciprocalMassSum = ( 1.0f / shapeAPtr->mass( ) ) + ( 1.0f / shapeBPtr->mass( ) );	
//
//		float const radiusTangentADotNormal = D3DXVec4Dot( &radiusTangentA, &collisionUnitNormal );
//		float const radiusTangentBDotNormal = D3DXVec4Dot( &radiusTangentB, &collisionUnitNormal );
//		
//		float const reciprocalInertiaSum = 
//			( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia( ) ) + 
//			( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia( ) );
//
//		float const e = 0.8f;
//		float const j = ( -( 1.0f + e ) * relVelDotNormal ) / ( reciprocalMassSum + reciprocalInertiaSum );
//		
//		XMVECTOR const velA2 = shapeAPtr->dynamics( ).velocity( ) + ( ( j / shapeAPtr->mass( ) ) * collisionUnitNormal );
//		XMVECTOR const velB2 = shapeBPtr->dynamics( ).velocity( ) - ( ( j / shapeBPtr->mass( ) ) * collisionUnitNormal );
//
//		float const angVelA2 = shapeAPtr->dynamics( ).angularVelocity( ) + ( ( j * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia( ) );
//		float const angVelB2 = shapeBPtr->dynamics( ).angularVelocity( ) - ( ( j * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia( ) );
//
//		shapeAPtr->setDynamics(
//			shapeAPtr->dynamics( ).position( ),
//			shapeAPtr->dynamics( ).orientation( ),
//			velA2,
//			angVelA2 );
//		
//		shapeBPtr->setDynamics(
//			shapeBPtr->dynamics( ).position( ),
//			shapeBPtr->dynamics( ).orientation( ),
//			velB2,
//			angVelB2 );
//	}
//}
//
//
//}
