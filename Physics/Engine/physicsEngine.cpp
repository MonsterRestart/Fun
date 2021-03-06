// Andrew Davies

#include "pch.h"
#include "Physics/Engine/physicsEngine.h"
#include "Physics/Feature/physicsFeature.h"
#include "Dav/container/container.hpp"
//#include <fuz/container/fixed_list.hpp>
//#include <fuz/container/fixed_vector.hpp>
#include <algorithm>
#include <vector>
#include "Misc/misc.h"

#include "DevGraphics/devGraphics.h"

using namespace std;
using namespace DirectX;

namespace Physics
{

// Utility class to be used with stl find and the like, generally, to match a UID
template< class T >
class EngineIdentify
{
private:
    int m_UID;

public:
    EngineIdentify( const int UID )
        : m_UID( UID )
    {
    }

    bool operator()( const T& obj ) const
    {
        return obj.UID() == m_UID;
    }
};


EngineClass::EngineClass()
    : m_nextShapeUID( 0 )
    , m_nextObjectUID( 0 )
    , m_nextHeightMapUID( 0 )
    , m_stepCount( 0 )
{
}

void EngineClass::create()
{
    m_nextShapeUID = 0;
    m_nextObjectUID = 0;
    m_nextHeightMapUID = 0;
    m_shapeList.clear();
    m_objectList.clear();
    m_heightMapList.clear();
    m_stepCount = 0;
}

void EngineClass::destroy()
{
    m_nextShapeUID = 0;
    m_nextObjectUID = 0;
    m_nextHeightMapUID = 0;
    assert( m_shapeList.empty() );
    m_shapeList.clear();
    assert( m_objectList.empty() );
    m_objectList.clear();
    assert( m_heightMapList.empty() );
    m_heightMapList.clear();
    m_stepCount = 0;
}

Shape* shapeInCollision(	
    Shape const & shape,
    EngineShapeListType & engineShapeList )
{
    for( EngineShapeListType::iterator shapeItr = engineShapeList.begin(); shapeItr != engineShapeList.end(); ++shapeItr )
    {
        Shape& otherShape = *shapeItr;

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

//Object * objectInCollision(	
//	const Object& object,
//	EngineObjectListType & engineObjectList )
//{
//	for( EngineObjectListType::iterator objectItr = engineObjectList.begin(); objectItr != engineObjectList.end(); ++objectItr )
//	{
//		Object& otherObject = *objectItr;
//
//		if( &object != &otherObject )
//		{
//			if( objectCollision( object, otherObject ) )
//			{
//				return &otherObject;
//			}
//		}
//	}
//
//	return 0;
//}

//#define PHYSICS_ENGINE_MAX_CONTACTS ( ( int )4 )
#define PHYSICS_ENGINE_MAX_CONTACTS ( ( int )32 )
#define PHYSICS_ENGINE_NEW_MAX_CONTACTS ( PHYSICS_ENGINE_MAX_CONTACTS / 2 )

struct ContactStruct
{
    ContactStruct(
        Shape& vertShape,
        const int vertIdx,
        Shape& edgeShape,
        const int edgeIdx )
        : m_vertShapePtr( &vertShape )
        , m_vertIdx( vertIdx )
        , m_edgeShapePtr( &edgeShape )
        , m_edgeIdx( edgeIdx )
    {
    }

    Shape* m_vertShapePtr;
    int m_vertIdx;
    Shape* m_edgeShapePtr;
    int m_edgeIdx;
};

typedef Dav::Vector< ContactStruct, PHYSICS_ENGINE_NEW_MAX_CONTACTS > ContactVectorType;
//typedef fuz::fixed_vector< ContactStruct, PHYSICS_ENGINE_MAX_CONTACTS > ContactVectorType;
    
#define PHYSICS_ENGINE_TOLLERENCE ( 0.0001f * 1000.0f )
//#define PHYSICS_ENGINE_TOLLERENCE ( 0.0001f ) //* 1000.0f )
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

typedef vector< int > IdxSetType;
//typedef fuz::fixed_vector< int, PHYSICS_ENGINE_MAX_CONTACTS > IdxSetType;


void EngineClass::step( const float deltaTime )
{
    stepObjects( deltaTime );
    stepShapes( deltaTime );
}

void EngineClass::stepObjects( const float deltaTime )
{
    // Advance all objects on from where ever they've been left (end of last frame or last collision resolution)
    for( EngineObjectListType::iterator objectItr = m_objectList.begin(); objectItr != m_objectList.end(); ++objectItr )
    { 
        Object& object = *objectItr;
        object.stepDynamics( deltaTime );
        object.clearForce();
        object.clearTorque();
    }
}

void EngineClass::stepShapes( const float deltaTime )
{
    
    #if 0
    {
        static float KINECT_HAND_MAXIMUM_X_DISTANCE = 1.1f;
        static float KINECT_HAND_MAXIMUM_Y_DISTANCE = 0.6f;
        static float KINECT_HAND_MAXIMUM_Z_DISTANCE = 1.2f; // This is an estimate of how far you could possibly reach along Z measured from the origin.
 
        static float kinectOriginInCameraSpace = 40.f; // This is where, in camera space, the origin of kinect space is.
        static float kinectHandMaximumZInCameraSpace = 200.f; // This is where, in camera space, your maximum reach would get you.
  
        static XMVECTOR handKinectSpacePositions[ NUM_KINECT_HANDS ] =
        {
            XMVECTOR( 0.f, 0.f, 0.f ),                        
            XMVECTOR( 0.f, 0.f, -KINECT_HAND_MAXIMUM_DISTANCE ),
            //XMVECTOR( -0.2f, 0.f, -1.f ),                          
            //XMVECTOR( 0.1f, -0.2f, -0.3f ),
        };
 
        // Scale and translate in the Z (assume flipped and everything is positive)
             
        const float scaleZ = ( kinectHandMaximumZInCameraSpace - kinectOriginInCameraSpace ) / KINECT_HAND_MAXIMUM_DISTANCE;
                    
        XMMATRIX flipZMtx;
        flipZMtx.setIdentity();                                             
        flipZMtx.setK( XMVECTOR( 0.f, 0.f, -1.f ).toFloat4_0() );
 
        XMMATRIX projXMtx;
        projXMtx.setIdentity();
        //                                       w,            z?,           y?,           x?
        Float4 temp = STATIC_FLOAT4(      100.f, 1.f,   0.f,   0.f );
        projXMtx.setK( temp );
        //projXMtx.setK( Float4(   100.f, 1.f,   0.f,   0.f ) );
 
        XMMATRIX scaleZMtx;
        scaleZMtx.setIdentity();                                     
        scaleZMtx.setK( XMVECTOR( 0.f, 0.f, scaleZ ).toFloat4_0() );
 
        XMMATRIX translateZMtx;
        translateZMtx.setIdentity();                                 
        translateZMtx.setT( XMVECTOR( 0.f, 0.f, kinectOriginInCameraSpace ).toFloat4_1() );
                    
        XMVECTOR handKinectSpacePos = handKinectSpacePositions[ handIdx ];
        //XMVECTOR handKinectSpacePos = KINECT_ADAPTER->getSpineRelativeBonePosition( cursorSkeletonIdx, KinectAdapterGetHandBone( ( KinectHandEnum )handIdx ) );
 
        XMVECTOR flippedPos;
        flipZMtx.transformPoint( flippedPos, handKinectSpacePos );
 
        XMVECTOR projedFlippedPos;
        projXMtx.transformPoint( projedFlippedPos, flippedPos );
                    
        XMVECTOR scaledFlippedPos;
        scaleZMtx.transformPoint( scaledFlippedPos, flippedPos );
                    
        XMVECTOR translatedScaledFlippedPos;
        translateZMtx.transformPoint( translatedScaledFlippedPos, scaledFlippedPos );
 
                    
        XMVECTOR handCameraSpacePos = translatedScaledFlippedPos;
        //kinectToCameraMtx.transformPoint( handCameraSpacePos, handKinectSpacePositions[ handIdx ] );
 
 
 
    }
    #endif





    //if( m_stepCount == 105 )
    //{
    //	printf( "poo\n" );
    //}

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
    // Collisiondetection is to iterate over each shape against all the others. iterate over all the vertices of the first and find the edge it has 
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
        
        //if( numLoops == 9 )
        //{
        //	printf( "poo\n" );
        //}

        // Copy current state at time = timeAdvance because we're (possibly repeatedly) trying 
        // to move on from here and may need to revert here if we move into penetration.
        advancedShapeList = m_shapeList;
        
        // Advance all shapes on from where ever they've been left (end of last frame or last collision resolution)
        for( EngineShapeListType::iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
        {
            Shape& shape = *shapeItr;
            shape.stepDynamics( deltaTimeRemaining );
        }

        // See if there are any penetrations
        static ContactVectorType contactVector;
        contactVector.clear();
        for( EngineShapeListType::iterator vertShapeItr = m_shapeList.begin(); vertShapeItr != m_shapeList.end(); ++vertShapeItr )
        {
            Shape& vertShape = *vertShapeItr;
    
            for( EngineShapeListType::iterator edgeShapeItr = m_shapeList.begin(); edgeShapeItr != m_shapeList.end(); ++edgeShapeItr )
            {
                Shape& edgeShape = *edgeShapeItr;

                if( vertShapeItr != edgeShapeItr )
                {
                    const int numVerts = vertShape.numberOfVertices();
                    const int numEdges = edgeShape.numberOfEdges();

                    for( int vertIdx = 0; vertIdx != numVerts; ++vertIdx )
                    {	
                        const XMVECTOR vertPos = vertShape.transformedVertexPosition( vertIdx );	

                        bool inside = true;
                        int witnessEdgeIdx = numEdges;	
                        float witnessDist = -FLT_MAX;
                        for( int edgeIdx = 0; ( edgeIdx != numEdges ) && inside; ++edgeIdx )
                        {							
                            const XMVECTOR edgePos = edgeShape.transformedVertexPosition( edgeIdx );
                            const XMVECTOR edgeToVertDir = vertPos - edgePos;
                            const XMVECTOR edgeUnitNrml = edgeShape.transformedEdgeNormalDirectionUnit( edgeIdx );
                            const float signedDist = XMVectorGetX( XMVector4Dot( edgeUnitNrml, edgeToVertDir ) );
                            //const float signedDist = D3DXVec4Dot( &edgeUnitNrml, &edgeToVertDir );
                                
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

            const XMVECTOR contactPos = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );
            const XMVECTOR contactNorm = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );	
            const XMVECTOR contactTang{ -XMVectorGetY( contactNorm ), XMVectorGetX( contactNorm ), XMVectorGetZ( contactNorm ), XMVectorGetW( contactNorm ) };

            DevGraphics::point( ( PtrToInt( contact.m_vertShapePtr ) * PHYSICS_SHAPE_MAX_VERTS ) + contact.m_vertIdx, contactPos );

            DevGraphics::line( 
                7267 +  ( PtrToInt( contact.m_vertShapePtr ) * PHYSICS_SHAPE_MAX_VERTS ) + contact.m_vertIdx, 
                contactPos, 
                contactPos + ( contactTang * 5.0f ), 
                XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f },
                XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f } );

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
                                                                            
                    Shape* const shapeAPtr = contact.m_vertShapePtr;
                    Shape* const shapeBPtr = contact.m_edgeShapePtr;
                    const XMVECTOR collisionPosition = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );
                    const XMVECTOR collisionUnitNormal = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );

                    XMVECTOR const radiusA = collisionPosition - shapeAPtr->centerOfMassWorldPosition();
                    XMVECTOR const radiusB = collisionPosition - shapeBPtr->centerOfMassWorldPosition();
                    
                    XMVECTOR const radiusTangentA{ -XMVectorGetY( radiusA ), XMVectorGetX( radiusA ), XMVectorGetZ( radiusA ), XMVectorGetW( radiusA ) };
                    XMVECTOR const radiusTangentB{ -XMVectorGetY( radiusB ), XMVectorGetX( radiusB ), XMVectorGetZ( radiusB ), XMVectorGetW( radiusB ) };

                    XMVECTOR const velAP = shapeAPtr->dynamics().velocity() + ( radiusTangentA * shapeAPtr->dynamics().angularVelocity() );
                    XMVECTOR const velBP = shapeBPtr->dynamics().velocity() + ( radiusTangentB * shapeBPtr->dynamics().angularVelocity() );
                    XMVECTOR const relVel = velAP - velBP;
                    
                    const float relVelDotNormal = XMVectorGetX( XMVector4Dot( relVel, collisionUnitNormal ) );
                    //float const relVelDotNormal = D3DXVec4Dot( &relVel, &collisionUnitNormal );
                    
                    const float radiusTangentADotNormal = XMVectorGetX( XMVector4Dot( radiusTangentA, collisionUnitNormal ) );
                    //float const radiusTangentADotNormal = D3DXVec4Dot( &radiusTangentA, &collisionUnitNormal );
                    const float radiusTangentBDotNormal = XMVectorGetX( XMVector4Dot( radiusTangentB, collisionUnitNormal ) );
                    //float const radiusTangentBDotNormal = D3DXVec4Dot( &radiusTangentB, &collisionUnitNormal );

                    float reciprocalMassSum = 0.0f;		
                    float reciprocalInertiaSum = 0.0f;
                    if( !shapeAPtr->infiniteMass() )
                    {
                        assert( shapeAPtr->mass() > FLT_EPSILON );
                        assert( shapeAPtr->mass() < FLT_MAX );

                        reciprocalMassSum += ( 1.0f / shapeAPtr->mass() );
                        reciprocalInertiaSum += ( ( radiusTangentADotNormal * radiusTangentADotNormal ) / shapeAPtr->momentOfInertia() );
                    }
                    if( !shapeBPtr->infiniteMass() )
                    {
                        assert( shapeBPtr->mass() > FLT_EPSILON );
                        assert( shapeBPtr->mass() < FLT_MAX );

                        reciprocalMassSum += ( 1.0f / shapeBPtr->mass() );
                        reciprocalInertiaSum += ( ( radiusTangentBDotNormal * radiusTangentBDotNormal ) / shapeBPtr->momentOfInertia() );
                    }

                    float const e = 0.2f;
                    static float minSpeed = 0.05f;

                    const float jDivisor = ( reciprocalMassSum + reciprocalInertiaSum );
                    assert( fabs( jDivisor ) > FLT_EPSILON );
                    float const j = ( -( 1.0f + e ) * relVelDotNormal ) / jDivisor;

                    if( !shapeAPtr->infiniteMass() )
                    {
                        assert( shapeAPtr->mass() > FLT_EPSILON );
                        assert( shapeAPtr->mass() < FLT_MAX );
                                
                        XMVECTOR velA2 = shapeAPtr->dynamics().velocity();
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
                                
                        XMVECTOR velB2 = shapeBPtr->dynamics().velocity();
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
                float new_A[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ][ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ] = { 0.0f };
                float new_b[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ] = { 0.0f };
                float A[ PHYSICS_ENGINE_MAX_CONTACTS ][ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };
                float b[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };

                for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
                {	
                    ContactStruct& iContact = contactVector[ contactIdx ];

                    const XMVECTOR iContactPos = iContact.m_vertShapePtr->transformedVertexPosition( iContact.m_vertIdx );
                    DevGraphics::point( ( PtrToInt( iContact.m_vertShapePtr ) * PHYSICS_SHAPE_MAX_VERTS ) + iContact.m_vertIdx, iContactPos ); 

                    const XMVECTOR iContactVertShapeCOMPos = iContact.m_vertShapePtr->centerOfMassWorldPosition();
                    //DevGraphics::point( 9923 + ( int )iContact.m_vertShapePtr, iContactVertShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, XMVECTOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
                    const XMVECTOR iContactEdgeShapeCOMPos = iContact.m_edgeShapePtr->centerOfMassWorldPosition();
                    //DevGraphics::point( 12355 + ( int )iContact.m_edgeShapePtr, iContactEdgeShapeCOMPos, DEV_GRAPHICS_DEFAULT_POINT_SIZE, XMVECTOR( 0.0f, 0.0f, 1.0f, 1.0f ) );
        
                    const XMVECTOR iContactRadius1 = iContactPos - iContactVertShapeCOMPos;			
                    //DevGraphics::line( 
                    //	7267 + ( int )iContact.m_vertShapePtr, 
                    //	iContactVertShapeCOMPos, 
                    //	iContactVertShapeCOMPos + iContactRadius1, 
                    //	XMVECTOR( 1.0f, 0.0f, 0.0f, 1.0f ),
                    //	XMVECTOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
                    const XMVECTOR iContactRadius2 = iContactPos - iContactEdgeShapeCOMPos;		
                    //DevGraphics::line( 
                    //	43322 + ( int )iContact.m_edgeShapePtr, 
                    //	iContactEdgeShapeCOMPos, 
                    //	iContactEdgeShapeCOMPos + iContactRadius2, 
                    //	XMVECTOR( 0.0f, 0.0f, 1.0f, 1.0f ),
                    //	XMVECTOR( 0.0f, 0.0f, 1.0f, 1.0f ) );

                    const XMVECTOR iContactNorm = iContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( iContact.m_edgeIdx );		
                    //DevGraphics::line( 55 + ( int )iContact.m_edgeShapePtr, iContactPos, iContactPos + ( iContactNorm * 10.0f ) );
                    const XMVECTOR iContactNegNorm = -iContactNorm;		
                    
                    const XMVECTOR iContactTang{ -XMVectorGetY( iContactNorm ), XMVectorGetX( iContactNorm ), XMVectorGetZ( iContactNorm ), XMVectorGetW( iContactNorm ) };
                    const XMVECTOR iContactNegTang = -iContactTang;

    
                    for( int jContactIdx = 0; jContactIdx != numContacts; ++jContactIdx )
                    {
                        const ContactStruct& jContact = contactVector[ jContactIdx ];
            
                        //ni ⋅ (nj / m1 + (rj × nj) × r1 / I1)

                        const XMVECTOR jContactPos = jContact.m_vertShapePtr->transformedVertexPosition( jContact.m_vertIdx );		
                        const XMVECTOR jContactNorm = jContact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( jContact.m_edgeIdx );
                        const XMVECTOR jContactNegNorm = -jContactNorm;						
                        const XMVECTOR jContactTang{ -XMVectorGetY( jContactNorm ), XMVectorGetX( jContactNorm ), XMVectorGetZ( jContactNorm ), XMVectorGetW( jContactNorm ) };
                        const XMVECTOR jContactNegTang = -jContactTang;		

                        if( !iContact.m_vertShapePtr->infiniteMass() )
                        {
                            if( iContact.m_vertShapePtr == jContact.m_vertShapePtr )
                            {
                                assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
                                assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
                                assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
                                assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

                                const XMVECTOR jContactRadius = jContactPos - iContactVertShapeCOMPos;
                    
                                const XMVECTOR jRadXJNorm = XMVector4Cross( jContactRadius, jContactNorm, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNorm, &Misc::origin() );
                                const XMVECTOR jRadXJTang = XMVector4Cross( jContactRadius, jContactTang, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTang, &jContactRadius, &jContactTang, &Misc::origin() );
                    
                                const XMVECTOR jRadXJNormXIRad1 = XMVector4Cross( jRadXJNorm, iContactRadius1, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );
                                const XMVECTOR jRadXJTangXIRad1 = XMVector4Cross( jRadXJTang, iContactRadius1, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius1, &Misc::origin() );
                                
                                XMVECTOR tempNorm = ( jContactNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
                                XMVECTOR tempTang = ( jContactTang / iContact.m_vertShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
                                
                                // Norm on Norm
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNorm, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempNorm );

                                // Tang on Tang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactTang, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempTang );
                                
                                // iNorm on jTang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNorm, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempTang );

                                // iTang on jTang
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactTang, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempNorm );
                                
                                A[ contactIdx ][ jContactIdx ] += XMVectorGetX( XMVector4Dot( iContactNorm, tempNorm ) );
                                //A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &tempNorm );
                            }
            
                            if( iContact.m_vertShapePtr == jContact.m_edgeShapePtr )
                            {				
                                assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
                                assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
                                assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
                                assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );

                                const XMVECTOR jContactRadius = jContactPos - iContactVertShapeCOMPos;
                    
                                const XMVECTOR jRadXJNorm = XMVector4Cross( jContactRadius, jContactNegNorm, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );
                                const XMVECTOR jRadXJTang = XMVector4Cross( jContactRadius, jContactNegTang, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegTang, &Misc::origin() );
                    
                                const XMVECTOR jRadXJNormXIRad1 = XMVector4Cross( jRadXJNorm, iContactRadius1, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius1, &Misc::origin() );
                                const XMVECTOR jRadXJTangXIRad1 = XMVector4Cross( jRadXJTang, iContactRadius1, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius1, &Misc::origin() );
                                
                                XMVECTOR tempNorm = ( jContactNegNorm / iContact.m_vertShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
                                XMVECTOR tempTang = ( jContactNegTang / iContact.m_vertShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_vertShapePtr->momentOfInertia() );
                                
                                // Norm on Norm
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNorm, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempNorm );

                                // Tang on Tang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactTang, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempTang );
                                
                                // iNorm on jTang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNorm, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &tempTang );

                                // iTang on jTang
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactTang, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &tempNorm );
                    
                                A[ contactIdx ][ jContactIdx ] += XMVectorGetX( XMVector4Dot( iContactNorm, tempNorm ) );
                                //A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNorm, &tempNorm );
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

                                const XMVECTOR jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
                    
                                const XMVECTOR jRadXJNorm = XMVector4Cross( jContactRadius, jContactNorm, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNorm, &Misc::origin() );
                                const XMVECTOR jRadXJTang = XMVector4Cross( jContactRadius, jContactTang, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTang, &jContactRadius, &jContactTang, &Misc::origin() );
                    
                                const XMVECTOR jRadXJNormXIRad1 = XMVector4Cross( jRadXJNorm, iContactRadius2, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );
                                const XMVECTOR jRadXJTangXIRad1 = XMVector4Cross( jRadXJTang, iContactRadius2, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius2, &Misc::origin() );
                                
                                XMVECTOR tempNorm = ( jContactNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );	
                                XMVECTOR tempTang = ( jContactTang / iContact.m_edgeShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );								
                                
                                // Norm on Norm
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNegNorm, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );

                                // Tang on Tang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactNegTang, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempTang );
                                
                                // iNorm on jTang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNegNorm, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempTang );

                                // iTang on jTang
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactNegTang, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempNorm );
                                
                                A[ contactIdx ][ jContactIdx ] += XMVectorGetX( XMVector4Dot( iContactNegNorm, tempNorm ) );
                                //A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );
                            }
            
                            if( iContact.m_edgeShapePtr == jContact.m_edgeShapePtr )
                            {				
                                assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
                                assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
                                assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
                                assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

                                const XMVECTOR jContactRadius = jContactPos - iContactEdgeShapeCOMPos;					
                    
                                const XMVECTOR jRadXJNorm = XMVector4Cross( jContactRadius, jContactNegNorm, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNorm, &jContactRadius, &jContactNegNorm, &Misc::origin() );	
                                const XMVECTOR jRadXJTang = XMVector4Cross( jContactRadius, jContactNegTang, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTang, &jContactRadius, &jContactNegTang, &Misc::origin() );			
                    
                                const XMVECTOR jRadXJNormXIRad1 = XMVector4Cross( jRadXJNorm, iContactRadius2, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJNormXIRad1, &jRadXJNorm, &iContactRadius2, &Misc::origin() );
                                const XMVECTOR jRadXJTangXIRad1 = XMVector4Cross( jRadXJTang, iContactRadius2, Misc::origin() );
                                //D3DXVec4Cross( &jRadXJTangXIRad1, &jRadXJTang, &iContactRadius2, &Misc::origin() );
                                
                                XMVECTOR tempNorm = ( jContactNegNorm / iContact.m_edgeShapePtr->mass() ) + ( jRadXJNormXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );
                                XMVECTOR tempTang = ( jContactNegTang / iContact.m_edgeShapePtr->mass() ) + ( jRadXJTangXIRad1 / iContact.m_edgeShapePtr->momentOfInertia() );
                                
                                // Norm on Norm
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNegNorm, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );

                                // Tang on Tang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactNegTang, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempTang );
                                
                                // iNorm on jTang
                                new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNegNorm, tempTang ) );
                                //new_A[ ( contactIdx * 2 ) + 1 ][ ( jContactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNegNorm, &tempTang );

                                // iTang on jTang
                                new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactNegTang, tempNorm ) );
                                //new_A[ ( contactIdx * 2 ) + 0 ][ ( jContactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactNegTang, &tempNorm );
                    
                                A[ contactIdx ][ jContactIdx ] += XMVectorGetX( XMVector4Dot( iContactNegNorm, tempNorm ) );
                                //A[ contactIdx ][ jContactIdx ] += D3DXVec4Dot( &iContactNegNorm, &tempNorm );
                            }
                        }
                    }
        
                    const XMVECTOR vertShapeAngVel{ 0.0f, 0.0f, iContact.m_vertShapePtr->angularVelocity(), 0.0f };
                    const XMVECTOR edgeShapeAngVel{ 0.0f, 0.0f, iContact.m_edgeShapePtr->angularVelocity(), 0.0f };
        
                    const XMVECTOR vertShapeAngVelCrossRadius1 = XMVector4Cross( vertShapeAngVel, iContactRadius1, Misc::origin() );
                    //D3DXVec4Cross( &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &iContactRadius1, &Misc::origin() );
                    
                    const XMVECTOR edgeShapeAngVelCrossRadius2 = XMVector4Cross( edgeShapeAngVel, iContactRadius2, Misc::origin() );
                    //D3DXVec4Cross( &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &iContactRadius2, &Misc::origin() );
        
                    XMVECTOR const temp =
                        ( iContact.m_vertShapePtr->velocity() + vertShapeAngVelCrossRadius1 ) - 
                        ( iContact.m_edgeShapePtr->velocity() + edgeShapeAngVelCrossRadius2 );					
                    
                    const XMVECTOR edgeShapeAngVelCrossNorm = XMVector4Cross( edgeShapeAngVel, iContactNorm, Misc::origin() );
                    //D3DXVec4Cross( &edgeShapeAngVelCrossNorm, &edgeShapeAngVel, &iContactNorm, &Misc::origin() );
        
                    // Stricktly 2D					
                    new_b[ ( contactIdx * 2 ) + 0 ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
                         ( ( XMVectorGetX( iContactNorm ) * XMVectorGetY( temp ) ) - ( XMVectorGetY( iContactNorm ) * XMVectorGetX( temp ) ) );
                    new_b[ ( contactIdx * 2 ) + 1 ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
                         ( ( -XMVectorGetY( iContactTang ) * XMVectorGetY( temp ) ) - ( XMVectorGetX( iContactNorm ) * XMVectorGetX( temp ) ) );
                    
                    b[ contactIdx ] += 2.0f * iContact.m_edgeShapePtr->angularVelocity() *
                         ( ( XMVectorGetX( iContactNorm ) * XMVectorGetY( temp ) ) - ( XMVectorGetY( iContactNorm ) * XMVectorGetX( temp ) ) );
                    //b[ contactIdx ] += 2.0f * D3DXVec4Dot( &iContactNorm, &temp );
                    //b[ contactIdx ] += 2.0f * D3DXVec4Dot( &edgeShapeAngVelCrossNorm, &temp );



                    XMVECTOR vertShapeAcc = Misc::zero();
                    XMVECTOR vertShapeAngAcc = Misc::zero();
                    if( !iContact.m_vertShapePtr->infiniteMass() )
                    {	
                        assert( iContact.m_vertShapePtr->mass() > FLT_EPSILON );
                        assert( iContact.m_vertShapePtr->mass() < FLT_MAX );
                        assert( iContact.m_vertShapePtr->momentOfInertia() > FLT_EPSILON );
                        assert( iContact.m_vertShapePtr->momentOfInertia() < FLT_MAX );	

                        vertShapeAcc = iContact.m_vertShapePtr->force() / iContact.m_vertShapePtr->mass();
                        XMVectorSetZ( vertShapeAngAcc, iContact.m_vertShapePtr->torque() / iContact.m_vertShapePtr->momentOfInertia() );
                    }
        
                    XMVECTOR edgeShapeAcc = Misc::zero();
                    XMVECTOR edgeShapeAngAcc = Misc::zero();
                    if( !iContact.m_edgeShapePtr->infiniteMass() )
                    {
                        assert( iContact.m_edgeShapePtr->mass() > FLT_EPSILON );
                        assert( iContact.m_edgeShapePtr->mass() < FLT_MAX );
                        assert( iContact.m_edgeShapePtr->momentOfInertia() > FLT_EPSILON );
                        assert( iContact.m_edgeShapePtr->momentOfInertia() < FLT_MAX );

                        edgeShapeAcc = iContact.m_edgeShapePtr->force() / iContact.m_edgeShapePtr->mass();
                        XMVectorSetZ( edgeShapeAngAcc, iContact.m_edgeShapePtr->torque() / iContact.m_edgeShapePtr->momentOfInertia() );
                    }
        
                    const XMVECTOR vertShapeAngAccCrossRadius1 = XMVector4Cross( vertShapeAngAcc, iContactRadius1, Misc::origin() );
                    //D3DXVec4Cross( &vertShapeAngAccCrossRadius1, &vertShapeAngAcc, &iContactRadius1, &Misc::origin() );
                    
                    const XMVECTOR edgeShapeAngAccCrossRadius2 = XMVector4Cross( edgeShapeAngAcc, iContactRadius2, Misc::origin() );
                    //D3DXVec4Cross( &edgeShapeAngAccCrossRadius2, &edgeShapeAngAcc, &iContactRadius2, &Misc::origin() );
        
                    const XMVECTOR vertShapeAngVelCrossVertShapeAngVelCrossRadius1 = XMVector4Cross( vertShapeAngVelCrossRadius1, vertShapeAngVel, Misc::origin() );
                    //D3DXVec4Cross( &vertShapeAngVelCrossVertShapeAngVelCrossRadius1,  &vertShapeAngVelCrossRadius1, &vertShapeAngVel, &Misc::origin() );
                    
                    const XMVECTOR edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2 = XMVector4Cross( edgeShapeAngVelCrossRadius2, edgeShapeAngVel, Misc::origin() );
                    //D3DXVec4Cross( &edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2, &edgeShapeAngVelCrossRadius2, &edgeShapeAngVel, &Misc::origin() );
        
                    XMVECTOR const temp2 =
                        ( vertShapeAcc + vertShapeAngAccCrossRadius1 + vertShapeAngVelCrossVertShapeAngVelCrossRadius1 ) -
                        ( edgeShapeAcc + edgeShapeAngAccCrossRadius2 + edgeShapeAngVelCrossEdgeShapeAngVelCrossRadius2 );
        
                    new_b[ ( contactIdx * 2 ) + 0 ] += XMVectorGetX( XMVector4Dot( iContactNorm, temp2 ) );
                    //new_b[ ( contactIdx * 2 ) + 0 ] += D3DXVec4Dot( &iContactNorm, &temp2 );
                    new_b[ ( contactIdx * 2 ) + 1 ] += XMVectorGetX( XMVector4Dot( iContactTang, temp2 ) );
                    //new_b[ ( contactIdx * 2 ) + 1 ] += D3DXVec4Dot( &iContactTang, &temp2 );
                    
                    b[ contactIdx ] += XMVectorGetX( XMVector4Dot( iContactNorm, temp2 ) );
                    //b[ contactIdx ] += D3DXVec4Dot( &iContactNorm, &temp2 );
                }
                
                float new_f[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ] = { 0.0f };
                float f[ PHYSICS_ENGINE_NEW_MAX_CONTACTS ] = { 0.0f };

                if( numContacts > 2 )
                {
                    printf( "poo\n" );
                }


#if 1
                {

                    // COMPUTE FORCES

                    // Init all forces to zero.
                    //float f[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ] = { 0.0f };

                    // a[] is speeds at each contact point.
                    // Init a to be b
                    float* const new_a = new_b;

                    float ACC[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ][ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ];							
    
                    float v[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ];	
                    //float x[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ];
                    float deltaF[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ];
                    float deltaA[ PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 ];
                
                    IdxSetType Cn;
                    IdxSetType NCn;
                    IdxSetType Cf;
                    IdxSetType NCfpos;
                    IdxSetType NCfneg;

                    bool conditionsMet = true;
                    do
                    {
                        conditionsMet = true;

                        // Iterate over all the contacts and drive a to zero if it is less
                        // It should be fine to use a 'for()' here even though I'm modifying the list within 
                        // because any modifications occur before contactItr therefore it's still alid to 
                        // increment and compare against .end().			
                        for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
                        {

                            // NORMAL NORMAL NORMAL NORMAL
                            // NORMAL NORMAL NORMAL NORMAL
                            // NORMAL NORMAL NORMAL NORMAL
                            // Satisfy normal conditions i.e. Drive normal acc to zero
                            {	

                                bool drivenToZero = ( new_a[ ( contactIdx * 2 ) + 0 ] >= -FLT_EPSILON );
                                while( drivenToZero == false ) 
                                {
                                    ZeroMemory( ACC, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );									
                                    ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );
                                    

                                    for( int rowCIdx = 0; rowCIdx != contactIdx; ++rowCIdx )
                                    {	
                                        if( find( Cn.begin(), Cn.end(), rowCIdx ) != Cn.end() )
                                        {
                                            for( int colCIdx = 0; colCIdx != contactIdx; ++colCIdx )
                                            {
                                                if( find( Cn.begin(), Cn.end(), colCIdx ) != Cn.end() )
                                                {
                                                    ACC[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 0 ] = new_A[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 0 ];
                                                    //ACC[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 0 ] = new_A[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 0 ];
                                                    ACC[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 1 ] = new_A[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 1 ];
                                                    //ACC[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 1 ] = new_A[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 1 ];
                                                }
                                            }
                                            
                                            v[ ( rowCIdx * 2 ) + 0 ] = -new_A[ ( rowCIdx * 2 ) + 0 ][ ( contactIdx * 2 ) + 0 ];
                                            //v[ ( rowCIdx * 2 ) + 1 ] = -new_A[ ( rowCIdx * 2 ) + 1 ][ ( contactIdx * 2 ) + 0 ];
                                        }
                                    }

                                    ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );
                                    deltaF[ ( contactIdx * 2 ) + 0 ] = 1.0f;

                                    solve( ACC, v, ( contactIdx * 2 ), deltaF );

                                    ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );
                                    matrixMultiplyVector( new_A, deltaF, ( contactIdx * 2 ) + 1, deltaA );

                                    float s = FLT_MAX;
                                    int j = -1;
                                    IdxSetType::iterator inCnItr = Cn.end();
                                    IdxSetType::iterator inNCnItr = NCn.end();
                                    IdxSetType::iterator inCfItr = Cf.end();
                                    IdxSetType::iterator inNCfposItr = NCfpos.end();
                                    IdxSetType::iterator inNCfnegItr = NCfneg.end();
                                    bool inCn = false;
                                    bool inNCn = false;
                                    bool inCf = false;
                                    bool inNCfpos = false;
                                    bool inNCfneg = false;
                                    if( deltaA[ ( contactIdx * 2 ) + 0 ] > FLT_EPSILON )
                                    {
                                        j = contactIdx;
                                        s = -new_a[ ( contactIdx * 2 ) + 0 ] / deltaA[ ( contactIdx * 2 ) + 0 ];
                                    }
                                    else
                                    {
                                        //assert( false );
                                    }

                                    for( IdxSetType::iterator CnItr = Cn.begin(); CnItr != Cn.end(); ++CnItr )
                                    {
                                        const int idx = ( *CnItr * 2 ) + 0;
                                        if( deltaF[ idx ] < -FLT_EPSILON )
                                        {
                                            const float posS = -new_f[ idx ] / deltaF[ idx ];
                                            if( posS < s )
                                            {
                                                s = posS;
                                                j = *CnItr;
                                                inCnItr = CnItr;
                                                inCn = true;
                                            }
                                        }
                                    }

                                    for( IdxSetType::iterator NCnItr = NCn.begin(); NCnItr != NCn.end(); ++NCnItr )
                                    {
                                        const int idx = ( *NCnItr * 2 ) + 0;
                                        if( deltaA[ idx ] < -FLT_EPSILON )
                                        {
                                            const float posS = -new_a[ idx ] / deltaA[ idx ];
                                            if( posS < s )
                                            {
                                                s = posS;
                                                j = *NCnItr;	
                                                inNCnItr = NCnItr;
                                                inNCn = true;
                                            }
                                        }
                                    }

                                    for( IdxSetType::iterator CfItr = Cf.begin(); CfItr != Cf.end(); ++CfItr )
                                    {
                                        const int idx = ( *CfItr * 2 ) + 1;
                                        if( deltaF[ idx ] < -FLT_EPSILON )
                                        {
                                            const float posS = -new_f[ idx ] / deltaF[ idx ];
                                            if( posS < s )
                                            {
                                                s = posS;
                                                j = *CfItr;
                                                inCfItr = CfItr;
                                                inCf = true;
                                            }
                                        }
                                    }

                                    for( IdxSetType::iterator NCfposItr = NCfpos.begin(); NCfposItr != NCfpos.end(); ++NCfposItr )
                                    {
                                        const int idx = ( *NCfposItr * 2 ) + 1;
                                        if( deltaA[ idx ] < -FLT_EPSILON )
                                        {
                                            const float posS = -new_a[ idx ] / deltaA[ idx ];
                                            if( posS < s )
                                            {
                                                s = posS;
                                                j = *NCfposItr;	
                                                inNCfposItr = NCfposItr;
                                                inNCfpos = true;
                                            }
                                        }
                                    }

                                    for( IdxSetType::iterator NCfnegItr = NCfneg.begin(); NCfnegItr != NCfneg.end(); ++NCfnegItr )
                                    {
                                        const int idx = ( *NCfnegItr * 2 ) + 1;
                                        if( deltaA[ idx ] < -FLT_EPSILON )
                                        {
                                            const float posS = -new_a[ idx ] / deltaA[ idx ];
                                            if( posS < s )
                                            {
                                                s = posS;
                                                j = *NCfnegItr;	
                                                inNCfnegItr = NCfnegItr;
                                                inNCfneg = true;
                                            }
                                        }
                                    }
            
                                    for( int e = 0; e != ( numContacts * 2 ); ++e )
                                    {				
                                        new_f[ e ] += s * deltaF[ e ];
                                        new_a[ e ] += s * deltaA[ e ];
                                    }

                                    if( inCnItr != Cn.end() )
                                    {
                                        Cn.erase( inCnItr );
                                        NCn.push_back( j );
                                    }
                                    else if( inNCnItr != NCn.end() )
                                    {
                                        Cn.push_back( j );
                                        NCn.erase( inNCnItr );
                                    }
                                    else if( inCfItr != Cf.end() )
                                    {
                                        Cf.erase( inCfItr );

                                        // How to decide?										
                                        NCfpos.push_back( j );
                                    }
                                    else if( inNCfposItr != NCfpos.end() )
                                    {
                                        Cf.push_back( j );											
                                        NCfpos.erase( inNCfposItr );
                                    }
                                    else if( inNCfnegItr != NCfneg.end() )
                                    {
                                        Cf.push_back( j );											
                                        NCfneg.erase( inNCfnegItr );
                                    }
                                    else
                                    {
                                        drivenToZero = true;

                                        Cn.push_back( contactIdx );
                                        new_a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
                                    }
                                }
                            }
                            // NORMAL NORMAL NORMAL NORMAL
                            // NORMAL NORMAL NORMAL NORMAL
                            // NORMAL NORMAL NORMAL NORMAL

                            // FRICTION FRICTION FRICTION FRICTION
                            // FRICTION FRICTION FRICTION FRICTION
                            // FRICTION FRICTION FRICTION FRICTION
                            // Satisfy friction conditions, either get tang acc to zero or max the force out to +/- Fcoef * normalF
                            {

                                // We only need to calculate friction if this point is normal clamped and there is some tangental acceleration to counter
                                // Shouldn't need search for this index as we should know if it's clamped or not from the last bit of code.
                                if( ( find( Cn.begin(), Cn.end(), contactIdx ) != Cn.end() ) && 
                                    ( fabs( new_f[ ( contactIdx * 2 ) + 0 ] ) > FLT_EPSILON ) && 
                                    ( fabs( new_a[ ( contactIdx * 2 ) + 1 ] ) > FLT_EPSILON ) )
                                {
                                    static float coefFric = 1.0f;
                                    
                                    bool aDrivenToZero = ( new_a[ ( contactIdx * 2 ) + 1 ] >= -FLT_EPSILON );
                                    bool fDrivenToMax = ( fabs( new_f[ ( contactIdx * 2 ) + 1 ] ) >= ( coefFric * fabs( new_f[ ( contactIdx * 2 ) + 0 ] ) ) );
                                    while( ( aDrivenToZero == false ) && ( fDrivenToMax == false ) )
                                    {
                                        //ZeroMemory( ACC, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );									
                                        //ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );
                                        //

                                        //for( int rowCIdx = 0; rowCIdx != contactIdx; ++rowCIdx )
                                        //{	

                                        //	if( find( Cn.begin(), Cn.end(), rowCIdx ) != Cn.end() )
                                        //	{
                                        //		for( int colCIdx = 0; colCIdx != contactIdx; ++colCIdx )
                                        //		{
                                        //			if( find( Cn.begin(), Cn.end(), colCIdx ) != Cn.end() )
                                        //			{
                                        //				ACC[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 0 ] = new_A[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 0 ];
                                        //				ACC[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 0 ] = new_A[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 0 ];
                                        //				ACC[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 1 ] = new_A[ ( rowCIdx * 2 ) + 0 ][ ( colCIdx * 2 ) + 1 ];
                                        //				ACC[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 1 ] = new_A[ ( rowCIdx * 2 ) + 1 ][ ( colCIdx * 2 ) + 1 ];

                                        //			}
                                        //			else if( find( NCn.begin(), NCn.end(), colCIdx ) != NCn.end() )
                                        //			{
                                        //			}
                                        //			else
                                        //			{
                                        //			}
                                        //		}			
                                        //		
                                        //		v[ ( rowCIdx * 2 ) + 0 ] = -new_A[ ( rowCIdx * 2 ) + 0 ][ ( contactIdx * 2 ) + 0 ];
                                        //		v[ ( rowCIdx * 2 ) + 1 ] = -new_A[ ( rowCIdx * 2 ) + 1 ][ ( contactIdx * 2 ) + 0 ];

                                        //	}
                                        //	else if( find( NCn.begin(), NCn.end(), rowCIdx ) != NCn.end() )
                                        //	{
                                        //	}
                                        //	else
                                        //	{
                                        //	}
                                        //	
                                        //}

                                        ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );
                                        deltaF[ ( contactIdx * 2 ) + 1 ] = ( ( new_a[ ( contactIdx * 2 ) + 1 ] > 0.0f ) ? 1.0f : -1.0f );

                                        //solve( ACC, v, ( contactIdx * 2 ), deltaF );

                                        //ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_NEW_MAX_CONTACTS * 2 );
                                        //matrixMultiplyVector( new_A, deltaF, ( contactIdx * 2 ) + 1, deltaA );

                                        //float s = FLT_MAX;
                                        //int j = -1;
                                        //IdxSetType::iterator inCnItr = Cn.end();
                                        //IdxSetType::iterator inNCnItr = NCn.end();
                                        //IdxSetType::iterator inCfItr = Cf.end();
                                        //IdxSetType::iterator inNCfposItr = NCfpos.end();
                                        //IdxSetType::iterator inNCfnegItr = NCfneg.end();
                                        //bool inCn = false;
                                        //bool inNCn = false;
                                        //bool inCf = false;
                                        //bool inNCfpos = false;
                                        //bool inNCfneg = false;
                                        //if( deltaA[ ( contactIdx * 2 ) + 0 ] > FLT_EPSILON )
                                        //{
                                        //	j = contactIdx;
                                        //	s = -new_a[ ( contactIdx * 2 ) + 0 ] / deltaA[ ( contactIdx * 2 ) + 0 ];
                                        //}
                                        //else
                                        //{
                                        //	//assert( false );
                                        //}

                                        //for( IdxSetType::iterator CnItr = Cn.begin(); CnItr != Cn.end(); ++CnItr )
                                        //{
                                        //	const int idx = ( *CnItr * 2 ) + 0;
                                        //	if( deltaF[ idx ] < -FLT_EPSILON )
                                        //	{
                                        //		const float posS = -new_f[ idx ] / deltaF[ idx ];
                                        //		if( posS < s )
                                        //		{
                                        //			s = posS;
                                        //			j = *CnItr;
                                        //			inCnItr = CnItr;
                                        //			inCn = true;
                                        //		}
                                        //	}
                                        //}

                                        //for( IdxSetType::iterator NCnItr = NCn.begin(); NCnItr != NCn.end(); ++NCnItr )
                                        //{
                                        //	const int idx = ( *NCnItr * 2 ) + 0;
                                        //	if( deltaA[ idx ] < -FLT_EPSILON )
                                        //	{
                                        //		const float posS = -new_a[ idx ] / deltaA[ idx ];
                                        //		if( posS < s )
                                        //		{
                                        //			s = posS;
                                        //			j = *NCnItr;	
                                        //			inNCnItr = NCnItr;
                                        //			inNCn = true;
                                        //		}
                                        //	}
                                        //}

                                        //for( IdxSetType::iterator CfItr = Cf.begin(); CfItr != Cf.end(); ++CfItr )
                                        //{
                                        //	const int idx = ( *CfItr * 2 ) + 1;
                                        //	if( deltaF[ idx ] < -FLT_EPSILON )
                                        //	{
                                        //		const float posS = -new_f[ idx ] / deltaF[ idx ];
                                        //		if( posS < s )
                                        //		{
                                        //			s = posS;
                                        //			j = *CfItr;
                                        //			inCfItr = CfItr;
                                        //			inCf = true;
                                        //		}
                                        //	}
                                        //}

                                        //for( IdxSetType::iterator NCfposItr = NCfpos.begin(); NCfposItr != NCfpos.end(); ++NCfposItr )
                                        //{
                                        //	const int idx = ( *NCfposItr * 2 ) + 1;
                                        //	if( deltaA[ idx ] < -FLT_EPSILON )
                                        //	{
                                        //		const float posS = -new_a[ idx ] / deltaA[ idx ];
                                        //		if( posS < s )
                                        //		{
                                        //			s = posS;
                                        //			j = *NCfposItr;	
                                        //			inNCfposItr = NCfposItr;
                                        //			inNCfpos = true;
                                        //		}
                                        //	}
                                        //}

                                        //for( IdxSetType::iterator NCfnegItr = NCfneg.begin(); NCfnegItr != NCfneg.end(); ++NCfnegItr )
                                        //{
                                        //	const int idx = ( *NCfnegItr * 2 ) + 1;
                                        //	if( deltaA[ idx ] < -FLT_EPSILON )
                                        //	{
                                        //		const float posS = -new_a[ idx ] / deltaA[ idx ];
                                        //		if( posS < s )
                                        //		{
                                        //			s = posS;
                                        //			j = *NCfnegItr;	
                                        //			inNCfnegItr = NCfnegItr;
                                        //			inNCfneg = true;
                                        //		}
                                        //	}
                                        //}
            
                                        //for( int e = 0; e != ( numContacts * 2 ); ++e )
                                        //{				
                                        //	new_f[ e ] += s * deltaF[ e ];
                                        //	new_a[ e ] += s * deltaA[ e ];
                                        //}

                                        //if( inCnItr != Cn.end() )
                                        //{
                                        //	Cn.erase( inCnItr );
                                        //	NCn.push_back( j );
                                        //}
                                        //else if( inNCnItr != NCn.end() )
                                        //{
                                        //	Cn.push_back( j );
                                        //	NCn.erase( inNCnItr );
                                        //}
                                        //else if( inCfItr != Cf.end() )
                                        //{
                                        //	Cf.erase( inCfItr );

                                        //	// How to decide?										
                                        //	NCfpos.push_back( j );
                                        //}
                                        //else if( inNCfposItr != NCfpos.end() )
                                        //{
                                        //	Cf.push_back( j );											
                                        //	NCfpos.erase( inNCfposItr );
                                        //}
                                        //else if( inNCfnegItr != NCfneg.end() )
                                        //{
                                        //	Cf.push_back( j );											
                                        //	NCfneg.erase( inNCfnegItr );
                                        //}
                                        //else
                                        //{
                                            aDrivenToZero = true;
                                            fDrivenToMax = true;
                                
                                        //	drivenToZero = true;

                                        //	Cn.push_back( contactIdx );
                                        //	new_a[ contactIdx ] = 0.0f;	// assert( a[ contactIdx ] >= -FLT_EPSILON );
                                        //}
                                    }
                                }



                            }
                            // FRICTION FRICTION FRICTION FRICTION
                            // FRICTION FRICTION FRICTION FRICTION
                            // FRICTION FRICTION FRICTION FRICTION







                        }

                    } while( conditionsMet == false );
                }

#endif







#if 1
                {
                    // COMPUTE FORCES

                    // Init all forces to zero.
                    //float f[ PHYSICS_ENGINE_MAX_CONTACTS ] = { 0.0f };

                    // a[] is speeds at each contact point.
                    // Init a to be b
                    //float* const new_a = new_b;
                    float* const a = b;
    
                    float v[ PHYSICS_ENGINE_MAX_CONTACTS ];	
                    float x[ PHYSICS_ENGINE_MAX_CONTACTS ];
                    float deltaF[ PHYSICS_ENGINE_MAX_CONTACTS ];
                    float deltaA[ PHYSICS_ENGINE_MAX_CONTACTS ];
                
                    IdxSetType Cn;
                    IdxSetType NCn;
                    //IdxSetType Cf;
                    //IdxSetType NCfpos;
                    //IdxSetType NCfneg;

                    // Iterate over all the contacts and drive a to zero if it is less
                    // It should be fine to use a 'for()' here even though I'm modifying the list within 
                    // because any modifications occur before contactItr therefore it's still alid to 
                    // increment and compare against .end().			
                    for( int contactIdx = 0; contactIdx != numContacts; ++contactIdx )
                    {
                        // Keep looping and trying to drive b to zero
                        bool drivenToZero = ( a[ contactIdx ] >= -FLT_EPSILON );
                        while( drivenToZero == false ) 
                        {
                            const int numC = ( int )Cn.size();

                            ZeroMemory( v, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );  // just zero contactList.size()	

                            for( int innerContactIdxIdx = 0; innerContactIdxIdx != numC; ++innerContactIdxIdx )
                            {	
                                v[ innerContactIdxIdx ] = -A[ Cn[ innerContactIdxIdx ] ][ contactIdx ];
                            }

                            ZeroMemory( x, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );

                            ZeroMemory( deltaF, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
                            deltaF[ contactIdx ] = 1.0f;			

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
                        
                            for( int CIdx = 0; CIdx != numC; ++CIdx )						
                            {
                                const int idx = Cn[ CIdx ];
                            
                                deltaF[ idx ] = x[ CIdx ];
                            }

                            ZeroMemory( deltaA, sizeof( float ) * PHYSICS_ENGINE_MAX_CONTACTS );
                            matrixMultiplyVector( A, deltaF, ( contactIdx + 1 ), deltaA );
                            //matrixMultiplyVector( A, deltaF, numContacts, deltaA );

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
            
                            for( int e = 0; e != numContacts; ++e )
                            {				
                                f[ e ] += s * deltaF[ e ];
                                a[ e ] += s * deltaA[ e ];
                            }

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
                        }
                    }


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
        
                        const XMVECTOR contactPos = contact.m_vertShapePtr->transformedVertexPosition( contact.m_vertIdx );	
                        const XMVECTOR contactNorm = contact.m_edgeShapePtr->transformedEdgeNormalDirectionUnit( contact.m_edgeIdx );	

                        static float fakeMult = 1.0f;

                        //assert( f[ contactIdx ] == new_f[ ( contactIdx * 2 ) + 0 ] );

                        if( !contact.m_vertShapePtr->infiniteMass() )
                        {
                            contact.m_vertShapePtr->addForceActingAtWorldPosition( f[ contactIdx ] * fakeMult * contactNorm, contactPos );

                            DevGraphics::line( 
                                561 + PtrToInt( contact.m_vertShapePtr ) + PtrToInt( &contact ),
                                contactPos, 
                                contactPos + ( f[ contactIdx ] * contactNorm * 0.02f ), 
                                XMVECTOR{ 1.0f, 0.0f, 0.0f, 1.0f },
                                XMVECTOR{ 1.0f, 0.0f, 0.0f, 1.0f } );
                        }

                        if( !contact.m_edgeShapePtr->infiniteMass() )
                        {
                            contact.m_edgeShapePtr->addForceActingAtWorldPosition( -f[ contactIdx ] * fakeMult * contactNorm, contactPos );

                            DevGraphics::line( 
                                99812 + PtrToInt( contact.m_edgeShapePtr ) + PtrToInt( &contact ),
                                contactPos, 
                                contactPos + ( -f[ contactIdx ] * contactNorm * 0.02f ), 
                                XMVECTOR{ 1.0f, 0.0f, 0.0f, 1.0f },
                                XMVECTOR{ 1.0f, 0.0f, 0.0f, 1.0f } );
                        }
                    }
                }
#endif


                

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

    for( EngineShapeListType::iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
    {
        Shape& shape = *shapeItr;
        shape.clearForce();
        shape.clearTorque();
    }
    
    ++m_stepCount;
}

void EngineClass::draw(
    GraphicsContext& gfxContext,
    const Math::Matrix4& ViewProjMat ) const
{
    //for( EngineShapeListType::const_iterator shapeItr = m_shapeList.begin(); shapeItr != m_shapeList.end(); ++shapeItr )
    //{
    //    shapeItr->draw(); // d3dDevice );
    //}

    for( EngineObjectListType::const_iterator objectItr = m_objectList.begin(); objectItr != m_objectList.end(); ++objectItr )
    {
        objectItr->draw( gfxContext, ViewProjMat );
    }

    //for( EngineHeightMapList::const_iterator heightMapItr = m_heightMapList.begin(); heightMapItr != m_heightMapList.end(); ++heightMapItr )
    //{
    //    heightMapItr->draw(); // d3dDevice );
    //}
}

int EngineClass::createShape(
    XMVECTORVectorType const & vertexVector,
    XMVECTOR const & position,
    float const orientation,
    XMVECTOR const & velocity,
    float const angularVelocity,
    XMVECTOR const & force,
    float const torque,
    bool const infiniteMass )
{
    int UID = m_nextShapeUID;
    ++m_nextShapeUID;
    
    assert( find_if( m_shapeList.begin(), m_shapeList.end(), EngineIdentify< Shape >( UID ) ) == m_shapeList.end() );
    
    m_shapeList.push_back( Shape(
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
    EngineShapeListType::iterator const shapeItr = find_if( m_shapeList.begin(), m_shapeList.end(), EngineIdentify< Shape >( UID ) );
    if( shapeItr != m_shapeList.end() )
    {
        m_shapeList.erase( shapeItr );
    }
}

const Shape* EngineClass::shape( const int UID ) const
{
    EngineShapeListType::const_iterator const shapeItr = find_if( m_shapeList.begin(), m_shapeList.end(), EngineIdentify< Shape >( UID ) );
    if( shapeItr != m_shapeList.end() )
    {
        return &( *shapeItr );
    }
    return 0;
}

Shape* EngineClass::shape( const int UID )
{
    EngineShapeListType::iterator const shapeItr = find_if( m_shapeList.begin(), m_shapeList.end(), EngineIdentify< Shape >( UID ) );
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

int EngineClass::createObject(
    const float mass,
    const bool infiniteMass,
    const XMVECTOR& centerOfMassLocalPosition,
    const XMVECTOR& inertia,
    const Dynamics& dynamics,
    const XMVECTOR& force,
    const XMVECTOR& torque,
    const Object::Vertices& vertices,
    const Object::Edges& edges,
    const Object::Triangles& triangles )
    //const Graphics::Model& model )
{
    int UID = m_nextObjectUID;
    ++m_nextObjectUID;
    
    assert( find_if( m_objectList.begin(), m_objectList.end(), EngineIdentify< Object >( UID ) ) == m_objectList.end() );
    
    m_objectList.push_back( Object() );
    m_objectList.back().create( ( int )UID, mass, infiniteMass, centerOfMassLocalPosition,
        inertia, dynamics, force, torque, vertices, edges, triangles ); // , model );

    return UID;
}

void EngineClass::destroyObject( const int UID )
{
    EngineObjectListType::iterator const objectItr = find_if( m_objectList.begin(), m_objectList.end(), EngineIdentify< Object >( UID ) );
    if( objectItr != m_objectList.end() )
    {
        objectItr->destroy();
        m_objectList.erase( objectItr );
    }
}

const Object* EngineClass::object( const int UID ) const
{
    EngineObjectListType::const_iterator const objectItr = find_if( m_objectList.begin(), m_objectList.end(), EngineIdentify< Object >( UID ) );
    if( objectItr != m_objectList.end() )
    {
        return &( *objectItr );
    }
    return 0;
}

Object* EngineClass::object( const int UID )
{
    EngineObjectListType::iterator const objectItr = find_if( m_objectList.begin(), m_objectList.end(), EngineIdentify< Object >( UID ) );
    if( objectItr != m_objectList.end() )
    {
        return &( *objectItr );
    }
    return 0;
}

//bool EngineClass::anyObjectsInCollision() const
//{
//  for( EngineObjectListType::const_iterator objectAItr = m_objectList.begin(); objectAItr != m_objectList.end(); ++objectAItr )
//  {
//      EngineObjectListType::const_iterator objectBItr = objectAItr;
//      ++objectBItr;
//      for( ; objectBItr != m_objectList.end(); ++objectBItr )
//      {
//          if( objectCollision( *objectAItr, *objectBItr ) )
//          {
//              return true;
//          }
//      }
//  }
//
//  return false;
//}

int EngineClass::createHeightMap( 
    const XMVECTOR& position,
    const int imageWidth,
    const int imageHeight,
    const vector< unsigned char >& imageRGBA,
    const int heightsStartX,
    const int heightsStartZ,
    const int numHeightsX,
    const int numHeightsZ )
    //IDirect3DDevice9& d3dDevice )
{
    int UID = m_nextHeightMapUID;
    ++m_nextHeightMapUID;

    assert( find_if( m_heightMapList.begin(), m_heightMapList.end(), EngineIdentify< HeightMap >( UID ) ) == m_heightMapList.end() );
    
    m_heightMapList.push_back( HeightMap() );
    m_heightMapList.back().create( UID, position,
        imageWidth, imageHeight, imageRGBA,
        heightsStartX, heightsStartZ, numHeightsX, numHeightsZ );
        //d3dDevice );

    return UID;
}

void EngineClass::destroyHeightMap( const int UID )
{
    EngineHeightMapList::iterator const heightMapItr = find_if( m_heightMapList.begin(), m_heightMapList.end(), EngineIdentify< HeightMap >( UID ) );
    if( heightMapItr != m_heightMapList.end() )
    {
        heightMapItr->destroy();
        m_heightMapList.erase( heightMapItr );
    }
}

const HeightMap* EngineClass::heightMap( const int UID ) const
{
    EngineHeightMapList::const_iterator const heightMapItr = find_if( m_heightMapList.begin(), m_heightMapList.end(), EngineIdentify< HeightMap >( UID ) );
    if( heightMapItr != m_heightMapList.end() )
    {
        return &( *heightMapItr );
    }
    return 0;
}

HeightMap* EngineClass::heightMap( const int UID )
{
    EngineHeightMapList::iterator const heightMapItr = find_if( m_heightMapList.begin(), m_heightMapList.end(), EngineIdentify< HeightMap >( UID ) );
    if( heightMapItr != m_heightMapList.end() )
    {
        return &( *heightMapItr );
    }
    return 0;
}

bool EngineClass::anyHeightMapsInCollision() const
{
    return false;
}

} // namespace Physics


