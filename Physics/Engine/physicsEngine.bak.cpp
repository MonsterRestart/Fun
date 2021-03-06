// Andrew Davies

#include "DXUT.h"
#include "Physics/Engine/physicsEngine.h"
#include "Physics/Feature/physicsFeature.h"
#include <fuz/container/fixed_list.hpp>
#include <fuz/container/fixed_vector.hpp>
#include <algorithm>
#include "misc.h"

#include "DevGraphics/devGraphics.h"

namespace Physics
{

// Utility class to be used with stl find and the like, generally, to match a UID and a shape.
class EngineShapeIdentifyClass
{
private:
	int m_UID;

public:
	EngineShapeIdentifyClass( int const UID )
		: m_UID( UID )
	{
	}

	bool operator()( ShapeClass const & shape ) const
	{
		return shape.UID() == m_UID;
	}
};


EngineClass::EngineClass()
	: m_nextUID( 0 )
	, m_stepCount( 0 )
{
}

void EngineClass::create()
{
	m_nextUID = 0;
	m_shapeList.clear();
	m_stepCount = 0;
}

void EngineClass::destroy()
{
	m_nextUID = 0;
	assert( m_shapeList.empty() );
	m_shapeList.clear();
	m_stepCount = 0;
}

ShapeClass * shapeInCollision(	
	ShapeClass const & shape,
	EngineShapeListType & engineShapeList )
{
	for( EngineShapeListType::iterator shapeItr = engineShapeList.begin(); shapeItr != engineShapeList.end(); ++shapeItr )
	{
		ShapeClass & otherShape = *shapeItr;

		if( &shape != &otherShape )
		{
			if( shapeCollision( shape, otherShape ) )
			{
				return &otherShape;
			}
		}
	}

	return 0;
}

//#define PHYSICS_ENGINE_MAX_CONTACTS ( ( int )4 )
#define PHYSICS_ENGINE_MAX_CONTACTS ( ( int )32 )

struct ContactStruct
{
	ContactStruct(
		ShapeClass & vertShape,
		const int vertIdx,
		ShapeClass & edgeShape,
		const int edgeIdx )
		: m_vertShapePtr( &vertShape )
		, m_vertIdx( vertIdx )
		, m_edgeShapePtr( &edgeShape )
		, m_edgeIdx( edgeIdx )
	{
	}

	ShapeClass * m_vertShapePtr;
	int m_vertIdx;
	ShapeClass * m_edgeShapePtr;
	int m_edgeIdx;
};

typedef fuz::fixed_vector< ContactStruct, PHYSICS_ENGINE_MAX_CONTACTS > ContactVectorType;
	
#define PHYSICS_ENGINE_TOLLERENCE ( 0.0001f )
//#define PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ( 0.1f )

#define PHYSICS_ENGINE_SOLVE_MAX_SIZE ( PHYSICS_ENGINE_MAX_CONTACTS )
#define PHYSICS_ENGINE_SOLVE_A_NUM_ROWS ( PHYSICS_ENGINE_SOLVE_MAX_SIZE )
#define PHYSICS_ENGINE_SOLVE_A_NUM_COLS ( PHYSICS_ENGINE_SOLVE_MAX_SIZE + 1 )
//void solve( const float** m, const float* v, const int size, float *const result )
void solve( 
	const float m[ PHYSICS_ENGINE_SOLVE_MAX_SIZE ][ PHYSICS_ENGINE_SOLVE_MAX_SIZE ], 
	const float v[ PHYSICS_ENGINE_SOLVE_MAX_SIZE ], 
	const int size, 
	float result[ PHYSICS_ENGINE_SOLVE_MAX_SIZE ] )
{	
	// http://marekrychlik.com/cgi-bin/gauss.cgi
	//assert( size > 0 );
	assert( size <= PHYSICS_ENGINE_SOLVE_MAX_SIZE );

	static float a[ PHYSICS_ENGINE_SOLVE_A_NUM_ROWS ][ PHYSICS_ENGINE_SOLVE_A_NUM_COLS ];
	ZeroMemory( a, sizeof( float ) * PHYSICS_ENGINE_SOLVE_A_NUM_ROWS * PHYSICS_ENGINE_SOLVE_A_NUM_COLS );	
	
	ZeroMemory( result, sizeof( float ) * size );

	// copy input matrix and vector into local augmented workspace matrix
	for( int rowIdx = 0; rowIdx != size; ++rowIdx )
	{
		memcpy( a[ rowIdx ], m[ rowIdx ], sizeof( float ) * size );
		//memcpy( a[ rowIdx ], m + ( rowIdx * size ), sizeof( float ) * size );
		a[ rowIdx ][ size ] = v[ rowIdx ];
	}

	for( int colIdx = 0; colIdx != size; ++colIdx )
	{
		// To improve numerical stability, find the largest value in this
		// column from the pivot point down (can't use above the pivot point 
		// because this would disrupt the nice triangular '1's and '0's we've
		// calculated already.
		int maxRowIdx = colIdx;
		for( int rowIdx = colIdx + 1; rowIdx != size; ++rowIdx )
		{
			if( fabs( a[ rowIdx ][ colIdx ] ) > fabs( a[ maxRowIdx ][ colIdx ] ) )
			{
				maxRowIdx = rowIdx;
			}
		}

		// Of course, swapping in the maximum pivot is not essesntial and could 
		// be one of the steps we don't need to do when time becomes a factor.

		// if the max value found is 0 then we don't need to do any thing else in the column.
		if( fabs( a[ maxRowIdx ][ colIdx ] ) > FLT_EPSILON )
		{
			// Remember we can use 'colIdx' for 'rowIdx' here.
			const int rowIdx = colIdx;

			if( maxRowIdx != colIdx )
			{
				// swap the rows, don't forget the augmented column.
				static float temp[ PHYSICS_ENGINE_SOLVE_A_NUM_COLS ];
				memcpy( temp, a[ maxRowIdx ], sizeof( float ) * ( size + 1 ) );				
				memcpy( a[ maxRowIdx ], a[ rowIdx ], sizeof( float ) * ( size + 1 ) );		
				memcpy( a[ rowIdx ], temp, sizeof( float ) * ( size + 1 ) );
			}

			const float reciprocalAPivot = 1.0f / a[ rowIdx ][ colIdx ];

			for( int divColIdx = 0; divColIdx != ( size + 1 ); ++divColIdx )
			{
				a[ rowIdx ][ divColIdx ] *= reciprocalAPivot;
			}
			a[ rowIdx ][ colIdx ] = 1.0f;
			
			for( int subRowIdx = ( rowIdx + 1 ); subRowIdx != size; ++subRowIdx )
			{
				const float multiplier = a[ subRowIdx ][ colIdx ];

				for( int subColIdx = 0; subColIdx != ( size + 1 ); ++subColIdx )
				{
					a[ subRowIdx ][ subColIdx ] -= ( multiplier * a[ rowIdx ][ subColIdx ] );
				}
			}
		}
	}

	// From the last row back, use the triangular matrix and substitution to populate the result array.
	for( int rowIdx = size - 1; rowIdx != -1; --rowIdx )
	{
		result[ rowIdx ] = a[ rowIdx ][ size ];
	
		for( int colIdx = size - 1; colIdx != rowIdx; --colIdx )
		{
			result[ rowIdx ] -= ( a[ rowIdx ][ colIdx ] * result[ colIdx ] );
		}
	}
}

void matrixMultiplyVector( 
	const float m[ PHYSICS_ENGINE_SOLVE_MAX_SIZE ][ PHYSICS_ENGINE_SOLVE_MAX_SIZE ], 
	const float v[ PHYSICS_ENGINE_SOLVE_MAX_SIZE ], 
	const int size, 
	float result[ PHYSICS_ENGINE_SOLVE_MAX_SIZE ] )
{
	for( int i = 0; i != size; ++i )
	{
		result[ i ] = 0.0f;
		for( int j = 0; j != size; ++j )
		{
			result[ i ] += m[ j ][ i ] * v[ j ];
			//result[ i ] += m[ i ][ j ] * v[ j ];
		}
	}
}

void EngineClass::step( float const deltaTime )
{
	static EngineShapeListType advancedShapeList;
			
	float advancedDeltaTime = 0.0f;
	float deltaTimeRemaining = deltaTime;

	int numLoops = 0;
	do
	{
		++numLoops;

		// Copy current state at time = timeAdvance because we're (possibly repeatedly) trying 
		// to move on from here and may need to revert here if we move into penetration.
		advancedShapeList = m_shapeList;
		
		// Advance all shapes on from where ever they've been left (end of last frame or last collision resolution)
		for( EngineShapeListType::iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
		{
			ShapeClass & shape = *shapeItr;
			shape.stepDynamics( deltaTimeRemaining );
		}

		// See if there are any penetrations
		static ContactVectorType contactVector;
		contactVector.clear();
		for( EngineShapeListType::iterator vertShapeItr = m_shapeList.begin(); vertShapeItr != m_shapeList.end(); ++vertShapeItr )
		{
			ShapeClass& vertShape = *vertShapeItr;
	
			for( EngineShapeListType::iterator edgeShapeItr = m_shapeList.begin(); edgeShapeItr != m_shapeList.end(); ++edgeShapeItr )
			{
				ShapeClass& edgeShape = *edgeShapeItr;

				if( vertShapeItr != edgeShapeItr )
				{
					const int numVerts = vertShape.numberOfVertices();
					const int numEdges = edgeShape.numberOfEdges();

					for( int vertIdx = 0; vertIdx != numVerts; ++vertIdx )
					{	
						const D3DXVECTOR4 vertPos = vertShape.transformedVertexPosition( vertIdx );	

						bool inside = true;
						int witnessEdgeIdx = numEdges;	
						float witnessDist = -FLT_MAX;
						for( int edgeIdx = 0; ( edgeIdx != numEdges ) && inside; ++edgeIdx )
						{							
							const D3DXVECTOR4 edgePos = edgeShape.transformedVertexPosition( edgeIdx );
							const D3DXVECTOR4 edgeToVertDir = vertPos - edgePos;
							const D3DXVECTOR4 edgeUnitNrml = edgeShape.transformedEdgeNormalDirectionUnit( edgeIdx );
							const float signedDist = D3DXVec4Dot( &edgeUnitNrml, &edgeToVertDir );
								
							if( signedDist <= 0.0f )
							{
								if( signedDist > witnessDist )
								{
									witnessEdgeIdx = edgeIdx;
									witnessDist = signedDist;
								}
							}
							else
							{
								// This vertex is on the outside of this edge so it can't be inside.
								inside = false;
							}
						}

						if( inside )
						{
							if( !contactVector.full() )
							{
								contactVector.push_back( ContactStruct( vertShape, vertIdx, edgeShape, witnessEdgeIdx ) );
							}
						}
					}
				}
			}
		}
		
		const int numContacts = ( int )contactVector.size();
		
		for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
		{	
			const ContactStruct& contact = contactVector[ contactIdx ];
			const D3DXVECTOR4 contactPos = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );
			DevGraphics::point( ( ( int )contact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + contact.m_vertIdx, contactPos );
		}

		if( numContacts > 0 )
		{
			// Gone too far into contact
			// roll back 
			m_shapeList = advancedShapeList;

			// See if the last step we tried was very small. if so we're right at Time of collision.
			if( deltaTimeRemaining <= PHYSICS_ENGINE_TOLLERENCE )
			{
				//handleCollisions(collisions);
				
				//static ContactVectorType restingContactVector;
				//restingContactVector.clear();

				for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
				{	
					ContactStruct& contact = contactVector[ contactIdx ];
																			
					ShapeClass* const shapeAPtr = contact.m_vertShapePtr;
					ShapeClass* const shapeBPtr = contact.m_edgeShapePtr;
					const D3DXVECTOR4 collisionPosition = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );
					const D3DXVECTOR4 collisionUnitNormal = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );

					D3DXVECTOR4 const radiusA = collisionPosition - shapeAPtr->centerOfMassWorldPosition();
					D3DXVECTOR4 const radiusB = collisionPosition - shapeBPtr->centerOfMassWorldPosition();
					
					D3DXVECTOR4 const radiusTangentA( -radiusA.y, radiusA.x, radiusA.z, radiusA.w );
					D3DXVECTOR4 const radiusTangentB( -radiusB.y, radiusB.x, radiusB.z, radiusB.w );

					D3DXVECTOR4 const velAP = shapeAPtr->dynamics().velocity() + ( radiusTangentA * shapeAPtr->dynamics().angularVelocity() );
					D3DXVECTOR4 const velBP = shapeBPtr->dynamics().velocity() + ( radiusTangentB * shapeBPtr->dynamics().angularVelocity() );
					D3DXVECTOR4 const relVel = velAP - velBP;

					float const relVelDotNormal = D3DXVec4Dot( &relVel, &collisionUnitNormal );

					//static float RESTING_SPEED = 0.001f;
					//if( relVelDotNormal < RESTING_SPEED )
					{
						float const radiusTangentADotNormal = D3DXVec4Dot( &radiusTangentA, &collisionUnitNormal );
						float const radiusTangentBDotNormal = D3DXVec4Dot( &radiusTangentB, &collisionUnitNormal );							

						float reciprocalMassSum = 0.0f;		
						float reciprocalInirtiaSum = 0.0f;
						if( !shapeAPtr->infiniteMass() )
						{
							assert( shapeAPtr->mass() > FLT_EPSILON );
							assert( shapeAPtr->mass() < FLT_MAX );

							reciprocalMassSum += ( 1.0f / shapeAPtr->mass() );
							reciprocalInirtiaSum += ( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
						}
						if( !shapeBPtr->infiniteMass() )
						{
							assert( shapeBPtr->mass() > FLT_EPSILON );
							assert( shapeBPtr->mass() < FLT_MAX );

							reciprocalMassSum += ( 1.0f / shapeBPtr->mass() );
							reciprocalInirtiaSum += ( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
						}

						float const e = 0.2f;
						static float minSpeed = 0.05f;

						const float jDivisor = ( reciprocalMassSum + reciprocalInirtiaSum );
						assert( fabs( jDivisor ) > FLT_EPSILON );
						float const j = ( -( 1.0f + e ) * relVelDotNormal ) / jDivisor;

						if( !shapeAPtr->infiniteMass() )
						{
							assert( shapeAPtr->mass() > FLT_EPSILON );
							assert( shapeAPtr->mass() < FLT_MAX );
								
							D3DXVECTOR4 velA2 = shapeAPtr->dynamics().velocity();
							velA2 += ( ( ( j / shapeAPtr->mass() ) + minSpeed ) * collisionUnitNormal );
						
							float const angVelA2 = shapeAPtr->dynamics().angularVelocity() + ( ( j * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );

							shapeAPtr->setDynamics(
								shapeAPtr->dynamics().position(),
								shapeAPtr->dynamics().orientation(),
								velA2,
								angVelA2 );
						}

						if( !shapeBPtr->infiniteMass() )
						{
							assert( shapeBPtr->mass() > FLT_EPSILON );
							assert( shapeBPtr->mass() < FLT_MAX );
								
							D3DXVECTOR4 velB2 = shapeBPtr->dynamics().velocity();
							velB2 -= ( ( ( j / shapeBPtr->mass() ) + minSpeed ) * collisionUnitNormal );
								
							float const angVelB2 = shapeBPtr->dynamics().angularVelocity() - ( ( j * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );

							shapeBPtr->setDynamics(
								shapeBPtr->dynamics().position(),
								shapeBPtr->dynamics().orientation(),
								velB2,
								angVelB2 );
						}
					}
					//else if( relVelDotNormal > RESTING_SPEED )
					//{

					//}
					//else // THIS IS A RESTING CONTACT 
					//{
					//	// add it to a list of resting contacts??? to be processed next??
					//	restingContactVector.push_back( contact );
					//}
				}

				// Process resting contacts???		
				//const int numRestingContacts = restingContactVector.size();		
				//for( int restingContactIdx = 0; restingContactIdx != numRestingContacts; ++restingContactIdx )
				//{	
				//	const ContactStruct& restingContact = restingContactVector[ restingContactIdx ];
				//	const D3DXVECTOR4 contactPos = restingContact.m_vertShapePtr->transformedVertexPosition( restingContact.m_vertIdx );
				//	DevGraphics::point(
				//		( ( int )restingContact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + restingContact.m_vertIdx + 9999,
				//		contactPos, 16.0f,
				//		D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
				//}



				
				// SET UP A MATRIX AND b VECTOR

				// We're aiming to calculate a force at each of the contact points that should 
				// be enough to keep the objects from interpenmatrating but not soo much that 
				// they move apart.

				// Here we populate a 2d matrix and a 	
				float A[ PHYSICS_ENGINE_MAX_CONTACTS ][ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
				float b[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };

				for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
				{	
					ContactStruct& iContact = contactVector[ contactIdx ];

					const D3DXVECTOR4 iContactPos = iContact.m_vertShapePtr->transformedVertexPosition( iContact.m_vertIdx );
					DevGraphics::point( ( ( int )iContact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + iContact.m_vertIdx, iContactPos ); 

					const D3DXVECTOR4 iContactVertShapeCOMPos = iContact.m_vertShapePtr->centerOfMassWorldPosition();
					//DevGraphics::point( 9923 + ( int )iContact.m_vertShapePtr, iContactVertShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
					const D3DXVECTOR4 iContactEdgeShapeCOMPos = iContact.m_edgeShapePtr->centerOfMassWorldPosition();
					//DevGraphics::point( 12355 + ( int )iContact.m_edgeShapePtr, iContactEdgeShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );
		
					const D3DXVECTOR4 iContactRadius1 = iContactPos - iContactVertShapeCOMPos;			
					//DevGraphics::line( 
					//	7267 + ( int )iContact.m_vertShapePtr, 
					//	iContactVertShapeCOMPos, 
					//	iContactVertShapeCOMPos + iContactRadius1, 
					//	D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
					//	D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
					const D3DXVECTOR4 iContactRadius2 = iContactPos - iContactEdgeShapeCOMPos;		
					//DevGraphics::line( 
					//	43322 + ( int )iContact.m_edgeShapePtr, 
					//	iContactEdgeShapeCOMPos, 
					//	iContactEdgeShapeCOMPos + iContactRadius2, 
					//	D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ),
					//	D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );

					const D3DXVECTOR4 iContactNorm = iContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( iContact.m_edgeIdx );		
					//DevGraphics::line( 55 + ( int )iContact.m_edgeShapePtr, iContactPos, iContactPos + ( iContactNorm * 10.0f ) );
					const D3DXVECTOR4 iContactNegNorm = -iContactNorm;		
	
					for( int jContactIdx = 0; jContactIdx != numContacts; ++jContactIdx )
					{	
						const ContactStruct& jContact = contactVector[ jContactIdx ];
			
						//ni ⋅ (nj / m1 + (rj × nj) × r1 / I1)

						const D3DXVECTOR4 jContactPos = jContact.m_vertShapePtr->transformedVertexPosition( jContact.m_vertIdx );		
						const D3DXVECTOR4 jContactNorm = jContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( jContact.m_edgeIdx );
						const D3DXVECTOR4 jContactNegNorm = -jContactNorm;

						if( !iContact.m_vertShapePtr->infiniteMass() )
						{
							if( iContact.m_vertShapePtr == jContact.m_vertShapePtr )
							{
								assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
								assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
								assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
								assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

								const D3DXVECTOR4 jRad = jContactPos - iContactVertShapeCOMPos;
					
								D3DXVECTOR4 jRadXJNorm;
								D3DXVec4Cross( &jRadXJNorm, &jRad, &jContactNorm, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );

								D3DXVECTOR4 temp = ( jContactNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );

								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &temp );
							}
			
							if( iContact.m_vertShapePtr == jContact.m_edgeShapePtr )
							{				
								assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
								assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
								assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
								assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

								const D3DXVECTOR4 jContactRadius = jContactPos - iContactVertShapeCOMPos;
					
								D3DXVECTOR4 jRadXJNorm;
								D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );

								D3DXVECTOR4 temp = ( jContactNegNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
					
								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &temp );
							}
						}

						if( !iContact.m_edgeShapePtr->infiniteMass() )
						{
							if( iContact.m_edgeShapePtr == jContact.m_vertShapePtr )
							{				
								assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
								assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
								assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
								assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

								const D3DXVECTOR4 jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
					
								D3DXVECTOR4 jRadXJNorm;
								D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNorm, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );

								D3DXVECTOR4 temp = ( jContactNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );

								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &temp );
							}
			
							if( iContact.m_edgeShapePtr == jContact.m_edgeShapePtr )
							{				
								assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
								assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
								assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
								assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

								const D3DXVECTOR4 jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
					
								D3DXVECTOR4 jRadXJNorm;
								D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );

								D3DXVECTOR4 temp = ( jContactNegNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );
					
								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &temp );
							}
						}
					}
		
					const D3DXVECTOR4 vertShapeAngVel( 0.0f, 0.0f, iContact.m_vertShapePtr->angularVelocity(), 0.0f );
					const D3DXVECTOR4 edgeShapeAngVel( 0.0f, 0.0f, iContact.m_edgeShapePtr->angularVelocity(), 0.0f );
		
					D3DXVECTOR4 vertShapeAngVelCrossRadius1;
					D3DXVec4Cross( &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &iContactRadius1, &Misc::origin() );

					D3DXVECTOR4 edgeShapeAngVelCrossRadius2;
					D3DXVec4Cross( &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &iContactRadius2, &Misc::origin() );
		
					D3DXVECTOR4 const temp =
						( iContact.m_vertShapePtr->velocity() + vertShapeAngVelCrossRadius1 ) - 
						( iContact.m_edgeShapePtr->velocity() + edgeShapeAngVelCrossRadius2 );					

					D3DXVECTOR4 edgeShapeAngVelCrossNorm;
					D3DXVec4Cross( &edgeShapeAngVelCrossNorm, &edgeShapeAngVel, &iContactNorm, &Misc::origin() );
		
					// Stricktly 2D
					b[ contactIdx ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
						 ( ( iContactNorm.x * temp.y ) - ( iContactNorm.y * temp.x ) );
					//b[ contactIdx ] += 2.0f * D3DXVec4Dot( &iContactNorm, &temp );
					//b[ contactIdx ] += 2.0f * D3DXVec4Dot( &edgeShapeAngVelCrossNorm, &temp );



					D3DXVECTOR4 vertShapeAcc = Misc::zero();
					D3DXVECTOR4 vertShapeAngAcc = Misc::zero();
					if( !iContact.m_vertShapePtr->infiniteMass() )
					{	
						assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
						assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
						assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
						assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );	

						vertShapeAcc = iContact.m_vertShapePtr->force() / iContact.m_vertShapePtr->mass();
						vertShapeAngAcc.z = iContact.m_vertShapePtr->torque() / iContact.m_vertShapePtr->momentOfInertia();
					}
		
					D3DXVECTOR4 edgeShapeAcc = Misc::zero();
					D3DXVECTOR4 edgeShapeAngAcc = Misc::zero();
					if( !iContact.m_edgeShapePtr->infiniteMass() )
					{
						assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
						assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
						assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
						assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

						edgeShapeAcc = iContact.m_edgeShapePtr->force() / iContact.m_edgeShapePtr->mass();
						edgeShapeAngAcc.z = iContact.m_edgeShapePtr->torque() / iContact.m_edgeShapePtr->momentOfInertia();
					}
		
					D3DXVECTOR4 vertShapeAngAccCrossRadius1;
					D3DXVec4Cross( &vertShapeAngAccCrossRadius1, &vertShapeAngAcc, &iContactRadius1, &Misc::origin() );

					D3DXVECTOR4 edgeShapeAngAccCrossRadius2;
					D3DXVec4Cross( &edgeShapeAngAccCrossRadius2, &edgeShapeAngAcc, &iContactRadius2, &Misc::origin() );
		
					D3DXVECTOR4 vertShapeAngVelCrossVertShapeAngVelCrossRadius1;
					D3DXVec4Cross( &vertShapeAngVelCrossVertShapeAngVelCrossRadius1,  &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &Misc::origin() );

					D3DXVECTOR4 edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2;
					D3DXVec4Cross( &edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2, &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &Misc::origin() );
		
					D3DXVECTOR4 const temp2 =
						( vertShapeAcc + vertShapeAngAccCrossRadius1 + vertShapeAngVelCrossVertShapeAngVelCrossRadius1 ) -
						( edgeShapeAcc + edgeShapeAngAccCrossRadius2 + edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2 );
		
					b[ contactIdx ] += D3DXVec4Dot( &iContactNorm, &temp2 );
				}

				// COMPUTE FORCES

				// Init all forces to zero.
				float f[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };	

				// a[] is speeds at each contact point.
				// Init a to be b
				float* a = b;
	
				float v[ PHYSICS_ENGINE_MAX_CONTACTS ];	
				float deltaF[ PHYSICS_ENGINE_MAX_CONTACTS ];	
				float deltaA[ PHYSICS_ENGINE_MAX_CONTACTS ];	

				// Spare spce to use during swap
				float swapTemp[ PHYSICS_ENGINE_MAX_CONTACTS ];	
	
				// k is the current pivot point.
				// Contact <=k are clamped with f[i]>0 and a[i]<=0. 	
				// Contact >k are unclamped with f[i]=0 and a[i]>0. 
				int k = 0;	
				//ContactListType::iterator kContactItr = contactList.begin();		
				// This is initialised one element at at time in the loop below
				int C[ PHYSICS_ENGINE_MAX_CONTACTS ];	


				// Iterate over all the contacts and drive a to zero if it is less
				// It should be fine to use a 'for()' here even though I'm modifying the list within 
				// because any modifications occur before contactItr therefore it's still alid to 
				// increment and compare against .end().
				for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
				{
					C[ contactIdx ] = contactIdx;

					// Keep looping and trying to drive b to zero
					bool drivenToZero = ( a[ contactIdx ] >= -FLT_EPSILON );
					while( drivenToZero == false ) // a[ contactIdx ] < -FLT_EPSILON )
					{
						ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );  // just zero contactList.size()			
			
						for( int innerContactIdx = 0; innerContactIdx != k; ++innerContactIdx )
						{	
							v[ innerContactIdx ] = -A[ innerContactIdx ][ contactIdx ];
						}

						ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						deltaF[ contactIdx ] = 1.0f;
			
						solve( A, v, contactIdx, deltaF );  // THIS SOLVER IS FINE. STOP DEBUGGING IT.
			
						ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						matrixMultiplyVector( A, deltaF, numContacts, deltaA );

						// Calculate maxstep.
						float s = FLT_MAX;
						int j = -1;
						if( deltaA[ contactIdx ] > FLT_EPSILON )
						{
							j = contactIdx;
							s = -a[ contactIdx ] / deltaA[ contactIdx ];
						}
						else
						{
							assert( false );
						}
			
						int i = 0;		
						for( ; i != k; ++i )
						{			
							if( deltaF[ i ] < -FLT_EPSILON )
							{
								const float posS = -f[ i ] / deltaF[ i ];
								if( posS < s )
								{
									s = posS;
									j = i;		
								}
							}
						}

						for( ; i != contactIdx; ++i )
						{
							if( deltaA[ i ] < -FLT_EPSILON )
							{
								const float posS = -a[ i ] / deltaA[ i ];
								if( posS < s )
								{
									s = posS;
									j = i;			
								}
							}
						}
			
						for( int e = 0; e != numContacts; ++e )
						{				
							f[ e ] += s * deltaF[ e ];
							a[ e ] += s * deltaA[ e ];
						}

						if( j < k ) 
						{	
							memcpy( swapTemp, A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
							const float aSwapTemp = a[ j ];
							const float fSwapTemp = f[ j ];
							const int CSwapTemp = C[ j ];
							for( int shuffleIdx = ( j + 1 ); shuffleIdx != k; ++shuffleIdx )
							{
								memcpy( A[ shuffleIdx - 1 ], A[ shuffleIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
								a[ shuffleIdx - 1 ] = a[ shuffleIdx ];
								f[ shuffleIdx - 1 ] = f[ shuffleIdx ];
								C[ shuffleIdx - 1 ] = C[ shuffleIdx ];
							}
							memcpy( A[ k - 1 ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
							a[ k - 1 ] = aSwapTemp;
							f[ k - 1 ] = fSwapTemp;
							C[ k - 1 ] = CSwapTemp;

							for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
							{
								const float ASwapTemp = A[ rowIdx ][ j ];
								for( int shuffleIdx = ( j + 1 ); shuffleIdx != k; ++shuffleIdx )
								{
									A[ rowIdx ][ shuffleIdx - 1 ] = A[ rowIdx ][ shuffleIdx ];
								}
								A[ rowIdx ][ k - 1 ] = ASwapTemp;
							}

							--k;
						}
						else if( j < contactIdx )
						{
							memcpy( swapTemp, A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
							const float aSwapTemp = a[ j ];
							const float fSwapTemp = f[ j ];
							const int CSwapTemp = C[ j ];
							for( int shuffleIdx = j; shuffleIdx != k; --shuffleIdx )
							{
								memcpy( A[ shuffleIdx ], A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
								a[ shuffleIdx ] = a[ shuffleIdx - 1 ];
								f[ shuffleIdx ] = f[ shuffleIdx - 1 ];
								C[ shuffleIdx ] = C[ shuffleIdx - 1 ];
							}
							memcpy( A[ k ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
							a[ k ] = aSwapTemp;
							f[ k ] = fSwapTemp;
							C[ k ] = CSwapTemp;

							for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
							{
								const float ASwapTemp = A[ rowIdx ][ j ];
								for( int shuffleIdx = j; shuffleIdx != k; --shuffleIdx )
								{
									A[ rowIdx ][ shuffleIdx ] = A[ rowIdx ][ shuffleIdx - 1 ];
								}
								A[ rowIdx ][ k ] = ASwapTemp;
							}

							++k;
						}
						else
						{
							a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
							drivenToZero = true;

							memcpy( swapTemp, A[ contactIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
							const float aSwapTemp = a[ contactIdx ];
							const float fSwapTemp = f[ contactIdx ];
							const int CSwapTemp = C[ contactIdx ];
							for( int shuffleIdx = contactIdx; shuffleIdx != k; --shuffleIdx )
							{
								memcpy( A[ shuffleIdx ], A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
								a[ shuffleIdx ] = a[ shuffleIdx - 1 ];
								f[ shuffleIdx ] = f[ shuffleIdx - 1 ];
								C[ shuffleIdx ] = C[ shuffleIdx - 1 ];
							}
							memcpy( A[ k ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
							a[ k ] = aSwapTemp;
							f[ k ] = fSwapTemp;
							C[ k ] = CSwapTemp;

							for( int rowIdx = 0; rowIdx != ( contactIdx + 1 ); ++rowIdx )
							{
								const float ASwapTemp = A[ rowIdx ][ contactIdx ];
								for( int shuffleIdx = contactIdx; shuffleIdx != k; --shuffleIdx )
								{
									A[ rowIdx ][ shuffleIdx ] = A[ rowIdx ][ shuffleIdx - 1 ];
								}
								A[ rowIdx ][ k ] = ASwapTemp;
							}

							++k;
						}
					}
				}


				// Apply/add contact forces.
				// Iterate over all contacts and use the force magnitudes calculated in f[] 
				// above and the geometry of the contact (position + normal) to add force 
				// and torque to both objects (except if the object is infinite mass).
				for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
				{
					const ContactStruct& contact = contactVector[ C[ contactIdx ] ];

					// The above calculates equal but opposite forces to be applied to both finite mass 
					// objects involved in the contact. The location of the foce is at the vertex of the 
					// 'vertex object', the direction is along the normal of the 'edge object', the 
					// magnitude is +/-f[ contactIdx ].
		
					const D3DXVECTOR4 contactPos = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );	
					const D3DXVECTOR4 contactNorm = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );	

					static float fakeMult = 1.0f;

					if( !contact.m_vertShapePtr->infiniteMass() )
					{
						contact.m_vertShapePtr->addForceActingAtWorldPosition( f[ contactIdx ] * fakeMult * contactNorm, contactPos );

						DevGraphics::line( 
							561 + ( int )contact.m_vertShapePtr + ( int )&contact,
							contactPos, 
							contactPos + ( f[ contactIdx ] * contactNorm * 0.02f ), 
							D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
							D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
					}

					if( !contact.m_edgeShapePtr->infiniteMass() )
					{
						contact.m_edgeShapePtr->addForceActingAtWorldPosition( -f[ contactIdx ] * fakeMult * contactNorm, contactPos );

						DevGraphics::line( 
							99812 + ( int )contact.m_edgeShapePtr + ( int )&contact,
							contactPos, 
							contactPos + ( -f[ contactIdx ] * contactNorm * 0.02f ), 
							D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
							D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
					}
				}

















				deltaTimeRemaining = deltaTime - advancedDeltaTime;  // try to step to end of time period timeStep
			}
			else
			{
				deltaTimeRemaining *= 0.5f;
			}
		}
		else 
		{
			advancedDeltaTime += deltaTimeRemaining;
			if( deltaTimeRemaining > PHYSICS_ENGINE_TOLLERENCE )
			{
				deltaTimeRemaining *= 0.5f;
			}
		}
	}
	while( ( advancedDeltaTime < deltaTime ) && ( numLoops < 32 ) );


	






#if 0
	ContactVectorType contactVector;
	for( EngineShapeListType::iterator vertShapeItr = m_shapeList.begin(); vertShapeItr != m_shapeList.end(); ++vertShapeItr )
	{
		ShapeClass & vertShape = *vertShapeItr;
	
		for( EngineShapeListType::iterator edgeShapeItr = m_shapeList.begin(); edgeShapeItr != m_shapeList.end(); ++edgeShapeItr )
		{
			ShapeClass & edgeShape = *edgeShapeItr;

			if( vertShapeItr != edgeShapeItr )
			{
				int const numVerts = vertShape.numberOfVertices();
				int const numEdges = edgeShape.numberOfEdges();

				for( int vertIdx = 0; vertIdx != numVerts; ++vertIdx )
				{	
					D3DXVECTOR4 const vertPos = vertShape.transformedVertexPosition( vertIdx );	
						
					for( int edgeIdx = 0; edgeIdx != numEdges; ++edgeIdx )
					{
						// Okay so here is the test. We're considering a vert and and edge. They are in contact 
						// if they are close enough together (within the bounds of the edge) and we should really 
						// only consider fetures that are moving or accelerating towards each other. But whart 
						// about leaving fast moving things to be resolved by the collision detectiona and response?

						D3DXVECTOR4 const edgePos = edgeShape.transformedVertexPosition( edgeIdx );
						D3DXVECTOR4 const edgeDirUnit = edgeShape.transformedEdgeDirectionUnit( edgeIdx );
						float const edgeLen = edgeShape.edgeLength( edgeIdx );
						D3DXVECTOR4 const edgeUnitNrml = edgeShape.transformedEdgeNormalDirectionUnit( edgeIdx );

						D3DXVECTOR4 const edgeToVertDir = vertPos - edgePos;

						float const edgeToVertDirDotEdgeDirUnit = D3DXVec4Dot( &edgeToVertDir, &edgeDirUnit );
						float const signedDist = D3DXVec4Dot( &edgeUnitNrml, &edgeToVertDir );
							
						if( ( edgeToVertDirDotEdgeDirUnit > 0.0f ) && 
							( edgeToVertDirDotEdgeDirUnit < edgeLen ) &&
							( signedDist <= PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ) &&
							( signedDist >= -PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ) )
						{
							// This vert and edge are close enough together to be in contact but we need to determine their 
							// relative velocity along the edge normal to determine if they're not moving apart anyway.

							D3DXVECTOR4 const edgeContactPos = edgePos + ( edgeDirUnit * edgeToVertDirDotEdgeDirUnit );
								
							D3DXVECTOR4 const vertVel = shapeVertexVelocity( vertShape, vertIdx );
							D3DXVECTOR4 const edgeContactPosVel = shapeVelocityAtPoint( edgeShape, edgeContactPos );

							D3DXVECTOR4 const relVel = vertVel - edgeContactPosVel;

							float const signedSpd = D3DXVec4Dot( &edgeUnitNrml, &relVel );

							if( signedSpd <= 0.01f )
							{
								if( !contactVector.full() )
								{
									contactVector.push_back( ContactStruct( vertShape, vertIdx, edgeShape, edgeIdx ) );
								}								
							}
						}
					}			
				}
			}
		}
	}

	const int numContacts = ( int )contactVector.size();	
	
	// SET UP A MATRIX AND b VECTOR

	// We're aiming to calculate a force at each of the contact points that should 
	// be enough to keep the objects from interpenmatrating but not soo much that 
	// they move apart.

	// Here we populate a 2d matrix and a 	
	float A[ PHYSICS_ENGINE_MAX_CONTACTS ][ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
	float b[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };

	for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
	{	
		ContactStruct& iContact = contactVector[ contactIdx ];

		const D3DXVECTOR4 iContactPos = iContact.m_vertShapePtr->transformedVertexPosition( iContact.m_vertIdx );
		DevGraphics::point( ( ( int )iContact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + iContact.m_vertIdx, iContactPos ); 

		const D3DXVECTOR4 iContactVertShapeCOMPos = iContact.m_vertShapePtr->centerOfMassWorldPosition();
		//DevGraphics::point( 9923 + ( int )iContact.m_vertShapePtr, iContactVertShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
		const D3DXVECTOR4 iContactEdgeShapeCOMPos = iContact.m_edgeShapePtr->centerOfMassWorldPosition();
		//DevGraphics::point( 12355 + ( int )iContact.m_edgeShapePtr, iContactEdgeShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );
		
		const D3DXVECTOR4 iContactRadius1 = iContactPos - iContactVertShapeCOMPos;			
		//DevGraphics::line( 
		//	7267 + ( int )iContact.m_vertShapePtr, 
		//	iContactVertShapeCOMPos, 
		//	iContactVertShapeCOMPos + iContactRadius1, 
		//	D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
		//	D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
		const D3DXVECTOR4 iContactRadius2 = iContactPos - iContactEdgeShapeCOMPos;		
		//DevGraphics::line( 
		//	43322 + ( int )iContact.m_edgeShapePtr, 
		//	iContactEdgeShapeCOMPos, 
		//	iContactEdgeShapeCOMPos + iContactRadius2, 
		//	D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ),
		//	D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );

		const D3DXVECTOR4 iContactNorm = iContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( iContact.m_edgeIdx );		
		//DevGraphics::line( 55 + ( int )iContact.m_edgeShapePtr, iContactPos, iContactPos + ( iContactNorm * 10.0f ) );
		const D3DXVECTOR4 iContactNegNorm = -iContactNorm;		
	
		for( int jContactIdx = 0; jContactIdx != numContacts; ++jContactIdx )
		{	
			const ContactStruct& jContact = contactVector[ jContactIdx ];
			
			//ni ⋅ (nj / m1 + (rj × nj) × r1 / I1)

			const D3DXVECTOR4 jContactPos = jContact.m_vertShapePtr->transformedVertexPosition( jContact.m_vertIdx );		
			const D3DXVECTOR4 jContactNorm = jContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( jContact.m_edgeIdx );
			const D3DXVECTOR4 jContactNegNorm = -jContactNorm;

			if( !iContact.m_vertShapePtr->infiniteMass() )
			{
				if( iContact.m_vertShapePtr == jContact.m_vertShapePtr )
				{
					assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
					assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
					assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
					assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

					const D3DXVECTOR4 jRad = jContactPos - iContactVertShapeCOMPos;
					
					D3DXVECTOR4 jRadXJNorm;
					D3DXVec4Cross( &jRadXJNorm, &jRad, &jContactNorm, &Misc::origin() );			
					
					D3DXVECTOR4 jRadXJNormXIRad1;
					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );

					D3DXVECTOR4 temp = ( jContactNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );

					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &temp );
				}
			
				if( iContact.m_vertShapePtr == jContact.m_edgeShapePtr )
				{				
					assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
					assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
					assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
					assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

					const D3DXVECTOR4 jContactRadius = jContactPos - iContactVertShapeCOMPos;
					
					D3DXVECTOR4 jRadXJNorm;
					D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );			
					
					D3DXVECTOR4 jRadXJNormXIRad1;
					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );

					D3DXVECTOR4 temp = ( jContactNegNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
					
					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &temp );
				}
			}

			if( !iContact.m_edgeShapePtr->infiniteMass() )
			{
				if( iContact.m_edgeShapePtr == jContact.m_vertShapePtr )
				{				
					assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
					assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
					assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
					assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

					const D3DXVECTOR4 jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
					
					D3DXVECTOR4 jRadXJNorm;
					D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNorm, &Misc::origin() );			
					
					D3DXVECTOR4 jRadXJNormXIRad1;
					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );

					D3DXVECTOR4 temp = ( jContactNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );

					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &temp );
				}
			
				if( iContact.m_edgeShapePtr == jContact.m_edgeShapePtr )
				{				
					assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
					assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
					assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
					assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

					const D3DXVECTOR4 jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
					
					D3DXVECTOR4 jRadXJNorm;
					D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );			
					
					D3DXVECTOR4 jRadXJNormXIRad1;
					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );

					D3DXVECTOR4 temp = ( jContactNegNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );
					
					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &temp );
				}
			}
		}
		
		const D3DXVECTOR4 vertShapeAngVel( 0.0f, 0.0f, iContact.m_vertShapePtr->angularVelocity(), 0.0f );
		const D3DXVECTOR4 edgeShapeAngVel( 0.0f, 0.0f, iContact.m_edgeShapePtr->angularVelocity(), 0.0f );
		
		D3DXVECTOR4 vertShapeAngVelCrossRadius1;
		D3DXVec4Cross( &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &iContactRadius1, &Misc::origin() );

		D3DXVECTOR4 edgeShapeAngVelCrossRadius2;
		D3DXVec4Cross( &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &iContactRadius2, &Misc::origin() );
		
		D3DXVECTOR4 const temp =
			( iContact.m_vertShapePtr->velocity() + vertShapeAngVelCrossRadius1 ) - 
			( iContact.m_edgeShapePtr->velocity() + edgeShapeAngVelCrossRadius2 );					

		D3DXVECTOR4 edgeShapeAngVelCrossNorm;
		D3DXVec4Cross( &edgeShapeAngVelCrossNorm, &edgeShapeAngVel, &iContactNorm, &Misc::origin() );
		
		// Stricktly 2D
		b[ contactIdx ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
			 ( ( iContactNorm.x * temp.y ) - ( iContactNorm.y * temp.x ) );
		//b[ contactIdx ] += 2.0f * D3DXVec4Dot( &iContactNorm, &temp );
		//b[ contactIdx ] += 2.0f * D3DXVec4Dot( &edgeShapeAngVelCrossNorm, &temp );



		D3DXVECTOR4 vertShapeAcc = Misc::zero();
		D3DXVECTOR4 vertShapeAngAcc = Misc::zero();
		if( !iContact.m_vertShapePtr->infiniteMass() )
		{	
			assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
			assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
			assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
			assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );	

			vertShapeAcc = iContact.m_vertShapePtr->force() / iContact.m_vertShapePtr->mass();
			vertShapeAngAcc.z = iContact.m_vertShapePtr->torque() / iContact.m_vertShapePtr->momentOfInertia();
		}
		
		D3DXVECTOR4 edgeShapeAcc = Misc::zero();
		D3DXVECTOR4 edgeShapeAngAcc = Misc::zero();
		if( !iContact.m_edgeShapePtr->infiniteMass() )
		{
			assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
			assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
			assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
			assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

			edgeShapeAcc = iContact.m_edgeShapePtr->force() / iContact.m_edgeShapePtr->mass();
			edgeShapeAngAcc.z = iContact.m_edgeShapePtr->torque() / iContact.m_edgeShapePtr->momentOfInertia();
		}
		
		D3DXVECTOR4 vertShapeAngAccCrossRadius1;
		D3DXVec4Cross( &vertShapeAngAccCrossRadius1, &vertShapeAngAcc, &iContactRadius1, &Misc::origin() );

		D3DXVECTOR4 edgeShapeAngAccCrossRadius2;
		D3DXVec4Cross( &edgeShapeAngAccCrossRadius2, &edgeShapeAngAcc, &iContactRadius2, &Misc::origin() );
		
		D3DXVECTOR4 vertShapeAngVelCrossVertShapeAngVelCrossRadius1;
		D3DXVec4Cross( &vertShapeAngVelCrossVertShapeAngVelCrossRadius1,  &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &Misc::origin() );

		D3DXVECTOR4 edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2;
		D3DXVec4Cross( &edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2, &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &Misc::origin() );
		
		D3DXVECTOR4 const temp2 =
			( vertShapeAcc + vertShapeAngAccCrossRadius1 + vertShapeAngVelCrossVertShapeAngVelCrossRadius1 ) -
			( edgeShapeAcc + edgeShapeAngAccCrossRadius2 + edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2 );
		
		b[ contactIdx ] += D3DXVec4Dot( &iContactNorm, &temp2 );
	}

	// COMPUTE FORCES

	// Init all forces to zero.
	float f[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };	

	// a[] is speeds at each contact point.
	// Init a to be b
	float* a = b;
	
	float v[ PHYSICS_ENGINE_MAX_CONTACTS ];	
	float deltaF[ PHYSICS_ENGINE_MAX_CONTACTS ];	
	float deltaA[ PHYSICS_ENGINE_MAX_CONTACTS ];	

	// Spare spce to use during swap
	float swapTemp[ PHYSICS_ENGINE_MAX_CONTACTS ];	
	
	// k is the current pivot point.
	// Contact <=k are clamped with f[i]>0 and a[i]<=0. 	
	// Contact >k are unclamped with f[i]=0 and a[i]>0. 
	int k = 0;	
	//ContactListType::iterator kContactItr = contactList.begin();		
	// This is initialised one element at at time in the loop below
	int C[ PHYSICS_ENGINE_MAX_CONTACTS ];	


	// Iterate over all the contacts and drive a to zero if it is less
	// It should be fine to use a 'for()' here even though I'm modifying the list within 
	// because any modifications occur before contactItr therefore it's still alid to 
	// increment and compare against .end().
	for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
	{
		C[ contactIdx ] = contactIdx;

		// Keep looping and trying to drive b to zero
		bool drivenToZero = ( a[ contactIdx ] >= -FLT_EPSILON );
		while( drivenToZero == false ) // a[ contactIdx ] < -FLT_EPSILON )
		{
			ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );  // just zero contactList.size()			
			
			for( int innerContactIdx = 0; innerContactIdx != k; ++innerContactIdx )
			{	
				v[ innerContactIdx ] = -A[ innerContactIdx ][ contactIdx ];
			}

			ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
			deltaF[ contactIdx ] = 1.0f;
			
			solve( A, v, contactIdx, deltaF );  // THIS SOLVER IS FINE. STOP DEBUGGING IT.
			
			ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
			matrixMultiplyVector( A, deltaF, numContacts, deltaA );

			// Calculate maxstep.
			float s = FLT_MAX;
			int j = -1;
			if( deltaA[ contactIdx ] > FLT_EPSILON )
			{
				j = contactIdx;
				s = -a[ contactIdx ] / deltaA[ contactIdx ];
			}
			else
			{
				assert( false );
			}
			
			int i = 0;		
			for( ; i != k; ++i )
			{			
				if( deltaF[ i ] < -FLT_EPSILON )
				{
					const float posS = -f[ i ] / deltaF[ i ];
					if( posS < s )
					{
						s = posS;
						j = i;		
					}
				}
			}

			for( ; i != contactIdx; ++i )
			{
				if( deltaA[ i ] < -FLT_EPSILON )
				{
					const float posS = -a[ i ] / deltaA[ i ];
					if( posS < s )
					{
						s = posS;
						j = i;			
					}
				}
			}
			
			for( int e = 0; e != numContacts; ++e )
			{				
				f[ e ] += s * deltaF[ e ];
				a[ e ] += s * deltaA[ e ];
			}

			if( j < k ) 
			{	
				memcpy( swapTemp, A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
				const float aSwapTemp = a[ j ];
				const float fSwapTemp = f[ j ];
				const int CSwapTemp = C[ j ];
				for( int shuffleIdx = ( j + 1 ); shuffleIdx != k; ++shuffleIdx )
				{
					memcpy( A[ shuffleIdx - 1 ], A[ shuffleIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
					a[ shuffleIdx - 1 ] = a[ shuffleIdx ];
					f[ shuffleIdx - 1 ] = f[ shuffleIdx ];
					C[ shuffleIdx - 1 ] = C[ shuffleIdx ];
				}
				memcpy( A[ k - 1 ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
				a[ k - 1 ] = aSwapTemp;
				f[ k - 1 ] = fSwapTemp;
				C[ k - 1 ] = CSwapTemp;

				for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
				{
					const float ASwapTemp = A[ rowIdx ][ j ];
					for( int shuffleIdx = ( j + 1 ); shuffleIdx != k; ++shuffleIdx )
					{
						A[ rowIdx ][ shuffleIdx - 1 ] = A[ rowIdx ][ shuffleIdx ];
					}
					A[ rowIdx ][ k - 1 ] = ASwapTemp;
				}

				--k;
			}
			else if( j < contactIdx )
			{
				memcpy( swapTemp, A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
				const float aSwapTemp = a[ j ];
				const float fSwapTemp = f[ j ];
				const int CSwapTemp = C[ j ];
				for( int shuffleIdx = j; shuffleIdx != k; --shuffleIdx )
				{
					memcpy( A[ shuffleIdx ], A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
					a[ shuffleIdx ] = a[ shuffleIdx - 1 ];
					f[ shuffleIdx ] = f[ shuffleIdx - 1 ];
					C[ shuffleIdx ] = C[ shuffleIdx - 1 ];
				}
				memcpy( A[ k ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
				a[ k ] = aSwapTemp;
				f[ k ] = fSwapTemp;
				C[ k ] = CSwapTemp;

				for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
				{
					const float ASwapTemp = A[ rowIdx ][ j ];
					for( int shuffleIdx = j; shuffleIdx != k; --shuffleIdx )
					{
						A[ rowIdx ][ shuffleIdx ] = A[ rowIdx ][ shuffleIdx - 1 ];
					}
					A[ rowIdx ][ k ] = ASwapTemp;
				}

				++k;
			}
			else
			{
				a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
				drivenToZero = true;

				memcpy( swapTemp, A[ contactIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
				const float aSwapTemp = a[ contactIdx ];
				const float fSwapTemp = f[ contactIdx ];
				const int CSwapTemp = C[ contactIdx ];
				for( int shuffleIdx = contactIdx; shuffleIdx != k; --shuffleIdx )
				{
					memcpy( A[ shuffleIdx ], A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
					a[ shuffleIdx ] = a[ shuffleIdx - 1 ];
					f[ shuffleIdx ] = f[ shuffleIdx - 1 ];
					C[ shuffleIdx ] = C[ shuffleIdx - 1 ];
				}
				memcpy( A[ k ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
				a[ k ] = aSwapTemp;
				f[ k ] = fSwapTemp;
				C[ k ] = CSwapTemp;

				for( int rowIdx = 0; rowIdx != ( contactIdx + 1 ); ++rowIdx )
				{
					const float ASwapTemp = A[ rowIdx ][ contactIdx ];
					for( int shuffleIdx = contactIdx; shuffleIdx != k; --shuffleIdx )
					{
						A[ rowIdx ][ shuffleIdx ] = A[ rowIdx ][ shuffleIdx - 1 ];
					}
					A[ rowIdx ][ k ] = ASwapTemp;
				}

				++k;
			}
		}
	}


	// Apply/add contact forces.
	// Iterate over all contacts and use the force magnitudes calculated in f[] 
	// above and the geometry of the contact (position + normal) to add force 
	// and torque to both objects (except if the object is infinite mass).
	for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
	{
		const ContactStruct& contact = contactVector[ C[ contactIdx ] ];

		// The above calculates equal but opposite forces to be applied to both finite mass 
		// objects involved in the contact. The location of the foce is at the vertex of the 
		// 'vertex object', the direction is along the normal of the 'edge object', the 
		// magnitude is +/-f[ contactIdx ].
		
		const D3DXVECTOR4 contactPos = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );	
		const D3DXVECTOR4 contactNorm = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );	

		static float fakeMult = 1.0f;

		if( !contact.m_vertShapePtr->infiniteMass() )
		{
			contact.m_vertShapePtr->addForceActingAtWorldPosition( f[ contactIdx ] * fakeMult * contactNorm, contactPos );

			DevGraphics::line( 
				561 + ( int )contact.m_vertShapePtr + ( int )&contact,
				contactPos, 
				contactPos + ( f[ contactIdx ] * contactNorm * 0.02f ), 
				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
		}

		if( !contact.m_edgeShapePtr->infiniteMass() )
		{
			contact.m_edgeShapePtr->addForceActingAtWorldPosition( -f[ contactIdx ] * fakeMult * contactNorm, contactPos );

			DevGraphics::line( 
				99812 + ( int )contact.m_edgeShapePtr + ( int )&contact,
				contactPos, 
				contactPos + ( -f[ contactIdx ] * contactNorm * 0.02f ), 
				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
		}
	}



	// Iterate over all shapes and update their dynamics by deltaTime.
	// Test for any resulting penetration
	// If there is any revert back to previous dynamic state (as an approximation for time of collision) and apply collision impulses.
	for( EngineShapeListType::iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
	{
		ShapeClass & shape = *shapeItr;

		// Turn this assert on if you're sure the shapes will never be intersecting 
		// at the start of each step.  This *should* be the case as the below collision 
		// code will always set the dynamics to a point before penetration and apply 
		// appropriate impulses to move the shapes apart however there's often external 
		// influences that move shapes into collision. Primarily the dev 'pick and move' 
		// system that lets the user move shapes around.  In the future it may also be 
		// possible that using code could apply impulses to shapes that oppose ones 
		// generated by collision responses, pushing shapes into collision.  There could 
		// be simple floating point accuracy issues and code bugs that leave shapes 
		// penetrating =(.
		// The loop below assumes the shape isn't in collision when deltaTime = 0 so it's 
		// always falling back towards it.  However it's a niave loop and loops infinitely 
		// if, in fact, the shape is in collision at deltaTime = 0.  Which is why it has a 
		// limit to the number of times it can loop before dropping out regardles.  Ideally 
		// the loop would drop out at the first deltaTime it finds where the shape is not 
		// in collision but is very close to colliding.  If we reach max loops before this 
		// time is found the dynamics are left at the last deltatTime found where the shape 
		// was not in collision.  This is partially satisfactory.  It will advance the 
		// dynamics some amount and leave shapes non-colliding (or as they where if they 
		// were in fact colliding in the first place) but they could be farther than the 
		// tollerance distance apart.  This becomes a problem during 'contact force' calculation 
		// because it depends on some tollerence distance between shapes.  This shape may 
		// need contact forces applied but could miss out because it's too far apart.  
		// This could result in the simulation getting 'stuck'.
		//assert( shapeInCollision( shape, m_shapeList ) == 0 );	

		// Take a copy of the dynamics at this time so I can reset them if this 
		// update moves the shape into collision.  It is assumed the shape is not 
		// in collision at the start.
		DynamicsClass const initialDynamics = shape.dynamics();
		
		shape.stepDynamics( deltaTime );

		// See if this update has moved the shape into collision.  If it has we
		// need to do a binary time search to find the approximate time of the 
		// collision.  If not, do nowt more, dynamics updated successfully.		
		ShapeClass * collidedShapePtr = shapeInCollision( shape, m_shapeList );    // <-- This is where the vclip cache will help.
		if( collidedShapePtr != 0 )
		{	
			// No point doing any collision response if both are infinite mass.
			// This prevents some division by zeros, partcularly 'j'
			if( !shape.infiniteMass( ) || !collidedShapePtr->infiniteMass( ) ) 
			{
				FeatureClass shapeFeature;
				FeatureClass collidedShapeFeature;
				
				float shapeDistanceAlongEdge = 0.0f;
				float collidedShapeDistanceAlongEdge = 0.0f;						
				float distanceSquared = FLT_MAX;

				// It's only worth while estimating the time of collision if this shape is not infinite mass
				if( !shape.infiniteMass( ) )
				{		
					float minTime = 0.0f;
					float timeRange = deltaTime / 2.0f;
	
					bool foundTimeOfCollision = false;
					static int maxLoops = 5;
					int numLoops = 0;
					do
					{
						++numLoops;
	
						// Could combine these two some how?
						shape.setDynamics( initialDynamics ); 
						shape.stepDynamics( minTime + timeRange );	
	
						ShapeClass * newCollidedShapePtr = shapeInCollision( shape, m_shapeList );
						if( newCollidedShapePtr != 0 )
						{						
							collidedShapePtr = newCollidedShapePtr;						
							
							timeRange /= 2.0f;
						}
						else
						{				
							shapeDistanceAlongEdge = 0.0f;
							collidedShapeDistanceAlongEdge = 0.0f;						
							distanceSquared = FLT_MAX;
				
							shapeClosestFeatures(
								distanceSquared,
								shape, shapeFeature, shapeDistanceAlongEdge,
								*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
							assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );

							//float distance = sqrtf( distanceSquared );

							if( distanceSquared > ( PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE * PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ) )
							{
								minTime += timeRange;
								timeRange /= 2.0f;
							}
							else
							{
								foundTimeOfCollision = true;
							}	
						}
					}
					while( !foundTimeOfCollision && ( numLoops != maxLoops ) );

					// It's possible to exit the loop because we've done too many iterations.  In this case the dynamics 
					// will have been reset to their initial values.  However 'minTime' may have been incremented beyond 
					// 0.0f which means some motion is possible before collision occurs which is why I've added this if 
					// to move the simulation on as far as possible.
					if( !foundTimeOfCollision )
					{
						// Could combine these two some how?
						shape.setDynamics( initialDynamics );						
						//shape.stepDynamics( deltaTime );
						shape.stepDynamics( minTime );
								
						shapeDistanceAlongEdge = 0.0f;
						collidedShapeDistanceAlongEdge = 0.0f;						
						distanceSquared = FLT_MAX;
				
						shapeClosestFeatures(
							distanceSquared,
							shape, shapeFeature, shapeDistanceAlongEdge,
							*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
						assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );
					}
				}
				else
				{		
					// shape is infinite mass, may as well just calculate the closest features now both shapes are in there new positions
					shapeClosestFeatures(
						distanceSquared,
						shape, shapeFeature, shapeDistanceAlongEdge,
						*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
					assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );
				}
										
				ShapeClass * shapeAPtr = 0;
				ShapeClass * shapeBPtr = 0;
				D3DXVECTOR4 collisionPosition( 0.0f, 0.0f, 0.0f, 1.0f );
				D3DXVECTOR4 collisionUnitNormal( 0.0f, 0.0f, 1.0f, 0.0f );

				if( shapeFeature.type() == VertexFeature )
				{
					shapeAPtr = &shape;
					shapeBPtr = collidedShapePtr;
					collisionPosition = shape.transformedVertexPosition( shapeFeature.index() );
					collisionUnitNormal = collidedShapePtr->transformedEdgeNormalDirectionUnit( collidedShapeFeature.index() );
				}
				else // if( collidedShapeFeature.type() == VertexFeature )
				{
					shapeAPtr = collidedShapePtr;
					shapeBPtr = &shape;
					collisionPosition = collidedShapePtr->transformedVertexPosition( collidedShapeFeature.index() );
					collisionUnitNormal = shape.transformedEdgeNormalDirectionUnit( shapeFeature.index() );
				}		
				//else
				//{
				//	// Edge edge whitnesses.

				//	shapeAPtr = &shape;
				//	shapeBPtr = collidedShapePtr;
				//	collisionPosition = 
				//		shape.transformedVertexPosition( shapeFeature.index() ) +
				//		( shape.transformedEdgeDirection( shapeFeature.index() ) * 0.5f );
				//	collisionUnitNormal = collidedShapePtr->transformedEdgeNormalDirectionUnit( collidedShapeFeature.index() );
				//}							

				D3DXVECTOR4 const radiusA = collisionPosition - shapeAPtr->centerOfMassWorldPosition();
				D3DXVECTOR4 const radiusB = collisionPosition - shapeBPtr->centerOfMassWorldPosition();
					
				D3DXVECTOR4 const radiusTangentA( -radiusA.y, radiusA.x, radiusA.z, radiusA.w );
				D3DXVECTOR4 const radiusTangentB( -radiusB.y, radiusB.x, radiusB.z, radiusB.w );

				D3DXVECTOR4 const velAP = shapeAPtr->dynamics().velocity() + ( radiusTangentA * shapeAPtr->dynamics().angularVelocity() );
				D3DXVECTOR4 const velBP = shapeBPtr->dynamics().velocity() + ( radiusTangentB * shapeBPtr->dynamics().angularVelocity() );
				D3DXVECTOR4 const relVel = velAP - velBP;

				float const relVelDotNormal = D3DXVec4Dot( &relVel, &collisionUnitNormal );
				float const radiusTangentADotNormal = D3DXVec4Dot( &radiusTangentA, &collisionUnitNormal );
				float const radiusTangentBDotNormal = D3DXVec4Dot( &radiusTangentB, &collisionUnitNormal );							

				float reciprocalMassSum = 0.0f;		
				float reciprocalInirtiaSum = 0.0f;
				if( !shapeAPtr->infiniteMass() )
				{
					assert( shapeAPtr->mass() > FLT_EPSILON );
					assert( shapeAPtr->mass() < FLT_MAX );

					reciprocalMassSum += ( 1.0f / shapeAPtr->mass() );
					reciprocalInirtiaSum += ( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
				}
				if( !shapeBPtr->infiniteMass() )
				{
					assert( shapeBPtr->mass() > FLT_EPSILON );
					assert( shapeBPtr->mass() < FLT_MAX );

					reciprocalMassSum += ( 1.0f / shapeBPtr->mass() );
					reciprocalInirtiaSum += ( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
				}

				float const e = 0.0f;

				const float jDivisor = ( reciprocalMassSum + reciprocalInirtiaSum );
				assert( fabs( jDivisor ) > FLT_EPSILON );
				float const j = ( -( 1.0f + e ) * relVelDotNormal ) / jDivisor;

				if( !shapeAPtr->infiniteMass() )
				{
					assert( shapeAPtr->mass() > FLT_EPSILON );
					assert( shapeAPtr->mass() < FLT_MAX );
								
					D3DXVECTOR4 velA2 = shapeAPtr->dynamics().velocity();
					velA2 += ( ( j / shapeAPtr->mass() ) * collisionUnitNormal );
						
					float const angVelA2 = shapeAPtr->dynamics().angularVelocity() + ( ( j * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );

					shapeAPtr->setDynamics(
						shapeAPtr->dynamics().position(),
						shapeAPtr->dynamics().orientation(),
						velA2,
						angVelA2 );
				}

				if( !shapeBPtr->infiniteMass() )
				{
					assert( shapeBPtr->mass() > FLT_EPSILON );
					assert( shapeBPtr->mass() < FLT_MAX );
								
					D3DXVECTOR4 velB2 = shapeBPtr->dynamics().velocity();
					velB2 -= ( ( j / shapeBPtr->mass() ) * collisionUnitNormal );
								
					float const angVelB2 = shapeBPtr->dynamics().angularVelocity() - ( ( j * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );

					shapeBPtr->setDynamics(
						shapeBPtr->dynamics().position(),
						shapeBPtr->dynamics().orientation(),
						velB2,
						angVelB2 );
				}
			}
		}
	}
#endif
	

	++m_stepCount;
}



//void EngineClass::step( float const deltaTime )
//{
//#if 1
//	////////////////////
//	// CONTACT FORCES //
//	////////////////////
//
//	// Before updating the simulation use the current state of the objects 
//	// to calculate any contact forces as these need to be added to the external forces.	
//
//	// GET SET OF CONTACTS
//
//	// Iterate across all shapes/objects and test against all others (This is obviously 
//	// one of those processes that'll benifit from broad/narrow phase, sweep and prune 
//	// style optimisations =) ). Each 'contact' will have involve a 'vertex shape' and 
//	// an 'edge shape'. The outer loop shape is the potential 'vertex' shape, the inner 
//	// is the 'edge'. Test each vertex on the vertex shape against each edge on the edge 
//	// shape to see if any pair is a contact. This is a process that'll benif from vclip 
//	// closest feature caching. A pair of shapes can produce multiple contact feature 
//	// pairs.	
//	ContactVectorType contactVector;
//	for( EngineShapeListType::iterator vertShapeItr = m_shapeList.begin(); vertShapeItr != m_shapeList.end(); ++vertShapeItr )
//	{
//		ShapeClass & vertShape = *vertShapeItr;
//	
//		for( EngineShapeListType::iterator edgeShapeItr = m_shapeList.begin(); edgeShapeItr != m_shapeList.end(); ++edgeShapeItr )
//		{
//			ShapeClass & edgeShape = *edgeShapeItr;
//
//			if( vertShapeItr != edgeShapeItr )
//			{
//				int const numVerts = vertShape.numberOfVertices();
//				int const numEdges = edgeShape.numberOfEdges();
//
//				for( int vertIdx = 0; vertIdx != numVerts; ++vertIdx )
//				{	
//					D3DXVECTOR4 const vertPos = vertShape.transformedVertexPosition( vertIdx );	
//						
//					for( int edgeIdx = 0; edgeIdx != numEdges; ++edgeIdx )
//					{
//						// Okay so here is the test. We're considering a vert and and edge. They are in contact 
//						// if they are close enough together (within the bounds of the edge) and we should really 
//						// only consider fetures that are moving or accelerating towards each other. But whart 
//						// about leaving fast moving things to be resolved by the collision detectiona and response?
//
//						D3DXVECTOR4 const edgePos = edgeShape.transformedVertexPosition( edgeIdx );
//						D3DXVECTOR4 const edgeDirUnit = edgeShape.transformedEdgeDirectionUnit( edgeIdx );
//						float const edgeLen = edgeShape.edgeLength( edgeIdx );
//						D3DXVECTOR4 const edgeUnitNrml = edgeShape.transformedEdgeNormalDirectionUnit( edgeIdx );
//
//						D3DXVECTOR4 const edgeToVertDir = vertPos - edgePos;
//
//						float const edgeToVertDirDotEdgeDirUnit = D3DXVec4Dot( &edgeToVertDir, &edgeDirUnit );
//						float const signedDist = D3DXVec4Dot( &edgeUnitNrml, &edgeToVertDir );
//							
//						if( ( edgeToVertDirDotEdgeDirUnit > 0.0f ) && 
//							( edgeToVertDirDotEdgeDirUnit < edgeLen ) &&
//							( signedDist <= PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ) &&
//							( signedDist >= -PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ) )
//						{
//							// This vert and edge are close enough together to be in contact but we need to determine their 
//							// relative velocity along the edge normal to determine if they're not moving apart anyway.
//
//							D3DXVECTOR4 const edgeContactPos = edgePos + ( edgeDirUnit * edgeToVertDirDotEdgeDirUnit );
//								
//							D3DXVECTOR4 const vertVel = shapeVertexVelocity( vertShape, vertIdx );
//							D3DXVECTOR4 const edgeContactPosVel = shapeVelocityAtPoint( edgeShape, edgeContactPos );
//
//							D3DXVECTOR4 const relVel = vertVel - edgeContactPosVel;
//
//							float const signedSpd = D3DXVec4Dot( &edgeUnitNrml, &relVel );
//
//							if( signedSpd <= 0.01f )
//							{
//								if( !contactVector.full() )
//								{
//									contactVector.push_back( ContactStruct( vertShape, vertIdx, edgeShape, edgeIdx ) );
//								}								
//							}
//						}
//					}			
//				}
//			}
//		}
//	}
//
//	const int numContacts = ( int )contactVector.size();	
//	
//	// SET UP A MATRIX AND b VECTOR
//
//	// We're aiming to calculate a force at each of the contact points that should 
//	// be enough to keep the objects from interpenmatrating but not soo much that 
//	// they move apart.
//
//	// Here we populate a 2d matrix and a 	
//	float A[ PHYSICS_ENGINE_MAX_CONTACTS ][ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
//	float b[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
//
//	for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
//	{	
//		ContactStruct& iContact = contactVector[ contactIdx ];
//
//		const D3DXVECTOR4 iContactPos = iContact.m_vertShapePtr->transformedVertexPosition( iContact.m_vertIdx );
//		DevGraphics::point( ( ( int )iContact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + iContact.m_vertIdx, iContactPos ); 
//
//		const D3DXVECTOR4 iContactVertShapeCOMPos = iContact.m_vertShapePtr->centerOfMassWorldPosition();
//		//DevGraphics::point( 9923 + ( int )iContact.m_vertShapePtr, iContactVertShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
//		const D3DXVECTOR4 iContactEdgeShapeCOMPos = iContact.m_edgeShapePtr->centerOfMassWorldPosition();
//		//DevGraphics::point( 12355 + ( int )iContact.m_edgeShapePtr, iContactEdgeShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );
//		
//		const D3DXVECTOR4 iContactRadius1 = iContactPos - iContactVertShapeCOMPos;			
//		//DevGraphics::line( 
//		//	7267 + ( int )iContact.m_vertShapePtr, 
//		//	iContactVertShapeCOMPos, 
//		//	iContactVertShapeCOMPos + iContactRadius1, 
//		//	D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
//		//	D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
//		const D3DXVECTOR4 iContactRadius2 = iContactPos - iContactEdgeShapeCOMPos;		
//		//DevGraphics::line( 
//		//	43322 + ( int )iContact.m_edgeShapePtr, 
//		//	iContactEdgeShapeCOMPos, 
//		//	iContactEdgeShapeCOMPos + iContactRadius2, 
//		//	D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ),
//		//	D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );
//
//		const D3DXVECTOR4 iContactNorm = iContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( iContact.m_edgeIdx );		
//		//DevGraphics::line( 55 + ( int )iContact.m_edgeShapePtr, iContactPos, iContactPos + ( iContactNorm * 10.0f ) );
//		const D3DXVECTOR4 iContactNegNorm = -iContactNorm;		
//	
//		for( int jContactIdx = 0; jContactIdx != numContacts; ++jContactIdx )
//		{	
//			const ContactStruct& jContact = contactVector[ jContactIdx ];
//			
//			//ni ⋅ (nj / m1 + (rj × nj) × r1 / I1)
//
//			const D3DXVECTOR4 jContactPos = jContact.m_vertShapePtr->transformedVertexPosition( jContact.m_vertIdx );		
//			const D3DXVECTOR4 jContactNorm = jContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( jContact.m_edgeIdx );
//			const D3DXVECTOR4 jContactNegNorm = -jContactNorm;
//
//			if( !iContact.m_vertShapePtr->infiniteMass() )
//			{
//				if( iContact.m_vertShapePtr == jContact.m_vertShapePtr )
//				{
//					assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
//					assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
//					assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
//					assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );
//
//					const D3DXVECTOR4 jRad = jContactPos - iContactVertShapeCOMPos;
//					
//					D3DXVECTOR4 jRadXJNorm;
//					D3DXVec4Cross( &jRadXJNorm, &jRad, &jContactNorm, &Misc::origin() );			
//					
//					D3DXVECTOR4 jRadXJNormXIRad1;
//					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );
//
//					D3DXVECTOR4 temp = ( jContactNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
//
//					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &temp );
//				}
//			
//				if( iContact.m_vertShapePtr == jContact.m_edgeShapePtr )
//				{				
//					assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
//					assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
//					assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
//					assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );
//
//					const D3DXVECTOR4 jContactRadius = jContactPos - iContactVertShapeCOMPos;
//					
//					D3DXVECTOR4 jRadXJNorm;
//					D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );			
//					
//					D3DXVECTOR4 jRadXJNormXIRad1;
//					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );
//
//					D3DXVECTOR4 temp = ( jContactNegNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
//					
//					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &temp );
//				}
//			}
//
//			if( !iContact.m_edgeShapePtr->infiniteMass() )
//			{
//				if( iContact.m_edgeShapePtr == jContact.m_vertShapePtr )
//				{				
//					assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
//					assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
//					assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
//					assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );
//
//					const D3DXVECTOR4 jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
//					
//					D3DXVECTOR4 jRadXJNorm;
//					D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNorm, &Misc::origin() );			
//					
//					D3DXVECTOR4 jRadXJNormXIRad1;
//					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );
//
//					D3DXVECTOR4 temp = ( jContactNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );
//
//					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &temp );
//				}
//			
//				if( iContact.m_edgeShapePtr == jContact.m_edgeShapePtr )
//				{				
//					assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
//					assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
//					assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
//					assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );
//
//					const D3DXVECTOR4 jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
//					
//					D3DXVECTOR4 jRadXJNorm;
//					D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );			
//					
//					D3DXVECTOR4 jRadXJNormXIRad1;
//					D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );
//
//					D3DXVECTOR4 temp = ( jContactNegNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );
//					
//					A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &temp );
//				}
//			}
//		}
//		
//		const D3DXVECTOR4 vertShapeAngVel( 0.0f, 0.0f, iContact.m_vertShapePtr->angularVelocity(), 0.0f );
//		const D3DXVECTOR4 edgeShapeAngVel( 0.0f, 0.0f, iContact.m_edgeShapePtr->angularVelocity(), 0.0f );
//		
//		D3DXVECTOR4 vertShapeAngVelCrossRadius1;
//		D3DXVec4Cross( &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &iContactRadius1, &Misc::origin() );
//
//		D3DXVECTOR4 edgeShapeAngVelCrossRadius2;
//		D3DXVec4Cross( &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &iContactRadius2, &Misc::origin() );
//		
//		D3DXVECTOR4 const temp =
//			( iContact.m_vertShapePtr->velocity() + vertShapeAngVelCrossRadius1 ) - 
//			( iContact.m_edgeShapePtr->velocity() + edgeShapeAngVelCrossRadius2 );					
//
//		D3DXVECTOR4 edgeShapeAngVelCrossNorm;
//		D3DXVec4Cross( &edgeShapeAngVelCrossNorm, &edgeShapeAngVel, &iContactNorm, &Misc::origin() );
//		
//		// Stricktly 2D
//		b[ contactIdx ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
//			 ( ( iContactNorm.x * temp.y ) - ( iContactNorm.y * temp.x ) );
//		//b[ contactIdx ] += 2.0f * D3DXVec4Dot( &iContactNorm, &temp );
//		//b[ contactIdx ] += 2.0f * D3DXVec4Dot( &edgeShapeAngVelCrossNorm, &temp );
//
//
//
//		D3DXVECTOR4 vertShapeAcc = Misc::zero();
//		D3DXVECTOR4 vertShapeAngAcc = Misc::zero();
//		if( !iContact.m_vertShapePtr->infiniteMass() )
//		{	
//			assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
//			assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
//			assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
//			assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );	
//
//			vertShapeAcc = iContact.m_vertShapePtr->force() / iContact.m_vertShapePtr->mass();
//			vertShapeAngAcc.z = iContact.m_vertShapePtr->torque() / iContact.m_vertShapePtr->momentOfInertia();
//		}
//		
//		D3DXVECTOR4 edgeShapeAcc = Misc::zero();
//		D3DXVECTOR4 edgeShapeAngAcc = Misc::zero();
//		if( !iContact.m_edgeShapePtr->infiniteMass() )
//		{
//			assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
//			assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
//			assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
//			assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );
//
//			edgeShapeAcc = iContact.m_edgeShapePtr->force() / iContact.m_edgeShapePtr->mass();
//			edgeShapeAngAcc.z = iContact.m_edgeShapePtr->torque() / iContact.m_edgeShapePtr->momentOfInertia();
//		}
//		
//		D3DXVECTOR4 vertShapeAngAccCrossRadius1;
//		D3DXVec4Cross( &vertShapeAngAccCrossRadius1, &vertShapeAngAcc, &iContactRadius1, &Misc::origin() );
//
//		D3DXVECTOR4 edgeShapeAngAccCrossRadius2;
//		D3DXVec4Cross( &edgeShapeAngAccCrossRadius2, &edgeShapeAngAcc, &iContactRadius2, &Misc::origin() );
//		
//		D3DXVECTOR4 vertShapeAngVelCrossVertShapeAngVelCrossRadius1;
//		D3DXVec4Cross( &vertShapeAngVelCrossVertShapeAngVelCrossRadius1,  &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &Misc::origin() );
//
//		D3DXVECTOR4 edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2;
//		D3DXVec4Cross( &edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2, &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &Misc::origin() );
//		
//		D3DXVECTOR4 const temp2 =
//			( vertShapeAcc + vertShapeAngAccCrossRadius1 + vertShapeAngVelCrossVertShapeAngVelCrossRadius1 ) -
//			( edgeShapeAcc + edgeShapeAngAccCrossRadius2 + edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2 );
//		
//		b[ contactIdx ] += D3DXVec4Dot( &iContactNorm, &temp2 );
//	}
//
//	// COMPUTE FORCES
//
//	// Init all forces to zero.
//	float f[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };	
//
//	// a[] is speeds at each contact point.
//	// Init a to be b
//	float* a = b;
//	
//	float v[ PHYSICS_ENGINE_MAX_CONTACTS ];	
//	float deltaF[ PHYSICS_ENGINE_MAX_CONTACTS ];	
//	float deltaA[ PHYSICS_ENGINE_MAX_CONTACTS ];	
//
//	// Spare spce to use during swap
//	float swapTemp[ PHYSICS_ENGINE_MAX_CONTACTS ];	
//	
//	// k is the current pivot point.
//	// Contact <=k are clamped with f[i]>0 and a[i]<=0. 	
//	// Contact >k are unclamped with f[i]=0 and a[i]>0. 
//	int k = 0;	
//	//ContactListType::iterator kContactItr = contactList.begin();		
//	// This is initialised one element at at time in the loop below
//	int C[ PHYSICS_ENGINE_MAX_CONTACTS ];	
//
//
//	// Iterate over all the contacts and drive a to zero if it is less
//	// It should be fine to use a 'for()' here even though I'm modifying the list within 
//	// because any modifications occur before contactItr therefore it's still alid to 
//	// increment and compare against .end().
//	for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
//	{
//		C[ contactIdx ] = contactIdx;
//
//		// Keep looping and trying to drive b to zero
//		bool drivenToZero = ( a[ contactIdx ] >= -FLT_EPSILON );
//		while( drivenToZero == false ) // a[ contactIdx ] < -FLT_EPSILON )
//		{
//			ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );  // just zero contactList.size()			
//			
//			for( int innerContactIdx = 0; innerContactIdx != k; ++innerContactIdx )
//			{	
//				v[ innerContactIdx ] = -A[ innerContactIdx ][ contactIdx ];
//			}
//
//			ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//			deltaF[ contactIdx ] = 1.0f;
//			
//			solve( A, v, contactIdx, deltaF );  // THIS SOLVER IS FINE. STOP DEBUGGING IT.
//			
//			ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//			matrixMultiplyVector( A, deltaF, numContacts, deltaA );
//
//			// Calculate maxstep.
//			float s = FLT_MAX;
//			int j = -1;
//			if( deltaA[ contactIdx ] > FLT_EPSILON )
//			{
//				j = contactIdx;
//				s = -a[ contactIdx ] / deltaA[ contactIdx ];
//			}
//			else
//			{
//				assert( false );
//			}
//			
//			int i = 0;		
//			for( ; i != k; ++i )
//			{			
//				if( deltaF[ i ] < -FLT_EPSILON )
//				{
//					const float posS = -f[ i ] / deltaF[ i ];
//					if( posS < s )
//					{
//						s = posS;
//						j = i;		
//					}
//				}
//			}
//
//			for( ; i != contactIdx; ++i )
//			{
//				if( deltaA[ i ] < -FLT_EPSILON )
//				{
//					const float posS = -a[ i ] / deltaA[ i ];
//					if( posS < s )
//					{
//						s = posS;
//						j = i;			
//					}
//				}
//			}
//			
//			for( int e = 0; e != numContacts; ++e )
//			{				
//				f[ e ] += s * deltaF[ e ];
//				a[ e ] += s * deltaA[ e ];
//			}
//
//			if( j < k ) 
//			{	
//				memcpy( swapTemp, A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//				const float aSwapTemp = a[ j ];
//				const float fSwapTemp = f[ j ];
//				const int CSwapTemp = C[ j ];
//				for( int shuffleIdx = ( j + 1 ); shuffleIdx != k; ++shuffleIdx )
//				{
//					memcpy( A[ shuffleIdx - 1 ], A[ shuffleIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//					a[ shuffleIdx - 1 ] = a[ shuffleIdx ];
//					f[ shuffleIdx - 1 ] = f[ shuffleIdx ];
//					C[ shuffleIdx - 1 ] = C[ shuffleIdx ];
//				}
//				memcpy( A[ k - 1 ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//				a[ k - 1 ] = aSwapTemp;
//				f[ k - 1 ] = fSwapTemp;
//				C[ k - 1 ] = CSwapTemp;
//
//				for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
//				{
//					const float ASwapTemp = A[ rowIdx ][ j ];
//					for( int shuffleIdx = ( j + 1 ); shuffleIdx != k; ++shuffleIdx )
//					{
//						A[ rowIdx ][ shuffleIdx - 1 ] = A[ rowIdx ][ shuffleIdx ];
//					}
//					A[ rowIdx ][ k - 1 ] = ASwapTemp;
//				}
//
//				--k;
//			}
//			else if( j < contactIdx )
//			{
//				memcpy( swapTemp, A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//				const float aSwapTemp = a[ j ];
//				const float fSwapTemp = f[ j ];
//				const int CSwapTemp = C[ j ];
//				for( int shuffleIdx = j; shuffleIdx != k; --shuffleIdx )
//				{
//					memcpy( A[ shuffleIdx ], A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//					a[ shuffleIdx ] = a[ shuffleIdx - 1 ];
//					f[ shuffleIdx ] = f[ shuffleIdx - 1 ];
//					C[ shuffleIdx ] = C[ shuffleIdx - 1 ];
//				}
//				memcpy( A[ k ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//				a[ k ] = aSwapTemp;
//				f[ k ] = fSwapTemp;
//				C[ k ] = CSwapTemp;
//
//				for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
//				{
//					const float ASwapTemp = A[ rowIdx ][ j ];
//					for( int shuffleIdx = j; shuffleIdx != k; --shuffleIdx )
//					{
//						A[ rowIdx ][ shuffleIdx ] = A[ rowIdx ][ shuffleIdx - 1 ];
//					}
//					A[ rowIdx ][ k ] = ASwapTemp;
//				}
//
//				++k;
//			}
//			else
//			{
//				a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
//				drivenToZero = true;
//
//				memcpy( swapTemp, A[ contactIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//				const float aSwapTemp = a[ contactIdx ];
//				const float fSwapTemp = f[ contactIdx ];
//				const int CSwapTemp = C[ contactIdx ];
//				for( int shuffleIdx = contactIdx; shuffleIdx != k; --shuffleIdx )
//				{
//					memcpy( A[ shuffleIdx ], A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//					a[ shuffleIdx ] = a[ shuffleIdx - 1 ];
//					f[ shuffleIdx ] = f[ shuffleIdx - 1 ];
//					C[ shuffleIdx ] = C[ shuffleIdx - 1 ];
//				}
//				memcpy( A[ k ], swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
//				a[ k ] = aSwapTemp;
//				f[ k ] = fSwapTemp;
//				C[ k ] = CSwapTemp;
//
//				for( int rowIdx = 0; rowIdx != ( contactIdx + 1 ); ++rowIdx )
//				{
//					const float ASwapTemp = A[ rowIdx ][ contactIdx ];
//					for( int shuffleIdx = contactIdx; shuffleIdx != k; --shuffleIdx )
//					{
//						A[ rowIdx ][ shuffleIdx ] = A[ rowIdx ][ shuffleIdx - 1 ];
//					}
//					A[ rowIdx ][ k ] = ASwapTemp;
//				}
//
//				++k;
//			}
//		}
//	}
//
//
//	// Apply/add contact forces.
//	// Iterate over all contacts and use the force magnitudes calculated in f[] 
//	// above and the geometry of the contact (position + normal) to add force 
//	// and torque to both objects (except if the object is infinite mass).
//	for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
//	{
//		const ContactStruct& contact = contactVector[ C[ contactIdx ] ];
//
//		// The above calculates equal but opposite forces to be applied to both finite mass 
//		// objects involved in the contact. The location of the foce is at the vertex of the 
//		// 'vertex object', the direction is along the normal of the 'edge object', the 
//		// magnitude is +/-f[ contactIdx ].
//		
//		const D3DXVECTOR4 contactPos = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );	
//		const D3DXVECTOR4 contactNorm = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );	
//
//		static float fakeMult = 1.0f;
//
//		if( !contact.m_vertShapePtr->infiniteMass() )
//		{
//			contact.m_vertShapePtr->addForceActingAtWorldPosition( f[ contactIdx ] * fakeMult * contactNorm, contactPos );
//
//			DevGraphics::line( 
//				561 + ( int )contact.m_vertShapePtr + ( int )&contact,
//				contactPos, 
//				contactPos + ( f[ contactIdx ] * contactNorm * 0.02f ), 
//				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
//				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
//		}
//
//		if( !contact.m_edgeShapePtr->infiniteMass() )
//		{
//			contact.m_edgeShapePtr->addForceActingAtWorldPosition( -f[ contactIdx ] * fakeMult * contactNorm, contactPos );
//
//			DevGraphics::line( 
//				99812 + ( int )contact.m_edgeShapePtr + ( int )&contact,
//				contactPos, 
//				contactPos + ( -f[ contactIdx ] * contactNorm * 0.02f ), 
//				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ),
//				D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) );
//		}
//	}
//	
//	////////////////////
//	// CONTACT FORCES //
//	////////////////////
//#endif
//
//
//
//	// Iterate over all shapes and update their dynamics by deltaTime.
//	// Test for any resulting penetration
//	// If there is any revert back to previous dynamic state (as an approximation for time of collision) and apply collision impulses.
//	for( EngineShapeListType::iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
//	{
//		ShapeClass & shape = *shapeItr;
//
//		// Turn this assert on if you're sure the shapes will never be intersecting 
//		// at the start of each step.  This *should* be the case as the below collision 
//		// code will always set the dynamics to a point before penetration and apply 
//		// appropriate impulses to move the shapes apart however there's often external 
//		// influences that move shapes into collision. Primarily the dev 'pick and move' 
//		// system that lets the user move shapes around.  In the future it may also be 
//		// possible that using code could apply impulses to shapes that oppose ones 
//		// generated by collision responses, pushing shapes into collision.  There could 
//		// be simple floating point accuracy issues and code bugs that leave shapes 
//		// penetrating =(.
//		// The loop below assumes the shape isn't in collision when deltaTime = 0 so it's 
//		// always falling back towards it.  However it's a niave loop and loops infinitely 
//		// if, in fact, the shape is in collision at deltaTime = 0.  Which is why it has a 
//		// limit to the number of times it can loop before dropping out regardles.  Ideally 
//		// the loop would drop out at the first deltaTime it finds where the shape is not 
//		// in collision but is very close to colliding.  If we reach max loops before this 
//		// time is found the dynamics are left at the last deltatTime found where the shape 
//		// was not in collision.  This is partially satisfactory.  It will advance the 
//		// dynamics some amount and leave shapes non-colliding (or as they where if they 
//		// were in fact colliding in the first place) but they could be farther than the 
//		// tollerance distance apart.  This becomes a problem during 'contact force' calculation 
//		// because it depends on some tollerence distance between shapes.  This shape may 
//		// need contact forces applied but could miss out because it's too far apart.  
//		// This could result in the simulation getting 'stuck'.
//		//assert( shapeInCollision( shape, m_shapeList ) == 0 );	
//
//		// Take a copy of the dynamics at this time so I can reset them if this 
//		// update moves the shape into collision.  It is assumed the shape is not 
//		// in collision at the start.
//		DynamicsClass const initialDynamics = shape.dynamics();
//		
//		shape.stepDynamics( deltaTime );
//
//		// See if this update has moved the shape into collision.  If it has we
//		// need to do a binary time search to find the approximate time of the 
//		// collision.  If not, do nowt more, dynamics updated successfully.		
//		ShapeClass * collidedShapePtr = shapeInCollision( shape, m_shapeList );    // <-- This is where the vclip cache will help.
//		if( collidedShapePtr != 0 )
//		{	
//			// No point doing any collision response if both are infinite mass.
//			// This prevents some division by zeros, partcularly 'j'
//			if( !shape.infiniteMass( ) || !collidedShapePtr->infiniteMass( ) ) 
//			{
//				FeatureClass shapeFeature;
//				FeatureClass collidedShapeFeature;
//				
//				float shapeDistanceAlongEdge = 0.0f;
//				float collidedShapeDistanceAlongEdge = 0.0f;						
//				float distanceSquared = FLT_MAX;
//
//				// It's only worth while estimating the time of collision if this shape is not infinite mass
//				if( !shape.infiniteMass( ) )
//				{		
//					float minTime = 0.0f;
//					float timeRange = deltaTime / 2.0f;
//	
//					bool foundTimeOfCollision = false;
//					static int maxLoops = 5;
//					int numLoops = 0;
//					do
//					{
//						++numLoops;
//	
//						// Could combine these two some how?
//						shape.setDynamics( initialDynamics ); 
//						shape.stepDynamics( minTime + timeRange );	
//	
//						ShapeClass * newCollidedShapePtr = shapeInCollision( shape, m_shapeList );
//						if( newCollidedShapePtr != 0 )
//						{						
//							collidedShapePtr = newCollidedShapePtr;						
//							
//							timeRange /= 2.0f;
//						}
//						else
//						{				
//							shapeDistanceAlongEdge = 0.0f;
//							collidedShapeDistanceAlongEdge = 0.0f;						
//							distanceSquared = FLT_MAX;
//				
//							shapeClosestFeatures(
//								distanceSquared,
//								shape, shapeFeature, shapeDistanceAlongEdge,
//								*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
//							assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );
//
//							//float distance = sqrtf( distanceSquared );
//
//							if( distanceSquared > ( PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE * PHYSICS_ENGINE_CONTACT_DISTANCE_TOLLERENCE ) )
//							{
//								minTime += timeRange;
//								timeRange /= 2.0f;
//							}
//							else
//							{
//								foundTimeOfCollision = true;
//							}	
//						}
//					}
//					while( !foundTimeOfCollision && ( numLoops != maxLoops ) );
//
//					// It's possible to exit the loop because we've done too many iterations.  In this case the dynamics 
//					// will have been reset to their initial values.  However 'minTime' may have been incremented beyond 
//					// 0.0f which means some motion is possible before collision occurs which is why I've added this if 
//					// to move the simulation on as far as possible.
//					if( !foundTimeOfCollision )
//					{
//						// Could combine these two some how?
//						shape.setDynamics( initialDynamics );						
//						//shape.stepDynamics( deltaTime );
//						shape.stepDynamics( minTime );
//								
//						shapeDistanceAlongEdge = 0.0f;
//						collidedShapeDistanceAlongEdge = 0.0f;						
//						distanceSquared = FLT_MAX;
//				
//						shapeClosestFeatures(
//							distanceSquared,
//							shape, shapeFeature, shapeDistanceAlongEdge,
//							*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
//						assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );
//					}
//				}
//				else
//				{		
//					// shape is infinite mass, may as well just calculate the closest features now both shapes are in there new positions
//					shapeClosestFeatures(
//						distanceSquared,
//						shape, shapeFeature, shapeDistanceAlongEdge,
//						*collidedShapePtr, collidedShapeFeature, collidedShapeDistanceAlongEdge );
//					assert( ( shapeFeature.type() == VertexFeature ) || ( collidedShapeFeature.type() == VertexFeature ) );
//				}
//										
//				ShapeClass * shapeAPtr = 0;
//				ShapeClass * shapeBPtr = 0;
//				D3DXVECTOR4 collisionPosition( 0.0f, 0.0f, 0.0f, 1.0f );
//				D3DXVECTOR4 collisionUnitNormal( 0.0f, 0.0f, 1.0f, 0.0f );
//
//				if( shapeFeature.type() == VertexFeature )
//				{
//					shapeAPtr = &shape;
//					shapeBPtr = collidedShapePtr;
//					collisionPosition = shape.transformedVertexPosition( shapeFeature.index() );
//					collisionUnitNormal = collidedShapePtr->transformedEdgeNormalDirectionUnit( collidedShapeFeature.index() );
//				}
//				else // if( collidedShapeFeature.type() == VertexFeature )
//				{
//					shapeAPtr = collidedShapePtr;
//					shapeBPtr = &shape;
//					collisionPosition = collidedShapePtr->transformedVertexPosition( collidedShapeFeature.index() );
//					collisionUnitNormal = shape.transformedEdgeNormalDirectionUnit( shapeFeature.index() );
//				}		
//				//else
//				//{
//				//	// Edge edge whitnesses.
//
//				//	shapeAPtr = &shape;
//				//	shapeBPtr = collidedShapePtr;
//				//	collisionPosition = 
//				//		shape.transformedVertexPosition( shapeFeature.index() ) +
//				//		( shape.transformedEdgeDirection( shapeFeature.index() ) * 0.5f );
//				//	collisionUnitNormal = collidedShapePtr->transformedEdgeNormalDirectionUnit( collidedShapeFeature.index() );
//				//}							
//
//				D3DXVECTOR4 const radiusA = collisionPosition - shapeAPtr->centerOfMassWorldPosition();
//				D3DXVECTOR4 const radiusB = collisionPosition - shapeBPtr->centerOfMassWorldPosition();
//					
//				D3DXVECTOR4 const radiusTangentA( -radiusA.y, radiusA.x, radiusA.z, radiusA.w );
//				D3DXVECTOR4 const radiusTangentB( -radiusB.y, radiusB.x, radiusB.z, radiusB.w );
//
//				D3DXVECTOR4 const velAP = shapeAPtr->dynamics().velocity() + ( radiusTangentA * shapeAPtr->dynamics().angularVelocity() );
//				D3DXVECTOR4 const velBP = shapeBPtr->dynamics().velocity() + ( radiusTangentB * shapeBPtr->dynamics().angularVelocity() );
//				D3DXVECTOR4 const relVel = velAP - velBP;
//
//				float const relVelDotNormal = D3DXVec4Dot( &relVel, &collisionUnitNormal );
//				float const radiusTangentADotNormal = D3DXVec4Dot( &radiusTangentA, &collisionUnitNormal );
//				float const radiusTangentBDotNormal = D3DXVec4Dot( &radiusTangentB, &collisionUnitNormal );							
//
//				float reciprocalMassSum = 0.0f;		
//				float reciprocalInirtiaSum = 0.0f;
//				if( !shapeAPtr->infiniteMass() )
//				{
//					assert( shapeAPtr->mass() > FLT_EPSILON );
//					assert( shapeAPtr->mass() < FLT_MAX );
//
//					reciprocalMassSum += ( 1.0f / shapeAPtr->mass() );
//					reciprocalInirtiaSum += ( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
//				}
//				if( !shapeBPtr->infiniteMass() )
//				{
//					assert( shapeBPtr->mass() > FLT_EPSILON );
//					assert( shapeBPtr->mass() < FLT_MAX );
//
//					reciprocalMassSum += ( 1.0f / shapeBPtr->mass() );
//					reciprocalInirtiaSum += ( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
//				}
//
//				float const e = 0.0f;
//
//				const float jDivisor = ( reciprocalMassSum + reciprocalInirtiaSum );
//				assert( fabs( jDivisor ) > FLT_EPSILON );
//				float const j = ( -( 1.0f + e ) * relVelDotNormal ) / jDivisor;
//
//				if( !shapeAPtr->infiniteMass() )
//				{
//					assert( shapeAPtr->mass() > FLT_EPSILON );
//					assert( shapeAPtr->mass() < FLT_MAX );
//								
//					D3DXVECTOR4 velA2 = shapeAPtr->dynamics().velocity();
//					velA2 += ( ( j / shapeAPtr->mass() ) * collisionUnitNormal );
//						
//					float const angVelA2 = shapeAPtr->dynamics().angularVelocity() + ( ( j * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
//
//					shapeAPtr->setDynamics(
//						shapeAPtr->dynamics().position(),
//						shapeAPtr->dynamics().orientation(),
//						velA2,
//						angVelA2 );
//				}
//
//				if( !shapeBPtr->infiniteMass() )
//				{
//					assert( shapeBPtr->mass() > FLT_EPSILON );
//					assert( shapeBPtr->mass() < FLT_MAX );
//								
//					D3DXVECTOR4 velB2 = shapeBPtr->dynamics().velocity();
//					velB2 -= ( ( j / shapeBPtr->mass() ) * collisionUnitNormal );
//								
//					float const angVelB2 = shapeBPtr->dynamics().angularVelocity() - ( ( j * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
//
//					shapeBPtr->setDynamics(
//						shapeBPtr->dynamics().position(),
//						shapeBPtr->dynamics().orientation(),
//						velB2,
//						angVelB2 );
//				}
//			}
//		}
//	}
//	
//
//	++m_stepCount;
//}



void EngineClass::draw( IDirect3DDevice9 & d3dDevice ) const
{
	d3dDevice.SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );

	for( EngineShapeListType::const_iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
	{
		shapeItr->draw( d3dDevice );
	}
}

int EngineClass::createShape(
	D3DXVECTOR4VectorType const & vertexVector,
	D3DXVECTOR4 const & position,
	float const orientation,
	D3DXVECTOR4 const & velocity,
	float const angularVelocity,
	D3DXVECTOR4 const & force,
	float const torque,
	bool const infiniteMass )
{
	int UID = m_nextUID;
	++m_nextUID;
	
	assert( std::find_if( m_shapeList.begin(), m_shapeList.end(), EngineShapeIdentifyClass( UID ) ) == m_shapeList.end() );
	
	m_shapeList.push_back( ShapeClass( 
		( int )UID,
		vertexVector, 
		position, 
		orientation, 
		velocity, 
		angularVelocity,
		force,
		torque,
		infiniteMass ) );

	return UID;		
}

void EngineClass::destroyShape( const int UID )
{
	EngineShapeListType::iterator const shapeItr = std::find_if( m_shapeList.begin(), m_shapeList.end(), EngineShapeIdentifyClass( UID ) );
	if( shapeItr != m_shapeList.end() )
	{
		m_shapeList.erase( shapeItr );
	}
}

ShapeClass const * EngineClass::shape( const int UID ) const
{
	EngineShapeListType::const_iterator const shapeItr = std::find_if( m_shapeList.begin(), m_shapeList.end(), EngineShapeIdentifyClass( UID ) );
	if( shapeItr != m_shapeList.end() )
	{
		return &( *shapeItr );
	}
	return 0;
}

ShapeClass * EngineClass::shape( const int UID )
{
	EngineShapeListType::iterator const shapeItr = std::find_if( m_shapeList.begin(), m_shapeList.end(), EngineShapeIdentifyClass( UID ) );
	if( shapeItr != m_shapeList.end() )
	{
		return &( *shapeItr );
	}
	return 0;
}

bool EngineClass::anyShapesInCollision() const
{
	for( EngineShapeListType::const_iterator shapeAItr = m_shapeList.begin(); shapeAItr != m_shapeList.end(); ++shapeAItr )
	{
		EngineShapeListType::const_iterator shapeBItr = shapeAItr;
		++shapeBItr;
		for( ; shapeBItr != m_shapeList.end(); ++shapeBItr )
		{
			if( shapeCollision( *shapeAItr, *shapeBItr ) )
			{
				return true;
			}
		}
	}

	return false;
}

}


