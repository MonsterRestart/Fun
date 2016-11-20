// Andrew Davies.

#include "Modules/Maths/mamath.h"
#include "Game/AI/AI.h"
#include "Modules/Maths/AdditionalMaths.h"
#include <limits.h>



namespace AI
{


MAv4 directionUnit(
	MAv4 const & direction,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance )
{
	returnDistanceSquared =
		direction.dotWithoutW(
			direction );
	
	returnDistance =
		( returnDistanceSquared > ZERO ) ?
		maSqrt( returnDistanceSquared ) :
		0.0f;

	returnReciprocalDistance =
		( returnDistance > ZERO ) ?
		( 1.0f / returnDistance ) :
		0.0f;
		
	return direction * returnReciprocalDistance;
}

MAv2 directionUnit(
	MAv2 const & direction,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance )
{
	returnDistanceSquared =
		direction.dot(
			direction );
	
	returnDistance =
		( returnDistanceSquared > ZERO ) ?
		maSqrt( returnDistanceSquared ) :
		0.0f;

	returnReciprocalDistance =
		( returnDistance > ZERO ) ?
		( 1.0f / returnDistance ) :
		0.0f;
		
	return direction * returnReciprocalDistance;
}

MAv4 directionUnit(
	MAv4 const & positionA,
	MAv4 const & positionB,
	MAv4 & returnDirection,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance )
{
	returnDirection = positionB - positionA;

	return 
		directionUnit(
			returnDirection,
			returnDistanceSquared,
			returnDistance,
			returnReciprocalDistance );
}

MAv2 directionUnit(
	MAv2 const & positionA,
	MAv2 const & positionB,
	MAv2 & returnDirection,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance )
{
	returnDirection = positionB - positionA;

	return 
		directionUnit(
			returnDirection,
			returnDistanceSquared,
			returnDistance,
			returnReciprocalDistance );
}

////-----------------------------------------------------------------------
////-----------------------------------------------------------------------
//#include "stdafx.h"
//#include "Game/Life/LifeSystem.h"
//#include "Game/AI/Traffic/Traffic.h"
//void	CheckLaneTrackLists()
//{
//	CAutoPtr< const AtlasClass > const pAtlas = GetSingletonObject( ESingletonType_Atlas );
//	if( pAtlas )
//	{
//		Traffic::ManagerClass & rTrafficManager = GetSingletonObject( ESingletonType_LifeSystem )->trafficManager();
//
//		for( uint i = 0; i < pAtlas->totalNumberOfLaneTracks(); ++i )
//		{
//			//get the list header
//			Traffic::LaneTrackList const & rLL = rTrafficManager.laneTrackDataArray()[ i ];
//			
//			//verify this list
//			rLL.Verify();
//		}
//	}
//	
//}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
bool	PlayerHitVictim( MAv4* pPreColInfo )
{
	bool bRet = false;
	if( pPreColInfo != NULL )
	{
		MAv4 	VictimPos  = pPreColInfo[ 0 ];
		MAv4	VictimVel  = pPreColInfo[ 1 ];
		MAv4 	PlayerPos  = pPreColInfo[ 2 ];
		MAv4 	PlayerVel  = pPreColInfo[ 3 ];
		
		MAv4 vSep = VictimPos - PlayerPos;
		MAv4 vSepN;
		vSepN.getNormal( vSep );
		float fPlayerDP = PlayerVel.dot( vSepN );
		float fVictimDP = VictimVel.dot( vSepN ) * -1.0f;
		
		if( fPlayerDP >= fVictimDP )
		{
			bRet = true;
		}
	}
	return bRet;	
}

//-----------------------------------------------------------------------
//safe version of maACos
//NB maACos( 1.0f ) assets as it attempts to maSqrt( 0.0f )
//-----------------------------------------------------------------------
float	SafeACos( float fCosine )
{
	if( fCosine >= 1.0f )
	{
		return 0.0f;
	}
	else if( fCosine <= -1.0f )
	{
		return PI;
	}
	
	return maACos( fCosine );
}

//-----------------------------------------------------------------------
// returns a random number between 0.0f and 1.0f
//-----------------------------------------------------------------------
float GetUnitRandom()					
{
	unsigned int const   iRandom = ( unsigned int )OSGetTime( );
	static float fInvMax = ( 1.0f / ( float )UCHAR_MAX );
	float const  fRandom = ( ( float )( iRandom % UCHAR_MAX ) ) * fInvMax; // 0 <-> 1
	return fRandom;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float	XZSeparation( MAv4 const & vPosA, MAv4 const & vPosB )
{
	return maSqrt( AI::XZSeparationSq(vPosA, vPosB) );
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float	XZSeparationSq( MAv4 const & vPosA, MAv4 const & vPosB )
{
	MAv2  vSepXZ = MAv2( vPosA[0] - vPosB[0], vPosA[2] - vPosB[2] );
	float fSepXZSq = vSepXZ.dot( vSepXZ);
	return fSepXZSq;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
bool	SpheresIntersect(MAv4 const& vPos0, float fRad0, MAv4 const& vPos1, float fRad1 )
{
	float fSepRadSq = maSqr( fRad0 + fRad1 );
	float fSepSq    = ( vPos0 - vPos1 ).lengthSquared();
	if( fSepSq <= fSepRadSq ) 
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
const PIDParametersStruct g_PIDParameters =
{
   0.05f,              /* 'P' proportional gain          */
   0.05f,              /* 'I' integral gain              */
   0.025f,              /* 'D' derivative gain            */
   0.00f,              /* 'V' velocity feed forward      */
   0.00f,              /* 'B' bias                       */
   60.0f,              /* 'S' set point                  */
   -100.0f,              /* 'N' minimum output value       */
   300.0f,              /* 'M' maximum output value       */
   0.0f,              /* 'W' slew limit                 */
};

PIDClass::PIDClass( )
	: m_integral( 0.0f )
	, m_last_error( 0.0f )
	, m_last_output( 0.0f )
	, m_parameters( g_PIDParameters )
{
}

PIDClass::PIDClass( PIDParametersStruct const & parameters )
	: m_integral( 0.0f )
	, m_last_error( 0.0f )
	, m_last_output( 0.0f )
	, m_parameters( parameters )
{
}	

void PIDClass::set( PIDParametersStruct const & parameters )
{
	m_integral = 0.0f;
	m_last_error = 0.0f;
	m_last_output = 0.0f;
	m_parameters = parameters;
}	

void PIDClass::clear( )
{
	m_integral = 0.0f;
	m_last_error = 0.0f;
	m_last_output = 0.0f;
}	

float PIDClass::computePID( float const current, float const target )
{
	/* the error for the current pass is the difference   */
	/* between the current target and the current PV      */
	float const this_error = target - current;

	/* the derivative is the difference between the error */
	/* for the current pass and the previous pass         */
	float const deriv = this_error - m_last_error;

	/* the new error is added to the integral             */
	m_integral += target - current;

	/* now that all of the variable terms have been       */
	/* computed they can be multiplied by the appropriate */
	/* coefficients and the resulting products summed     */
	float this_output = 
				m_parameters.p_gain * this_error
			  + m_parameters.i_gain * m_integral
			  + m_parameters.d_gain * deriv
			  + m_parameters.vel_ff * target
			  + m_parameters.bias;

	m_last_error = this_error;

	/* check for slew rate limiting on the output change  */
	if( maAbs( m_parameters.slew ) > ZERO )
	{
		if( this_output - m_last_output > m_parameters.slew)
		{
			this_output = m_last_output + m_parameters.slew;
		}
		else if( m_last_output - this_output > m_parameters.slew)
		{
			this_output = m_last_output - m_parameters.slew;
		}
	}

	/* now check the output value for absolute limits     */
	this_output = maClamp( this_output, m_parameters.min, m_parameters.max );

	/* store the new output value to be used as the old   */
	/* output value on the next loop pass                 */
	m_last_output = this_output;
	return this_output;
}


}










//#include "stdafx.h"
//#include "Game/Life/LifeSystem.h"
//#include "Game/Life/Collectables/CollectableManager.h"
//#include "R:/Toolchain/Data/Attractors/TypesInclude.txt"
//#include "Attractor/AttractorManager.h"
//#include "Modules/Shared/DriverWii/Include/CrossPlatform/Attractor/Collectable.h"
//
//#define MANAGER_MAXIMUM_NUMBER_OF_ATTRACTORS ( 10 )
//void DSStuff( )
//{
//	// Access to all cop cars
//	{
//		// This would also include all the cop cars involved in cop road blocks but not 'fake' cops 'faked' in missions.
//
//		const Cop::ManagerClass& copManager = GetSingletonObject( ESingletonType_LifeSystem )->CopManager( );
//		const Cop::CarListType& copCarList = copManager.carList( );
//		const Cop::CarListType::const_iterator endCopCarItr = copCarList.end( );
//		for(	Cop::CarListType::const_iterator copCarItr = copCarList.begin( ); copCarItr != endCopCarItr; ++copCarItr )
//		{	
//			// Cop::CarClass is defined in S:\D5Wii-Code\Root\Game\AI\Cop\Car\copCar.h
//			// Cop::CarClass inherits GameVehicleClass that's defined in S:\D5Wii-Code\Root\Game\AI\GameVehicle\gameVehicle.h and has route information etc.
//			Cop::CarClass const & copCar = *copCarItr;
//
//			MAm4 const copCarTransformation = copCar.transformation( );
//
//			MAv4 const copCarPosition = copCarTransformation[ 3 ];
//			MAv4 const copCarDirection = copCarTransformation[ 2 ];
//	
//			// How to get the GameVehicle from a Cop::CarClass 
//			GameVehicleClass const & gameVehicle = copCar;
//		}
//	}
//
//	// Access to all enemies
//	{
//		// All vehicles in missions (including the players) are CLifeInstance_Vehicle. (so that's all vehicles except traffic, trams and cops)
//
//		LifeInstanceVehiclePtrListClass const & vehicleInstancePtrList = GetAllLifeInstanceVehicles( );
//		for(	LifeInstanceVehiclePtrListClass::const_iterator vehicleInstancePtrItr = vehicleInstancePtrList.begin( ); 
//				vehicleInstancePtrItr != vehicleInstancePtrList.end( );
//				++vehicleInstancePtrItr )
//		{
//			// CLifeInstance_Vehicle is defined in S:\D5Wii-Code\Root\Game\Life\Instances\CLifeInstance_Vehicle.h
//			CLifeInstance_Vehicle const & vehicleInstance = *( *vehicleInstancePtrItr );
//			
//			MAm4 const enemyTransformation = vehicleInstance.GetMatrix( );
//
//			MAv4 const enemyPosition = enemyTransformation[ 3 ];
//			MAv4 const enemyDirection = enemyTransformation[ 2 ];
//
//			// How to get the GameVehicle from a CLifeInstance_Vehicle
//			GameVehicleClass const & gameVehicle = vehicleInstance.gameVehicle( );
//		}
//	}
//
//	// Access to enemy AI route
//	{
//		// You can access a vehicle's generic route information through it's GameVehicleClass
//		// All vehicle's in the game use game vehicles at some level
//		// Above shows you how to access the GameVehicles of cops and mission enemies.
//		// Below shows you how to access a game vehicle's route information
//		// GameVehicleClass is defined in S:\D5Wii-Code\Root\Game\AI\GameVehicle\gameVehicle.h 
//
//		static GameVehicleClass const gameVehicle;
//
//		// IGameVehiclePath is defined in S:\D5Wii-Code\Root\Game\AI\GameVehicle\Path\gameVehiclePath.h
//		IGameVehiclePath const * const gameVehiclePathPtr = gameVehicle.pathPtr( );
//		RI_ASSERT( gameVehiclePathPtr != 0 ); // A game vehicle's path ptr should never be NULL.
//
//		// All measurements along a path are in meters.
//		float mapPointDistanceAlongPath = gameVehicle.distanceAlongPath( );
//
//		unsigned int const numberOfMapPoints = 10;
//		float const distanceBetweenMapPoints = 20.0f; // Meters
//		for( unsigned int mapPointIdx = 0; mapPointIdx != numberOfMapPoints; ++mapPointIdx, mapPointDistanceAlongPath += distanceBetweenMapPoints )
//		{
//			if( mapPointDistanceAlongPath < gameVehiclePathPtr->length( ) )
//			{
//				MAm4 const mapPointTransformation = gameVehiclePathPtr->transformation( mapPointDistanceAlongPath );
//				// Or
//				MAv4 const mapPointPosition = gameVehiclePathPtr->position( mapPointDistanceAlongPath );
//			}
//		}
//	}
//
//	// Create new roadblock
//	// +
//	// Add and remove waypoints
//	// +
//	// Drop bomb
//	// ------------------------------
//
//	// I think it's best to expose this information through the mission system so that the scripters can set up missions to use them however needed
//	// To this end I've started a system based in S:\D5Wii-Code\Root\Game\GameplayMechanic\PlayerWorld\playerWorld.cpp
//	// We're already using it to expose a position set during the pause menu to script, called the 'player world destination' position.
//	// The mission is told when there's a valid position set up and when it is turned off.  They can also turn it off themselves.
//	// When it's valid they can access it using a 'marker', which is basically a mission element that can be used to represent a position in a 
//	// loads of circumstances.  They can send AI to it, create props there, teleport the player there, attach a sound/effect/volume watch etc.
//
//	// If there is some way of us knowing or asking when a 'road block' or 'way point' is available, I can incorporate it into this system (along with the 'type' information)
//	// (or I could add a 'bool setRoadBlock( pos, type )' function and a 'void setWaypoint( pos, index )' to the 'PlayerWorld' singleton for you to call (returning false if invalid))
//
//	// Access to all roadblocks
//	// +	
//	// Access to all waypoints
//	// ------------------------------
//
//	// This would probably be a case of accessing the 'PlayerWorld' singleton 'GetSingletonObject( ESingletonType_PlayerWorld )' and looping over the valid 'roadblock' or 'waypoint' data.
//
//	// Roadblock hit
//	// ------------------------------
//
//	// If scripters use the 'PlayerWorld' system to place 'props' in the world to make road blocks, they can add collision watches or volume watches to them and be told when they're hit.  
//	// We should be able to pass this onto you in whatever way is best.
//
//	// Access to all points of interest
//	// ------------------------------
//
//	// As far as I know points of interest are 'collectables'.  You can access collectable information like this...
//	{
//		//S:\D5Wii-Code\Root\Game\Life\Collectables\CollectableManager.h
//		CAutoPtr< CCollectableManager const > const collectableManagerPtr = GetSingletonObject( ESingletonType_CollectableManager );
//
//
//		// To find the ten closest collectables to a position...
//		SAttractorCollectable const * closeCollectablePtrArray[ MANAGER_MAXIMUM_NUMBER_OF_ATTRACTORS ];	
//		memset( closeCollectablePtrArray, 0, sizeof( SAttractorCollectable ) * MANAGER_MAXIMUM_NUMBER_OF_ATTRACTORS );					
//
//		attractor::Search 					search;
//		search.m_where 						= MAv4::construct( 0.0f, 0.0f, 0.0f, 1.0f ); // <-----your pos here
//		search.m_radius 					= 10.0f;
//		search.m_type 						= EAttractorType_Collectable;
//		search.m_search_static_structures = true;
//		search.m_search_runtime_structures = false;
//
//		attractor::ArrayInserter< SAttractorCollectable > inserter( closeCollectablePtrArray, 0, MANAGER_MAXIMUM_NUMBER_OF_ATTRACTORS ); 
//
//		GetSingletonObject( ESingletonType_AttractorManager )->FindAttractors( search, inserter );
//	
//		for( unsigned int closeCollectableIndex = 0; closeCollectableIndex != MANAGER_MAXIMUM_NUMBER_OF_ATTRACTORS; ++closeCollectableIndex )
//		{
//			if( closeCollectablePtrArray[ closeCollectableIndex ] != 0 )
//			{				
//				MAv4 const collectablePosition = closeCollectablePtrArray[ closeCollectableIndex ]->GetPosition( );
//			}
//		}
//
//		// OR
//
//
//		// Iterate over all collectables			
//		CCollectableManager::TCollectableObjectContainer::const_iterator itCurrent = GetSingletonObject( ESingletonType_CollectableManager )->collectableObjects( ).begin();
//		CCollectableManager::TCollectableObjectContainer::const_iterator itEnd = GetSingletonObject( ESingletonType_CollectableManager )->collectableObjects( ).end();
//		for( ; itCurrent != itEnd ; ++itCurrent )
//		{
//			SCollectableObject& object = *itCurrent;
//			
//			// SCollectableObject is defined in S:\D5Wii-Code\Root\Game\Life\Collectables\CollectableManager.h and has positional and type info etc.
//		}
//	}
//
//	// Point of interest hit
//	// ------------------------------
//
//	// void CCollectableManager::CollectableAction( SCollectableObject& object, EPlayer const player ) is called when a collectable is collected/hit.
//	// We should be able to get this event to you somehow.
//
//
//	// End of mission	
//	// ------------------------------
//
//	// void CStatusControlNode::OnEnable( bool& rEnabled, baseLifeNode::eNodeFireWire& rWireToFire ) is called when missions start and end in S:\D5Wii-Code\Root\Game\Life\Nodes\StatusControl.cpp
//	// Alternatively you could investigate each 'onEnable' function in all the .cpp files in this directory S:\D5Wii-Code\Root\Game\Life\Nodes\ScriptLimits.	
//	// However, it's probably best if you check with Patrick as he controls the whole progression of the missions/game and is probably in a better position to tell you when it happens =).
//
//	
//	// Stun cars
//	// ------------------------------
//
//	// I think this would be a case of a new function being added to CLifeInstance_Vehicle, 'StunMe( )' for instance.  And then between us we be able to work out when and on which to call it.
//
//
//	// Access to enemies sight areas
//	// ------------------------------
//
//	// These values live in the mission data 'somewhere'.  They are basically the radii of some test volumes attached to the enemy AI.  
//	// They are not always the same and are set on a mission by mission basis.  I think we just need to put them somewhere a bit more obvious an accessible.  
//	// I think we'll have to get back to you with these =)
//
//
//}