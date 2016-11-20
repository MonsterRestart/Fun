// Andrew Davies

#include "StdAFX.h"
#include "Game/AI/GameVehicle/HighDetail/gameVehicleHighDetail.h"
#include "Modules/Hamster/Interfaces/ITime.h"
#include "Modules/Vehicles/IVehicleSpecificationManager.h"
#include "Modules/Vehicles/IHandlingInternal.h"
#include "Game/AI/GameVehicle/gameVehicle.h"
#include "Game/Life/LifeVisuals.h"
#include "Game/Visuals/Camera/CGameCameraManager.h"
#include "Game/VehicleGameLinks/NVehicleDamage.h"
#include "Modules/Vehicles/Simulation/Vehicle/CVehicleCreationData.h"

#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION ) || \
	defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_DRIVING_TARGET )
#include "Game/AI/DebugGraphic/AIDebugGraphic.h"
#endif

#define GAME_VEHICLE_HIGH_DETAIL_DEFAULT_PREVIOUS_THROTTLE ( 0.5f )

const AI::PIDParametersStruct g_GameVehicleHighDetailPhysicsAssistLongitudinalPIDParameters =
{
   0.007f, // 0.05f,              /* 'P' proportional gain          */
   0.005f, // 0.05f,              /* 'I' integral gain              */
   0.025f,              /* 'D' derivative gain            */
   0.00f,              /* 'V' velocity feed forward      */
   0.00f,              /* 'B' bias                       */
   60.0f,              /* 'S' set point                  */
   -100.0f,              /* 'N' minimum output value       */
   300.0f,              /* 'M' maximum output value       */
   0.0f,              /* 'W' slew limit                 */
};

const AI::PIDParametersStruct g_GameVehicleHighDetailPhysicsAssistLateralPIDParameters =
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

GameVehicleHighDetailClass::GameVehicleHighDetailClass( )
	: m_iVehiclePtr( 0 )
	, m_manipulateDeltaTime( 0.0f )
	, m_physicsAssistDeltaTime( 0.0f )
	, m_fStuckTime( 0.0f )
	, m_manipulatedPreviousStep( false )
	, m_previousVelocity( 0.0f, 0.0f, 0.0f, 0.0f )
	, m_previousThrottle( GAME_VEHICLE_HIGH_DETAIL_DEFAULT_PREVIOUS_THROTTLE )
	, m_distanceAlongPath( 0.0f )
	, m_physicsAssistLongitudinalPID( g_GameVehicleHighDetailPhysicsAssistLongitudinalPIDParameters )
	, m_physicsAssistLateralPID( g_GameVehicleHighDetailPhysicsAssistLateralPIDParameters )
	, m_stunnedThrust( 0.0f )
	, m_stunnedSteer( 0.0f )
{

}

GameVehicleHighDetailClass::~GameVehicleHighDetailClass( )
{
	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION )
	AIDebugGraphicClearPermanent( m_debugLineThrust[0] );
	AIDebugGraphicClearPermanent( m_debugLineThrust[1] );
	AIDebugGraphicClearPermanent( m_debugLineThrust[2] );
	#endif

	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_DRIVING_TARGET )
	AIDebugGraphicClearPermanent( m_debugLineDrivingTarget);
	#endif
}

void GameVehicleHighDetailClass::create( 
	CAutoPtr< IVehicle > const iVehiclePtr,
	float const distanceAlongPath,
	float const damage,
	CVehicleSoundSpecManager::ELod const soundLOD,
	eVehicleControl const priority,
	bool const infiniteMass,		
	bool const hasExploded,		
	bool const invincibleTyres,
	bool const wreckable,	
	bool const graphicalDamage,
	bool const VO3Damage,
	uint16 const burstTyresMask,
	uint32 const panelsLooseMask,
	uint32 const panelsDetachedMask,
	float const bulletSoftness,
	float const impactSoftness,
	float const explosionSoftness,
	float const impactFragility,	
	bool const sirens,
	bool const headLights,
	bool const leftIndicatorLights,
	bool const rightIndicatorLights,
	bool const applyManualOverride, 
	GameVehicleManualOverrideClass const & manualOverride,
	bool const shouldStep,
	float const restHeight )
{
	RI_ASSERT( iVehiclePtr != 0 );

	MAv4 const velocity = iVehiclePtr->GetVelocity( );
	
	m_iVehiclePtr = boost::get_pointer( iVehiclePtr );
	
	m_distanceAlongPath = distanceAlongPath;
	
	m_manipulatedPreviousStep = false;	
	m_manipulateDeltaTime = 0.0f;
	m_physicsAssistDeltaTime = 0.0f;
	m_fStuckTime = 0.0f;
	m_previousVelocity = velocity;
	m_previousThrottle = GAME_VEHICLE_HIGH_DETAIL_DEFAULT_PREVIOUS_THROTTLE;
	
	m_physicsAssistLongitudinalPID.clear( );
	m_physicsAssistLateralPID.clear( ); 

	if( damage <= ZERO )
	{
		m_iVehiclePtr->ResetDamage();
	}
	else
	{	
		RI_ASSERT( damage <= 1.0f );
		float fClampedDamage = maClamp( damage, 0.0f, 1.0f );
		
		// restore the vehicle's state
		nVehicleMessages::sInitVehicleHealthAndDamage msg( 1.0f - fClampedDamage, hasExploded, burstTyresMask, panelsLooseMask, panelsDetachedMask );
		m_iVehiclePtr->SendVehicleMessage( msg );
	}

	m_iVehiclePtr->SetSoundLOD( soundLOD );
	
	m_iVehiclePtr->SetPhysicsPriority( priority );
	m_iVehiclePtr->SetRenderingPriority( priority );

	m_iVehiclePtr->SetPhysicsState( infiniteMass ? IVehicle::eInfiniteMass : IVehicle::eDynamic );
	
	m_iVehiclePtr->SetInvincibleTyres( invincibleTyres );

	m_iVehiclePtr->SetWreckable( wreckable );
		
	m_iVehiclePtr->AllowGraphicalDamage( graphicalDamage );
	
	m_iVehiclePtr->UseVO3Damage( VO3Damage );
	
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eBulletDamageMultiplier, bulletSoftness );
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eDamageMultiplier, impactSoftness );
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eExplosionDamageMultiplier, explosionSoftness );
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eFragility, impactFragility );

	setSirens( sirens );		 
	setHeadLights( headLights );
	setLeftIndicatorLights( leftIndicatorLights );
	setRightIndicatorLights( rightIndicatorLights );

	setManualOverride( applyManualOverride, manualOverride );
	
	setShouldStep( shouldStep );	
}

void GameVehicleHighDetailClass::create(
	tVehicleModelUID const modelUID,
	unsigned int const tint,
	float const damage,
	CVehicleSoundSpecManager::ELod const soundLOD,
	eVehicleControl const priority,
	bool const infiniteMass,
	bool const hasExploded,
	bool const invincibleTyres,		
	bool const wreckable,	
	bool const graphicalDamage,
	bool const VO3Damage,
	uint16 const burstTyresMask,
	uint32 const panelsLooseMask,
	uint32 const panelsDetachedMask,
	float const bulletSoftness,
	float const impactSoftness,
	float const explosionSoftness,
	float const impactFragility,
	float const distanceAlongPath,
	MAm4 const & transformation,
	MAv4 const & velocity,	
	bool const sirens,
	bool const headLights,
	bool const leftIndicatorLights,
	bool const rightIndicatorLights,
	bool const applyManualOverride, 
	GameVehicleManualOverrideClass const & manualOverride,
	bool const shouldStep,
	float const restHeight )
{
	CAutoPtr< IVehicleSpecification const > const vehicleSpecificationPtr = GetSingletonObject( ESingletonType_VehicleSpecificationManager )->GetVehicleSpecification( modelUID );				
	RI_ASSERT( vehicleSpecificationPtr != 0 );
	
	m_manipulateDeltaTime = 0.0f;
	m_physicsAssistDeltaTime = 0.0f;
	m_fStuckTime = 0.0f;
	m_manipulatedPreviousStep = false;
	m_previousVelocity = velocity;
	m_previousThrottle = GAME_VEHICLE_HIGH_DETAIL_DEFAULT_PREVIOUS_THROTTLE;
	
	m_distanceAlongPath = distanceAlongPath;
	
	m_physicsAssistLongitudinalPID.clear( );
	m_physicsAssistLateralPID.clear( );

	
	MAm4 vehicleTransformation = transformation;
	vehicleTransformation[ 3 ] += ( vehicleTransformation[ 1 ] * restHeight );

	CAutoPtr< ICreateVehicle > const vehicleManagerPtr = GetSingletonObject( ESingletonType_VehicleManager );
	CAutoPtr< IVehicle > const iVehiclePtr = vehicleManagerPtr->CreateVehicle( 
		modelUID,
		vehicleTransformation[ 3 ],
		maATan2( vehicleTransformation[ 2 ][ 0 ], vehicleTransformation[ 2 ][ 2 ] ),
		false, // bool snapterraincheap = true,
		false, // bool snapterainexpensive = false,
		false ); // bool bnetworkpublish = true		

	RI_ASSERT_STR( iVehiclePtr != 0, "A GAME VEHICLE HAS NOT BEEN ABLE TO CREATE A CVEHICLE WHEN IT WAs EXPECTING TO BE ABLE TO." );
	// PHYSICAL_VEHICLE_INSTANCES 64

	if( iVehiclePtr != 0 )
	{
		// Low detail
		
		// High detail
		m_iVehiclePtr = boost::get_pointer( iVehiclePtr );

		iVehiclePtr->SetTint( tint );
		
		IOverridePhysics::cMaskedSnapshot snap;
		snap.SetPosition( vehicleTransformation[ 3 ] );
		snap.SetOrientation( MAq::construct( vehicleTransformation ) );
		snap.SetVelocity( velocity );
		snap.SetAngularVelocity( MAv4::construct( 0.0f, 0.0f, 0.0f, 0.0f ) );
		m_iVehiclePtr->ApplyPhysicsState( &snap );	
		
		if( damage <= ZERO )
		{
			m_iVehiclePtr->ResetDamage();
		}
		else
		{	
			RI_ASSERT( damage <= 1.0f );
			float fClampedDamage = maClamp( damage, 0.0f, 1.0f );

			// restore the vehicle's state
			nVehicleMessages::sInitVehicleHealthAndDamage msg( 1.0f - fClampedDamage, hasExploded, burstTyresMask, panelsLooseMask, panelsDetachedMask );
			m_iVehiclePtr->SendVehicleMessage( msg );

		}

		m_iVehiclePtr->SetSoundLOD( soundLOD );
		
		m_iVehiclePtr->SetPhysicsPriority( priority );
		m_iVehiclePtr->SetRenderingPriority( priority );
		
		m_iVehiclePtr->SetPhysicsState( infiniteMass ? IVehicle::eInfiniteMass : IVehicle::eDynamic );
	
		m_iVehiclePtr->SetInvincibleTyres( invincibleTyres );
		
		m_iVehiclePtr->SetWreckable( wreckable );
		
		m_iVehiclePtr->AllowGraphicalDamage( graphicalDamage );
		
		m_iVehiclePtr->UseVO3Damage( VO3Damage );		

		m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eBulletDamageMultiplier, bulletSoftness );
		m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eDamageMultiplier, impactSoftness );
		m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eExplosionDamageMultiplier, explosionSoftness );
		m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eFragility, impactFragility );

		m_manipulateDeltaTime = 0.0f;
		m_physicsAssistDeltaTime = 0.0f;
		m_fStuckTime = 0.0f;
		m_previousVelocity = velocity;
		m_previousThrottle = GAME_VEHICLE_HIGH_DETAIL_DEFAULT_PREVIOUS_THROTTLE;

		// State

		setSirens( sirens );		 
		setHeadLights( headLights );
		setLeftIndicatorLights( leftIndicatorLights );
		setRightIndicatorLights( rightIndicatorLights );		

		setManualOverride( applyManualOverride, manualOverride );
	
		setShouldStep( shouldStep );
	}
}

void GameVehicleHighDetailClass::destroy( bool const deleteIVehicle )
{
	if( ( m_iVehiclePtr != 0 ) && deleteIVehicle )
	{
		hamster::CAutoPtr< ICreateVehicle > const vehicleManagerPtr = GetSingletonObject( ESingletonType_VehicleManager );
		vehicleManagerPtr->Delete( m_iVehiclePtr );

		m_iVehiclePtr = 0;
	}

	
	m_physicsAssistLongitudinalPID.clear( );
	m_physicsAssistLateralPID.clear( );

	m_distanceAlongPath = 0.0f;
	m_manipulatedPreviousStep = false;	
	m_manipulateDeltaTime = 0.0f;
	m_physicsAssistDeltaTime = 0.0f;
	m_fStuckTime = 0.0f;
	m_previousVelocity = MAv4::construct( 0.0f, 0.0f, 0.0f, 0.0f );
	m_previousThrottle = GAME_VEHICLE_HIGH_DETAIL_DEFAULT_PREVIOUS_THROTTLE;	

	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION )
	AIDebugGraphicClearPermanent( m_debugLineThrust[0] );
	AIDebugGraphicClearPermanent( m_debugLineThrust[1] );
	AIDebugGraphicClearPermanent( m_debugLineThrust[2] );
	#endif

	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_DRIVING_TARGET )
	AIDebugGraphicClearPermanent( m_debugLineDrivingTarget);
	#endif
}

void GameVehicleHighDetailClass::teleport(
	MAm4 const & transformation,
	MAv4 const & velocity, 
	IGameVehiclePath const & path,
	MAm4 const & offsetFromPath )
{	
	RI_ASSERT( m_iVehiclePtr != 0 );

	MAm4 inverseOffsetFromPath;
	inverseOffsetFromPath.invertRotateTranslate( offsetFromPath );
	
	MAm4 transformationMinusOffsetFromPath = transformation * inverseOffsetFromPath;

	m_physicsAssistLongitudinalPID.clear( );
	m_physicsAssistLateralPID.clear( );

	IOverridePhysics::cMaskedSnapshot snap;

	snap.SetVelocity( velocity );
	snap.SetOrientation( MAq( transformation ) );
	snap.SetPosition( transformation[ 3 ] );
	snap.SetShortHop( false );			//discontinuous (uninterpolated) teleport
	
	m_iVehiclePtr->ApplyPhysicsState( &snap );

	m_distanceAlongPath = path.closestDistanceAlong( transformationMinusOffsetFromPath[ 3 ] );
}

void GameVehicleHighDetailClass::setVelocity(
	float const targetSpeed,
	IGameVehiclePath const & path,
	MAm4 const & offsetFromPath )
{	
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_physicsAssistLongitudinalPID.clear( );
	m_physicsAssistLateralPID.clear( );

	MAm4 const targetTransformation = path.transformation( m_distanceAlongPath ) * offsetFromPath;

	// It may be better to limit it depending on the direction of the vehicle.
	MAv4 const velocity = targetTransformation[ 2 ] * targetSpeed;

	IOverridePhysics::cMaskedSnapshot snap;
	snap.SetVelocity( velocity );
	snap.SetShortHop( false );			//discontinuous (uninterpolated) teleport
	
	m_iVehiclePtr->ApplyPhysicsState( &snap );
}

bool GameVehicleHighDetailClass::willManipulateNextStep( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	CAutoPtr< IVehicleHandling > const handlingPtr = m_iVehiclePtr->GetHandling( );
	RI_ASSERT( handlingPtr != 0 );

	return handlingPtr->IsTimeToUpdate( );
}

void GameVehicleHighDetailClass::step(
	GameVehicleClass const & gameVehicle,
	float const deltaTime,	 
	IGameVehiclePath const & path,
	MAm4 const & offsetFromPath,
	float const targetSpeed,
	bool const basicManipulation,
	bool const applyPhysicsAssists,
	bool const horn,	
	bool const applyManualOverride,
	GameVehicleManualOverrideClass const & manualOverride,
	bool const applyPathAlign,
	bool const useLowSpeedForwardProjection,
	float const restHeight,
	float const vehicleLength,
	bool const donateGroundPlane,
	bool const stunned )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	CAutoPtr< IVehicleHandling > const handlingPtr = m_iVehiclePtr->GetHandling( );
	RI_ASSERT( handlingPtr != 0 );

	m_manipulateDeltaTime += deltaTime;
	m_physicsAssistDeltaTime += deltaTime;
					
	MAm4 const cpWhere = m_iVehiclePtr->GetMatrix( );
	MAm4 transformation = cpWhere;
	transformation[ 3 ] -= ( transformation[ 1 ] * restHeight );

	if( m_manipulatedPreviousStep )
	{
		if( applyPhysicsAssists && !applyManualOverride && /*!applyPathAlign &&*/ !stunned && !dead( ) )
		{
			m_distanceAlongPath = path.closestDistanceAlong( transformation[ 3 ] ); // IF I'M GOING TO DO THIS OPTIMISE ME.

			physicsAssist( 
				gameVehicle,
				m_physicsAssistDeltaTime, 
				m_iVehiclePtr,
				path,
				offsetFromPath,	
				m_distanceAlongPath,
				targetSpeed );
		}

		m_physicsAssistDeltaTime = 0.0f;
		m_manipulatedPreviousStep = false;
	}

	////needs fresh controls (only) if the handling will update in the next frame (best practice for manipulation packets)
	////if( ( rTimeSinceLastUpdate + GAME_STEPTIME ) > HANDLING_UPDATE_TIME )
	////...or only immediately after a handling update (best practice for impulses)
	////ok, somewhat better but still drifting a bit
	if( willManipulateNextStep( ) )
	{	
		m_iVehiclePtr->UseAIOverrideHandling( IAIHandlingOverride::eTypeDefault );		

		m_distanceAlongPath = path.closestDistanceAlong( transformation[ 3 ] ); // IF I'M GOING TO DO THIS OPTIMISE ME.
	
		SVehicleManipulationPacket vehicleManipulationPacket;			

		if( basicManipulation )
		{	
			basicManipulate(
				m_manipulateDeltaTime, 
				path, 
				offsetFromPath,
				targetSpeed, 
				applyManualOverride,
				manualOverride,
				vehicleLength,
				restHeight,
				vehicleManipulationPacket );
		}
		else
		{	
			manipulate(
				m_manipulateDeltaTime, 
				path, 
				offsetFromPath,
				targetSpeed, 
				applyManualOverride,
				manualOverride,
				applyPathAlign,
				useLowSpeedForwardProjection,
				restHeight,
				vehicleLength,
				vehicleManipulationPacket );
		}
		
		if( donateGroundPlane )
		{
			MAm4 const pathTransformation = path.transformation( m_distanceAlongPath );	
			
			MAv4 wheelSurfacePositions[ g_nMaxWheels ];
			MAv4 wheelNormals[ g_nMaxWheels ];
			int32 wheelSurfaces[ g_nMaxWheels ];
			for( unsigned int wheelIndex = 0; wheelIndex != g_nMaxWheels; ++wheelIndex )
			{
				wheelSurfacePositions[ wheelIndex ] = MAv4::construct( 0.0f, 0.0f, 0.0f, 1.0f );

				RI_ASSERT( m_iVehiclePtr->GetSpecification( ) != 0 );
				if( m_iVehiclePtr->GetSpecification( )->IsWheelPresent( (eWheelIndex)wheelIndex ) )
				{
					MAv4 const wheelRaycastPosCarSpace = m_iVehiclePtr->GetWheelContactPosition( ( eWheelIndex )wheelIndex );
					MAv4 const wheelRaycastPosWorldSpace = cpWhere * wheelRaycastPosCarSpace;

					MAv4 const carY = -cpWhere[ 1 ];
					MAv4 const tpoint = wheelRaycastPosWorldSpace;

					float const d = pathTransformation[ 1 ].dot( carY );

					float t = 100.0f;
					if( maAbs( d ) > ZERO )
					{
						t = ( pathTransformation[ 1 ].dot( pathTransformation[ 3 ] - tpoint ) ) / d;
					}

					wheelSurfacePositions[ wheelIndex ] = tpoint + ( t * carY );
				}

				wheelNormals[ wheelIndex ] = pathTransformation[ 1 ];
				wheelSurfaces[ wheelIndex ] = 0;
			}

			m_iVehiclePtr->GetHandling( )->DonateWheelSuspensionCompressions( wheelSurfacePositions, wheelNormals, wheelSurfaces );
		}

		if( stunned )
		{
			vehicleManipulationPacket.fThrust = m_stunnedThrust;		
			vehicleManipulationPacket.fSteerValue = m_stunnedSteer;		
			vehicleManipulationPacket.bHandbrake = true;
		}
		else
		{
			m_stunnedThrust = ( ( vehicleManipulationPacket.fThrust > 0.0f ) ? 0.2f : -0.2f );
			m_stunnedSteer = ( ( vehicleManipulationPacket.fSteerValue > 0.0f ) ? 1.0f : -1.0f );
		}

		vehicleManipulationPacket.fHornVolume = ( horn ? 1.0f : 0.0f );

		if( dead( ) )
		{
			vehicleManipulationPacket.fThrust = 0.0f;		
			vehicleManipulationPacket.fSteerValue = 0.0f;
			vehicleManipulationPacket.bHandbrake = true;
			vehicleManipulationPacket.fHornVolume = 0.0f;
		}

		m_iVehiclePtr->SendManipulationPacket( vehicleManipulationPacket );
			
		#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION )
		m_PrevManipulationPacket = vehicleManipulationPacket;
		#endif		

		m_manipulateDeltaTime = 0.0f;
		m_manipulatedPreviousStep = true;
	}

	/*** debugging code - Vehicle Manipulation packet ***/
	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION )
	{			
		MAm4 transformation = m_iVehiclePtr->GetMatrix( );
		transformation[ 3 ] -= ( transformation[ 1 ] * restHeight );

		CAutoPtr<const IVehicleHandling> piHandling = ( m_iVehiclePtr->GetHandling() );
		#ifdef NEW_HANDLING
			MAreal Steer = piHandling->GetSteeringAngleInRadians();
		#else
			MAreal Steer = piHandling->GetWheelAngle();
		#endif
		
		MAm4 SteerRot;
		SteerRot.setYRotation( -Steer );
		MAv4 SteerVec = SteerRot * transformation[ 2 ];
				
		SteerVec *= 5.0f;
		float fThrustSign = ( m_PrevManipulationPacket.fThrust >= 0 )? 	1.0f:-1.0f;
							
		MAv4 SPos  = transformation[ 3 ] + ( MAv4::YAxis() * 2.0f);
		MAv4 EPos0 = SPos + ( SteerVec * m_PrevManipulationPacket.fThrust );
		MAv4 EPos1 = SPos + ( SteerVec * fThrustSign                      );
		MAv4 EPos2 = SPos + ( SteerVec 				                      ) + ( transformation[1] * 5.0f );
		AI::Graphic::LineEnum	LineCol = (m_PrevManipulationPacket.fThrust >= 0.0f)?	AI::Graphic::LineGreenLine : AI::Graphic::LineRedLine;
		m_debugLineThrust[0] = AI::Graphic::LineClass( LineCol,                   SPos,  EPos0, 0.1f);
		m_debugLineThrust[1] = AI::Graphic::LineClass( AI::Graphic::LineBlueLine, EPos0, EPos1, 0.1f);
		AIDebugGraphicSetPermanent( m_debugLineThrust[0] );
		AIDebugGraphicSetPermanent( m_debugLineThrust[1] );
		
		//draw handbrake
		if( m_PrevManipulationPacket.bHandbrake == true )
		{
			m_debugLineThrust[2] = AI::Graphic::LineClass( AI::Graphic::LineRedLine, SPos, EPos2, 0.1f);
			AIDebugGraphicSetPermanent( m_debugLineThrust[2] );
		}
		else
		{
			AIDebugGraphicClearPermanent( m_debugLineThrust[2] );
		}
	}
	#endif

	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_DRIVING_TARGET )
	{
		MAv4 SPos = m_DrivingTargetPos;
		MAv4 EPos = SPos + ( MAv4::YAxis() * 5.0f);
		m_debugLineDrivingTarget = AI::Graphic::LineClass( AI::Graphic::LineWhiteLine, SPos, EPos, 0.1f);
		AIDebugGraphicSetPermanent( m_debugLineDrivingTarget );
	}
	#endif
}

void GameVehicleHighDetailClass::stun( bool bStunSteer )
{
	//RI_ASSERT( m_iVehiclePtr != 0 );
	
	// These are the right values for ( bStunSteer == false )
	m_stunnedThrust = 0.0f;
	m_stunnedSteer = 0.0f;
	
	if( ( m_iVehiclePtr != 0 ) && bStunSteer )
	{
		MAm4 const vehicleTransformation = m_iVehiclePtr->GetMatrix( );
		MAv4 const vehicleVelocity = m_iVehiclePtr->GetVelocity( );
	
		m_stunnedThrust = ( ( vehicleTransformation[ 2 ].dot( vehicleVelocity ) > 0.0f ) ? 0.2f : -0.2f );
		m_stunnedSteer  = ( ( vehicleTransformation[ 0 ].dot( vehicleVelocity ) > 0.0f ) ? 1.0f : -1.0f );
	}
}	

void GameVehicleHighDetailClass::unstun( )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
}	

void GameVehicleHighDetailClass::registerBulletHit(
	MAv4 const & contactpos, 
	MAv4 const & contactnorm,
	float const hitdistance,
	float const fBulletDamage, 
	bool const bDoVisualDamage )   
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->RegisterBulletHit(
		contactpos, 
		contactnorm,
		hitdistance,
		fBulletDamage, 
		bDoVisualDamage );
}

bool GameVehicleHighDetailClass::dead( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->IsDead( );
}

float GameVehicleHighDetailClass::damage( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->QueryVehicleDamage();
}
 
void GameVehicleHighDetailClass::setDamage( float const damage )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	if( damage <= ZERO )
	{
		m_iVehiclePtr->ResetDamage();
	}
	else
	{	
		RI_ASSERT( damage <= 1.0f );
		NVehicleDamage::SetVehicleHealth( m_iVehiclePtr, 1.0f - maClamp( damage, 0.0f, 1.0f ) );
	}
}

CVehicleSoundSpecManager::ELod GameVehicleHighDetailClass::soundLOD( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetSoundLOD( );
}
 
void GameVehicleHighDetailClass::setSoundLOD( CVehicleSoundSpecManager::ELod const soundLOD )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->SetSoundLOD( soundLOD );
}

eVehicleControl GameVehicleHighDetailClass::priority( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetPhysicsPriority();
}
 
void GameVehicleHighDetailClass::setPriority( eVehicleControl const priority )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetPhysicsPriority( priority );
	m_iVehiclePtr->SetRenderingPriority( priority );
}

bool GameVehicleHighDetailClass::hasExploded( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	return m_iVehiclePtr->IsExploded();
}
 
bool GameVehicleHighDetailClass::infiniteMass( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return ( m_iVehiclePtr->GetPhysicsState( ) == IVehicle::eInfiniteMass );
}
 
void GameVehicleHighDetailClass::setInfiniteMass( bool const infiniteMass )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetPhysicsState( infiniteMass ? IVehicle::eInfiniteMass : IVehicle::eDynamic );
}

bool GameVehicleHighDetailClass::invincibleTyres( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetInvincibleTyres( );
}
 
void GameVehicleHighDetailClass::setInvincibleTyres( bool const invincibleTyres )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetInvincibleTyres( invincibleTyres );
}

bool GameVehicleHighDetailClass::wreckable( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetWreckable( );
}
 
void GameVehicleHighDetailClass::setWreckable( bool const wreckable )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetWreckable( wreckable );
}

bool GameVehicleHighDetailClass::graphicalDamage( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetAllowGraphicalDamage( );
}
 
void GameVehicleHighDetailClass::setGraphicalDamage( bool const graphicalDamage )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->AllowGraphicalDamage( graphicalDamage );
}

bool GameVehicleHighDetailClass::VO3Damage( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetUseVO3Damage( );
}
 
void GameVehicleHighDetailClass::setVO3Damage( bool const VO3Damage )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->UseVO3Damage( VO3Damage );
}

uint16 GameVehicleHighDetailClass::burstTyres( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	sCVehicleCreationDamage damageParams;
	damageParams.Fill( ( const CVehicle* ) m_iVehiclePtr );
	
	return damageParams.shotwheel;
}

uint32 GameVehicleHighDetailClass::panelsLoose( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	sCVehicleCreationDamage damageParams;
	damageParams.Fill( ( const CVehicle* ) m_iVehiclePtr );
	
	return damageParams.loose;
}

uint32 GameVehicleHighDetailClass::panelsDetached( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	sCVehicleCreationDamage damageParams;
	damageParams.Fill( ( const CVehicle* ) m_iVehiclePtr );
	
	return damageParams.detach;
}

float GameVehicleHighDetailClass::bulletSoftness( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetIValue( IVehicleInstanceSettings::eBulletDamageMultiplier );
}
 
void GameVehicleHighDetailClass::setBulletSoftness( float const bulletSoftness )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eBulletDamageMultiplier, bulletSoftness );
}

float GameVehicleHighDetailClass::impactSoftness( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetIValue( IVehicleInstanceSettings::eDamageMultiplier );
}
 
void GameVehicleHighDetailClass::setImpactSoftness( float const impactSoftness )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eDamageMultiplier, impactSoftness );
}

float GameVehicleHighDetailClass::explosionSoftness( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetIValue( IVehicleInstanceSettings::eExplosionDamageMultiplier );
}
 
void GameVehicleHighDetailClass::setExplosionSoftness( float const explosionSoftness )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eExplosionDamageMultiplier, explosionSoftness );
}

float GameVehicleHighDetailClass::impactFragility( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetIValue( IVehicleInstanceSettings::eFragility );
}
 
void GameVehicleHighDetailClass::setImpactFragility( float const impactFragility )
{
	RI_ASSERT( m_iVehiclePtr != 0 );
	
	m_iVehiclePtr->SetIValue( IVehicleInstanceSettings::eFragility, impactFragility );
}
 
void GameVehicleHighDetailClass::setSirens( bool const sirens )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->ActivateLamp( PST_LSIRENLIGHT1, sirens );
	m_iVehiclePtr->ActivateLamp( PST_LSIRENLIGHT2, sirens );
	m_iVehiclePtr->ActivateLamp( PST_RSIRENLIGHT1, sirens );
	m_iVehiclePtr->ActivateLamp( PST_RSIRENLIGHT2, sirens );
}
 
void GameVehicleHighDetailClass::setHeadLights( bool const headLights )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->ActivateLamp( PST_LHEADLIGHT, headLights );
	m_iVehiclePtr->ActivateLamp( PST_RHEADLIGHT, headLights );
}
 
void GameVehicleHighDetailClass::setLeftIndicatorLights( bool const leftIndicatorLights )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->ActivateLamp( PST_FLINDICATOR, leftIndicatorLights );
	m_iVehiclePtr->ActivateLamp( PST_LSIDELIGHT, leftIndicatorLights );
	m_iVehiclePtr->ActivateLamp( PST_BLINDICATOR, leftIndicatorLights );
}
 
void GameVehicleHighDetailClass::setRightIndicatorLights( bool const rightIndicatorLights )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->ActivateLamp( PST_FRINDICATOR, rightIndicatorLights );
	m_iVehiclePtr->ActivateLamp( PST_RSIDELIGHT, rightIndicatorLights );
	m_iVehiclePtr->ActivateLamp( PST_BRINDICATOR, rightIndicatorLights );
}

void GameVehicleHighDetailClass::setManualOverride( 
	bool const applyManualOverride,
	GameVehicleManualOverrideClass const & manualOverride )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	if( applyManualOverride )
	{
		SVehicleManipulationPacket vehicleManipulationPacket;	
		vehicleManipulationPacket.fThrust = manualOverride.thrust( );
		vehicleManipulationPacket.fSteerValue = manualOverride.steer( );
		vehicleManipulationPacket.fBurnout = 0.0f;
		vehicleManipulationPacket.fHornVolume = 0.0f; 
		vehicleManipulationPacket.fLeanFB = 0.0f;
		vehicleManipulationPacket.bHandbrake = manualOverride.handbrake( );
		vehicleManipulationPacket.bNitro = false;
		vehicleManipulationPacket.bAction1 = false;
		vehicleManipulationPacket.bBrakeIsPressed = false;
		vehicleManipulationPacket.bAutoBrakeIsPressed = false;
		m_iVehiclePtr->SendManipulationPacket( vehicleManipulationPacket );
	}
}

void GameVehicleHighDetailClass::setShouldStep( bool const shouldStep )
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	m_iVehiclePtr->UseAIOverrideHandling( shouldStep ? IAIHandlingOverride::eTypeDefault : IAIHandlingOverride::eNone );
}

// Movement + Control
void GameVehicleHighDetailClass::setPath( float const distanceAlongPath )
{
	m_distanceAlongPath = distanceAlongPath;
}

// State
MAm4 GameVehicleHighDetailClass::transformation( float const restHeight ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	MAm4 returnValue = m_iVehiclePtr->GetMatrix( );
	returnValue[ 3 ] -= ( returnValue[ 1 ] * restHeight );

	return returnValue;
}

MAv4 GameVehicleHighDetailClass::velocity( ) const
{
	RI_ASSERT( m_iVehiclePtr != 0 );

	return m_iVehiclePtr->GetVelocity( );
}

