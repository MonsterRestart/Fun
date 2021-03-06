// Andrew Davies

#include "DXUT.h"
#include "Physics/Engine/physicsEngine.h"
#include "Physics/Feature/physicsFeature.h"
#include <fuz/container/fixed_list.hpp>
#include <fuz/container/fixed_vector.hpp>
#include <algorithm>
#include <vector>
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
	
#define PHYSICS_ENGINE_TOLLERENCE ( 0.0001f ) //* 1000.0f )
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
		}
	}
}

typedef std::vector< int > IdxSetType;
//typedef fuz::fixed_vector< int, PHYSICS_ENGINE_MAX_CONTACTS > IdxSetType;

void EngineClass::step( float const deltaTime )
{
	if( m_stepCount == 105 )
	{
		printf( "poo\n" );
	}

	// Algorithm outline.
	//
	// Assume we start off without any penetration (which may be wrong because the sim could start with some pen) and advance the sim a full delta time.
	//
	// The main loop exits when it sees we have advanced the time up through the full deltatime. It has a safety of 32 loops max. This is hit if we start 
	// with penetration or if the shapes keep having collisions even though the collision response and contact forces 'should' keep them all apart. Right 
	// now this situation results in the simulation being frozen as ALL the shapes get reset to the time of the problematic penatration and do not advance 
	// even though they could. A solution would be to deal with shapes in initial penetration differently.
	//
	// advancedDeltaTime and advancedShapeList are used to store the most advanced state of the sim we have found that is not in collision. The idea is to 
	// use deltaTimeRemaining to move this state on as far as possible. The loop advances m_shapeList and it is reverted back to advancedShapeList if it's 
	// advanced in to collision. advancedShapeList takes a copy of m_shapeList at the start of each loop using the assumption we are never in collision 
	// then. advancedDeltaTime is initialised to 0 and deltaTimeRemaining is initialised to the full deltaTime. advancedDeltaTime is incremented by 
	// deltaTimeRemaining at the end of each loop that hasn't got any collisions.
	//
	// If there are no collisions the first time through this loop it should drop straight out without attempting to apply collision responses or contact 
	// forces etc. as you would expect. It drops out because advancedDeltaTime gets set to deltaTime, effectively.
	//
	// So the structure of the loop is..
	// copy current m_shapeList as the most advanced non collision state
	// Advance the dynamics of all the shapes by deltaTimeRemaining
	// See if this has moved any shape in to collision with any other by brute force.
	// Collisiondetection is to iterate over each shape against all the others. iterate over all the verticies of the first and find the edge it has 
	// penetrated the furthest though, if any, of the other. Collect data on all collisions.
	// If there are any collisions reset m_shapeList back to the most advanced non colliding state and...
	// We have set a minimum step size/tollerance for deltaTimeRemaining (0.0001f), if we have just tried to advance the simulation by only this amount 
	// and failed because of collisions then we are close enough to say that we have found the time of collision. deltaTimeRemaining typically starts 
	// off set to 1/60 and is halfed each time we find collisions, therefore it will probably take a few loops before it gets down to this tollerance 
	// level.
	// If deltaTimeRemaining is not small enough deltaTimeRemaining is halved and the loop begins again. Remember m_shapeList is reset to the most advanced 
	// safest time, so we try pushing the sim forward by only half the time. This halving will contiue until either it's below the tollerance level (and we 
	// perform more interesting deeds) or we find a time with no collisions...
	//
	// If there are no collisions, m_shapeList is left in the state it has been advanced to so we can leave it like that and set advancedDeltaTime to now 
	// (by adding deltaTimeRemaining). If this was the first time through the loop, the loop would terminate because we have pushed through all of deltaTime. 
	// If it is not the first time the whole algorithim effectively starts again but trying to push the sim forward from this new advanced time with an 
	// initially smaller detaTime. Ultimately it will converge on collision time because of the step tollerance OR possible have looped too many times. What 
	// happens then is...
	// Exit the loop leaving m_shapeList at it's most advanced sim state which could be any time between 0 -> deltaTime. This is why the system freezes if 
	// the sim started in penetration because the loop would not advance m_shapeList beyond its initial state. No collision responce or contact forces would 
	// have been applied (maybe they should have).
	//
	// If we have found the time of collision the first thing we do is apply the appropriate impulses to all the objects involved in all the collisions 
	// (infinite mass shapes do not have impulses applied). We do this first because impulses are instanteaneous changes in momentum which, obviously, means 
	// the objects velocities will change here. We want all the objects to be in their most up to date state before detemining contact and friction forces. 
	// There's no point calcuilating them for objects that will be bouncing apart. So next we calculate these forces. The main idea for contact forces it to 
	// apply forces to the right objcets so thaty the next dynamics update that takes in to account external forces (primarily gravity) will not push the 
	// objects back into collision (as if the table was pushing back on the box just enough to keep it above it)
	//
	// Calculating contact and friction forces is complicated because they all have to be done simulteaneously, changing a force at one contact point will 
	// change the state of a shpe to the point where another contact point it is envolved with can become incorrect.
	//
	// First we need to set up some data that represents how the contacts and external forces relate to each other. We set up matrix A[][]. This shows how 
	// changing the contact force at one collision effects the acceleration at all the others. We set up vetor b[] that represents how external forces such 
	// as gravity effect each contact point.
	//
	// The bulk of the operation involves vectors 'a' (basically 'b' from above), 'v', deltaF and deltaA. But the ultimate output is a vector 'f' which is 
	// the magniutude of the forces to be applied. We iterate over each contact and calculate a step size to increase the force at this contact. Using A[][] 
	// we can generate deltaF and deltaA to modifiy the contact forces and acc's we've already calculated. Each iteration may result in the contact becoming 
	// unclamped which requires vectors in a and values inn v[] to be pivoted.
	//
	// Currently we have vector Cn[] and index 'k' used to show the pivot point and therefore which contacts are clamped and therefore which are unclamped. To 
	// implement static friction I 'll introduce other sets like C and may remove the swaping in favour of re-constructing a matrix for the solver.
	//
	// These forces are applied to the shapes and the loop begins again from it's current most advanced time and tries to simulate forward all the way to the 
	// end of deltaTime in one step.
	//
	// Possible optimisations...
	// Only one loop iteration, if collisions occur, reset state and assume deltaTime == 0.0f is time of collision. 
	//
	// Possible problem solutions.
	// Initial Penetration: either treat penetrated shapes differently OR force collision responces before escaping the loop when ( numLoops < 32 ). This would 
	// require allowing the loop to go on beyond 32 until the next loop it finds contacts and resolves them or storing the last set it found and using them at 
	// the 32nd iteration.

	static EngineShapeListType advancedShapeList;
	float advancedDeltaTime = 0.0f;
	float deltaTimeRemaining = deltaTime;

	// Try to advance the simulation by as much time as possible and keep looping until it has been advanced by 
	// the full delta time. Break out if we can't do this by 32 loops. This break out is the cause of the freeze
	// when there's penetration because we can't find the time of collision because it happened 'before' this 
	// simulation step started. We need some way to treat penetrated objects seperatetly.
	int numLoops = 0;
	do
	{
		++numLoops;
		
		if( numLoops == 9 )
		{
			printf( "poo\n" );
		}

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
			const D3DXVECTOR4 contactNorm = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );	
			const D3DXVECTOR4 contactTang( -contactNorm[ 1 ], contactNorm[ 0 ], contactNorm[ 2 ], contactNorm[ 3 ] );

			DevGraphics::point( ( ( int )contact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + contact.m_vertIdx, contactPos );

			DevGraphics::line( 
				7267 +  ( ( int )contact.m_vertShapePtr * PHYSICS_SHAPE_MAX_VERTS ) + contact.m_vertIdx, 
				contactPos, 
				contactPos + ( contactTang * 5.0f ), 
				D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f ),
				D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f ) );

		}

		if( numContacts > 0 )
		{
			// Gone too far into contact
			// roll back 
			m_shapeList = advancedShapeList;

			// See if the last step we tried was very small. if so we're right at Time of collision.
			if( deltaTimeRemaining <= PHYSICS_ENGINE_TOLLERENCE )
			{				
				//if( numContacts > 1 )
				//{
				//	printf( "poo\n" );
				//}

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
				

				
				// SET UP A MATRIX AND b VECTOR

				// We're aiming to calculate a force at each of the contact points that should 
				// be enough to keep the objects from interpenmatrating but not soo much that 
				// they move apart.

				// Here we populate a 2d matrix and a 	
				//float new_A[ PHYSICS_ENGINE_MAX_CONTACTS * 2 ][ PHYSICS_ENGINE_MAX_CONTACTS * 2 ] = { 0.0f };
				//float new_b[ PHYSICS_ENGINE_MAX_CONTACTS * 2 ] = { 0.0f };
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
					
					//const D3DXVECTOR4 iContactTang( -iContactNorm[ 1 ], iContactNorm[ 0 ], iContactNorm[ 2 ], iContactNorm[ 3 ] );
					//const D3DXVECTOR4 iContactNegTang = -iContactTang;		

	
					for( int jContactIdx = 0; jContactIdx != numContacts; ++jContactIdx )
					{	
						const ContactStruct& jContact = contactVector[ jContactIdx ];
			
						//ni ⋅ (nj / m1 + (rj × nj) × r1 / I1)

						const D3DXVECTOR4 jContactPos = jContact.m_vertShapePtr->transformedVertexPosition( jContact.m_vertIdx );		
						const D3DXVECTOR4 jContactNorm = jContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( jContact.m_edgeIdx );
						const D3DXVECTOR4 jContactNegNorm = -jContactNorm;						
						const D3DXVECTOR4 jContactTang( -jContactNorm[ 1 ], jContactNorm[ 0 ], jContactNorm[ 2 ], jContactNorm[ 3 ] );
						const D3DXVECTOR4 jContactNegTang = -jContactTang;		

						if( !iContact.m_vertShapePtr->infiniteMass() )
						{
							if( iContact.m_vertShapePtr == jContact.m_vertShapePtr )
							{
								assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
								assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
								assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
								assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

								const D3DXVECTOR4 jContactRadius = jContactPos - iContactVertShapeCOMPos;
					
								D3DXVECTOR4 jRadXJNorm;
								D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNorm, &Misc::origin() );															
								//D3DXVECTOR4 jRadXJTang;
								//D3DXVec4Cross( &jRadXJTang, &jContactRadius, &jContactTang, &Misc::origin() );		
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );
								//D3DXVECTOR4 jRadXJTangXIRad1;
								//D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius1, &Misc::origin() );
								
								D3DXVECTOR4 tempNorm = ( jContactNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
								//D3DXVECTOR4 tempTang = ( jContactTang / iContact.m_vertShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
								
								//// Norm on Norm
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempNorm );

								//// Tang on Tang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempTang );
								//
								//// iNorm on jTang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempTang );

								//// iTang on jTang
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempNorm );
								
								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &tempNorm );
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
								//D3DXVECTOR4 jRadXJTang;
								//D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegTang, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );
								//D3DXVECTOR4 jRadXJTangXIRad1;
								//D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius1, &Misc::origin() );
								
								D3DXVECTOR4 tempNorm = ( jContactNegNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
								//D3DXVECTOR4 tempTang = ( jContactNegTang / iContact.m_vertShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
								
								//// Norm on Norm
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempNorm );

								//// Tang on Tang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempTang );
								//
								//// iNorm on jTang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempTang );

								//// iTang on jTang
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempNorm );
					
								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &tempNorm );
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
								//D3DXVECTOR4 jRadXJTang;
								//D3DXVec4Cross( &jRadXJTang, &jContactRadius, &jContactTang, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );
								//D3DXVECTOR4 jRadXJTangXIRad1;
								//D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius2, &Misc::origin() );
								
								D3DXVECTOR4 tempNorm = ( jContactNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );	
								//D3DXVECTOR4 tempTang = ( jContactTang / iContact.m_edgeShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );								
								
								//// Norm on Norm
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );

								//// Tang on Tang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempTang );
								//
								//// iNorm on jTang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempTang );

								//// iTang on jTang
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempNorm );

								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );
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
								//D3DXVECTOR4 jRadXJTang;
								//D3DXVec4Cross( &jRadXJTang, &jContactRadius, &jContactNegTang, &Misc::origin() );			
					
								D3DXVECTOR4 jRadXJNormXIRad1;
								D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );
								//D3DXVECTOR4 jRadXJTangXIRad1;
								//D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius2, &Misc::origin() );
								
								D3DXVECTOR4 tempNorm = ( jContactNegNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );	
								//D3DXVECTOR4 tempTang = ( jContactNegTang / iContact.m_edgeShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );							
								
								//// Norm on Norm
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );

								//// Tang on Tang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempTang );
								//
								//// iNorm on jTang
								//new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempTang );

								//// iTang on jTang
								//new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempNorm );
					
								A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );
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
					//new_b[ ( contactIdx * 2 ) + 0 ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
					//	 ( ( iContactNorm.x * temp.y ) - ( iContactNorm.y * temp.x ) );
					//new_b[ ( contactIdx * 2 ) + 1 ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
					//	 ( ( -iContactTang.y * temp.y ) - ( iContactNorm.x * temp.x ) );
					
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
		
					//new_b[ ( contactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &temp2 );
					//new_b[ ( contactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &temp2 );

					b[ contactIdx ] += D3DXVec4Dot( &iContactNorm, &temp2 );
				}










				
				//float old_A[ PHYSICS_ENGINE_MAX_CONTACTS ][ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
				//float old_b[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
				//memcpy( old_A, A, sizeof( old_A ) );
				//memcpy( old_b, b, sizeof( b ) );


				// COMPUTE FORCES

				// Init all forces to zero.
				float f[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
				//float old_f[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };

				// a[] is speeds at each contact point.
				// Init a to be b
				float* const a = b;
				//float* const old_a = old_b;
	
				float v[ PHYSICS_ENGINE_MAX_CONTACTS ];	
				//float old_v[ PHYSICS_ENGINE_MAX_CONTACTS ];	
				float x[ PHYSICS_ENGINE_MAX_CONTACTS ];
				float deltaF[ PHYSICS_ENGINE_MAX_CONTACTS ];	
				//float old_deltaF[ PHYSICS_ENGINE_MAX_CONTACTS ];	
				float deltaA[ PHYSICS_ENGINE_MAX_CONTACTS ];
				//float old_deltaA[ PHYSICS_ENGINE_MAX_CONTACTS ];

				
				// Spare spce to use during swap
				//float old_swapTemp[ PHYSICS_ENGINE_MAX_CONTACTS ];	
	
				// k is the current pivot point.
				// Contact <=k are clamped with f[i]>0 and a[i]<=0. 	
				// Contact >k are unclamped with f[i]=0 and a[i]>0. 
				//int old_k = 0;	
				//ContactListType::iterator kContactItr = contactList.begin();		
				// This is initialised one element at at time in the loop below
				//int old_C[ PHYSICS_ENGINE_MAX_CONTACTS ];	



				
				IdxSetType Cn;
				IdxSetType NCn;
				//IdxSetType Cf;
				//IdxSetType NCfpos;
				//IdxSetType NCfneg;




				//bool conditionsMet = true;
				//do
				//{
				//	conditionsMet = true;

				// Iterate over all the contacts and drive a to zero if it is less
				// It should be fine to use a 'for()' here even though I'm modifying the list within 
				// because any modifications occur before contactItr therefore it's still alid to 
				// increment and compare against .end().			
				for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
				{
					// Keep looping and trying to drive b to zero
					bool drivenToZero = ( a[ contactIdx ] >= -FLT_EPSILON );
					//bool old_drivenToZero = ( old_a[ contactIdx ] >= -FLT_EPSILON );
					while( drivenToZero == false ) 
					{
						const int numC = ( int )Cn.size();

						ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );  // just zero contactList.size()	
						//ZeroMemory( old_v, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );  // just zero contactList.size()						

						for( int innerContactIdxIdx = 0; innerContactIdxIdx != numC; ++innerContactIdxIdx )
						{	
							v[ innerContactIdxIdx ] = -A[ Cn[ innerContactIdxIdx ] ][ contactIdx ];
						}
						//for( int innerContactIdx = 0; innerContactIdx != old_k; ++innerContactIdx )
						//{	
						//	old_v[ innerContactIdx ] = -old_A[ innerContactIdx ][ contactIdx ];
						//}

						ZeroMemory( x, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );

						ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						deltaF[ contactIdx ] = 1.0f;	
						
						//ZeroMemory( old_deltaF, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//old_deltaF[ contactIdx ] = 1.0f;
			

						float ACC[ PHYSICS_ENGINE_MAX_CONTACTS ][ PHYSICS_ENGINE_MAX_CONTACTS ];
						ZeroMemory( ACC, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS * PHYSICS_ENGINE_MAX_CONTACTS );
						for( int rowCIdx = 0; rowCIdx != numC; ++rowCIdx )
						{	
							for( int colCIdx = 0; colCIdx != numC; ++colCIdx )
							{	
								ACC[ rowCIdx ][ colCIdx ] = A[ Cn[ rowCIdx ] ][ Cn[ colCIdx ] ];
							}
						}
						solve( ACC, v, numC, x );
						
						//solve( old_A, old_v, contactIdx, old_deltaF ); 
						
						for( int CIdx = 0; CIdx != numC; ++CIdx )						
						{
							const int idx = Cn[ CIdx ];
							
							deltaF[ idx ] = x[ CIdx ];
						}

						ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						matrixMultiplyVector( A, deltaF, ( contactIdx + 1 ), deltaA );
						//matrixMultiplyVector( A, deltaF, numContacts, deltaA );
									
						//ZeroMemory( old_deltaA, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//matrixMultiplyVector( old_A, old_deltaF, numContacts, old_deltaA );


						float s = FLT_MAX;
						int j = -1;
						IdxSetType::iterator inCItr = Cn.end();
						IdxSetType::iterator inNCItr = NCn.end();
						bool inC = false;
						bool inNC = false;
						if( deltaA[ contactIdx ] > FLT_EPSILON )
						{
							j = contactIdx;
							s = -a[ contactIdx ] / deltaA[ contactIdx ];
						}
						else
						{
							//assert( false );
						}


						
						//float old_s = FLT_MAX;
						//int old_j = -1;
						//if( old_deltaA[ contactIdx ] > FLT_EPSILON )
						//{
						//	old_j = contactIdx;
						//	old_s = -old_a[ contactIdx ] / old_deltaA[ contactIdx ];
						//}
						//else
						//{
						//	assert( false );
						//}


						for( IdxSetType::iterator CItr = Cn.begin(); CItr != Cn.end(); ++CItr )
						{
							const int idx = *CItr;
							if( deltaF[ idx ] < -FLT_EPSILON )
							{
								const float posS = -f[ idx ] / deltaF[ idx ];
								if( posS < s )
								{
									s = posS;
									j = idx;
									inCItr = CItr;
									inC = true;
								}
							}
						}

						for( IdxSetType::iterator NCItr = NCn.begin(); NCItr != NCn.end(); ++NCItr )
						{
							const int idx = *NCItr;
							if( deltaA[ idx ] < -FLT_EPSILON )
							{
								const float posS = -a[ idx ] / deltaA[ idx ];
								if( posS < s )
								{
									s = posS;
									j = idx;	
									inNCItr = NCItr;
									inNC = true;
								}
							}
						}



						
			
						//int i = 0;		
						//for( ; i != old_k; ++i )
						//{			
						//	if( old_deltaF[ i ] < -FLT_EPSILON )
						//	{
						//		const float posS = -old_f[ i ] / old_deltaF[ i ];
						//		if( posS < old_s )
						//		{
						//			old_s = posS;
						//			old_j = i;		
						//		}
						//	}
						//}

						//for( ; i != contactIdx; ++i )
						//{
						//	if( old_deltaA[ i ] < -FLT_EPSILON )
						//	{
						//		const float posS = -old_a[ i ] / old_deltaA[ i ];
						//		if( posS < old_s )
						//		{
						//			old_s = posS;
						//			old_j = i;			
						//		}
						//	}
						//}








			
						for( int e = 0; e != numContacts; ++e )
						{				
							f[ e ] += s * deltaF[ e ];
							a[ e ] += s * deltaA[ e ];
						}

						
						//for( int e = 0; e != numContacts; ++e )
						//{				
						//	old_f[ e ] += old_s * old_deltaF[ e ];
						//	old_a[ e ] += old_s * old_deltaA[ e ];
						//}



						if( inCItr != Cn.end() )
						{
							Cn.erase( inCItr );
							NCn.push_back( j );
						}
						else if( inNCItr != NCn.end() )
						{
							Cn.push_back( j );
							NCn.erase( inNCItr );
						}
						else
						{
							drivenToZero = true;

							Cn.push_back( contactIdx );
							a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
						}



						
						//if( old_j < old_k ) 
						//{	
						//	memcpy( old_swapTemp, old_A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//	const float aSwapTemp = old_a[ j ];
						//	const float fSwapTemp = old_f[ j ];
						//	const int CSwapTemp = old_C[ j ];
						//	for( int shuffleIdx = ( old_j + 1 ); shuffleIdx != old_k; ++shuffleIdx )
						//	{
						//		memcpy( old_A[ shuffleIdx - 1 ], old_A[ shuffleIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//		old_a[ shuffleIdx - 1 ] = old_a[ shuffleIdx ];
						//		old_f[ shuffleIdx - 1 ] = old_f[ shuffleIdx ];
						//		old_C[ shuffleIdx - 1 ] = old_C[ shuffleIdx ];
						//	}
						//	memcpy( old_A[ old_k - 1 ], old_swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//	old_a[ old_k - 1 ] = aSwapTemp;
						//	old_f[ old_k - 1 ] = fSwapTemp;
						//	old_C[ old_k - 1 ] = CSwapTemp;

						//	for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
						//	{
						//		const float ASwapTemp = old_A[ rowIdx ][ j ];
						//		for( int shuffleIdx = ( old_j + 1 ); shuffleIdx != old_k; ++shuffleIdx )
						//		{
						//			old_A[ rowIdx ][ shuffleIdx - 1 ] = old_A[ rowIdx ][ shuffleIdx ];
						//		}
						//		old_A[ rowIdx ][ old_k - 1 ] = ASwapTemp;
						//	}

						//	--old_k;
						//}
						//else if( old_j < contactIdx )
						//{
						//	memcpy( old_swapTemp, old_A[ j ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//	const float aSwapTemp = old_a[ j ];
						//	const float fSwapTemp = old_f[ j ];
						//	const int CSwapTemp = old_C[ j ];
						//	for( int shuffleIdx = old_j; shuffleIdx != old_k; --shuffleIdx )
						//	{
						//		memcpy( old_A[ shuffleIdx ], old_A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//		old_a[ shuffleIdx ] = old_a[ shuffleIdx - 1 ];
						//		old_f[ shuffleIdx ] = old_f[ shuffleIdx - 1 ];
						//		old_C[ shuffleIdx ] = old_C[ shuffleIdx - 1 ];
						//	}
						//	memcpy( old_A[ old_k ], old_swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//	old_a[ old_k ] = aSwapTemp;
						//	old_f[ old_k ] = fSwapTemp;
						//	old_C[ old_k ] = CSwapTemp;

						//	for( int rowIdx = 0; rowIdx != numContacts; ++rowIdx )
						//	{
						//		const float ASwapTemp = old_A[ rowIdx ][ j ];
						//		for( int shuffleIdx = old_j; shuffleIdx != old_k; --shuffleIdx )
						//		{
						//			old_A[ rowIdx ][ shuffleIdx ] = old_A[ rowIdx ][ shuffleIdx - 1 ];
						//		}
						//		old_A[ rowIdx ][ old_k ] = ASwapTemp;
						//	}

						//	++old_k;
						//}
						//else
						//{
						//	old_a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
						//	old_drivenToZero = true;

						//	memcpy( old_swapTemp, old_A[ contactIdx ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//	const float aSwapTemp = old_a[ contactIdx ];
						//	const float fSwapTemp = old_f[ contactIdx ];
						//	const int CSwapTemp = old_C[ contactIdx ];
						//	for( int shuffleIdx = contactIdx; shuffleIdx != old_k; --shuffleIdx )
						//	{
						//		memcpy( old_A[ shuffleIdx ], old_A[ shuffleIdx - 1 ], sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//		old_a[ shuffleIdx ] = old_a[ shuffleIdx - 1 ];
						//		old_f[ shuffleIdx ] = old_f[ shuffleIdx - 1 ];
						//		old_C[ shuffleIdx ] = old_C[ shuffleIdx - 1 ];
						//	}
						//	memcpy( old_A[ old_k ], old_swapTemp, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
						//	old_a[ old_k ] = aSwapTemp;
						//	old_f[ old_k ] = fSwapTemp;
						//	old_C[ old_k ] = CSwapTemp;

						//	for( int rowIdx = 0; rowIdx != ( contactIdx + 1 ); ++rowIdx )
						//	{
						//		const float ASwapTemp = old_A[ rowIdx ][ contactIdx ];
						//		for( int shuffleIdx = contactIdx; shuffleIdx != old_k; --shuffleIdx )
						//		{
						//			old_A[ rowIdx ][ shuffleIdx ] = old_A[ rowIdx ][ shuffleIdx - 1 ];
						//		}
						//		old_A[ rowIdx ][ old_k ] = ASwapTemp;
						//	}

						//	++old_k;
						//}












					}
				}
				//} while( conditionsMet );


				// Apply/add contact forces.
				// Iterate over all contacts and use the force magnitudes calculated in f[] 
				// above and the geometry of the contact (position + normal) to add force 
				// and torque to both objects (except if the object is infinite mass).
				for( int CIdx = 0; CIdx != ( int )Cn.size(); ++CIdx )						
				{
					const int contactIdx = Cn[ CIdx ];

					const ContactStruct& contact = contactVector[ contactIdx ];

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
	// Might be better to add a bit of tollerence on this time check as we're relying on 
	// addition (or too many increments) to push it to be equal to deltaTime which is 
	// always risky with floating point.

		


	

	++m_stepCount;
}




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


