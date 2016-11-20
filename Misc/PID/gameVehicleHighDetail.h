// Andrew Davies

#if !defined( GAME_VEHICLE_HIGH_DETAIL_H )
#define GAME_VEHICLE_HIGH_DETAIL_H

#include "Game/AI/GameVehicle/HighDetail/gameVehicleHighDetailFwd.h"
#include "Game/AI/GameVehicle/Path/gameVehiclePathFwd.h"
#include "Modules/Maths/maTypes.h"
#include "Game/AI/GameVehicle/gameVehicleFwd.h"
//#include "Modules/Hamster/Renderer/IViewport.h"
#include "Modules/Vehicles/SVehicleManipulationPacket.h"
#include "Game/Simulation/Audio/VehicleSoundSpecManager.h"
#include "Game/AI/AI.h"
#include "IVehicle.h"
	
#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION ) || \
	defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_DRIVING_TARGET )
#include "Game/AI/Graphic/AIGraphic.h"
#endif

class GameVehicleHighDetailClass
{
	friend class GameVehicleClass;

private:
	IVehicle * m_iVehiclePtr;
	float m_manipulateDeltaTime;
	float m_physicsAssistDeltaTime;
	float m_fStuckTime;
	bool m_manipulatedPreviousStep;
	MAv4 m_previousVelocity;
	float m_previousThrottle;

	float m_distanceAlongPath;
	
	// State	
	AI::PIDClass m_physicsAssistLongitudinalPID;
	AI::PIDClass m_physicsAssistLateralPID;

	float m_stunnedThrust;
	float m_stunnedSteer;

	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_VEHICLE_MANIPULATION )
	AI::Graphic::LineClass m_debugLineThrust[3];
	SVehicleManipulationPacket m_PrevManipulationPacket;		//last vehicle Manipulation packet sent
	#endif
	
	#if defined( GAME_VEHICLE_HIGH_DETAIL_SHOW_DRIVING_TARGET )
	AI::Graphic::LineClass m_debugLineDrivingTarget;
	MAv4 m_DrivingTargetPos;
	#endif

public:
	GameVehicleHighDetailClass( );
	virtual ~GameVehicleHighDetailClass( );

	void create(
		CAutoPtr< IVehicle > iVehiclePtr,
		float distanceAlongPath,
		float damage,
		CVehicleSoundSpecManager::ELod soundLOD,
		eVehicleControl priority,
		bool infiniteMass,
		bool hasExploded,
		bool invincibleTyres,
		bool wreckable,	
		bool graphicalDamage,
		bool VO3Damage,
		uint16 const burstTyresMask,
		uint32 const panelsLooseMask,
		uint32 const panelsDetachedMask,
		float bulletSoftness,
		float impactSoftness,
		float explosionSoftness,
		float impactFragility,		
		bool sirens,
		bool headLights,
		bool leftIndicatorLights,
		bool rightIndicatorLights,
		bool applyManualOverride, 
		GameVehicleManualOverrideClass const & manualOverride,
		bool shouldStep,
		float restHeight );

	void create(
		tVehicleModelUID modelUID,
		unsigned int tint,
		float damage,
		CVehicleSoundSpecManager::ELod soundLOD,
		eVehicleControl priority,		
		bool infiniteMass,
		bool hasExploded,
		bool invincibleTyres,
		bool wreckable,	
		bool graphicalDamage,
		bool VO3Damage,
		uint16 const burstTyresMask,
		uint32 const panelsLooseMask,
		uint32 const panelsDetachedMask,
		float bulletSoftness,
		float impactSoftness,
		float explosionSoftness,
		float impactFragility,		
		float distanceAlongPath,
		MAm4 const & transformation,
		MAv4 const & velocity,	
		bool sirens,
		bool headLights,
		bool leftIndicatorLights,
		bool rightIndicatorLights,
		bool applyManualOverride, 
		GameVehicleManualOverrideClass const & manualOverride,
		bool shouldStep,
		float restHeight );

	void destroy( bool const deleteIVehicle );
	
	void teleport( 
		MAm4 const & transformation, 
		MAv4 const & velocity,
		IGameVehiclePath const & path,
		MAm4 const & offsetFromPath );

	void setVelocity(	
		float const targetSpeed,
		IGameVehiclePath const & path,
		MAm4 const & offsetFromPath );	

	bool willManipulateNextStep( ) const;
	
	void step( 
		GameVehicleClass const & gameVehicle,
		float deltaTime,
		IGameVehiclePath const & path,
		MAm4 const & offsetFromPath,
		float targetSpeed,
		bool basicManipulation,
		bool applyPhysicsAssists,
		bool horn,
		bool applyManualOverride,
		GameVehicleManualOverrideClass const & manualOverride,		
		bool applyPathAlign,
		bool useLowSpeedForwardProjection,
		float restHeight, 
		float length,
		bool donateGroundPlane,
		bool stunned );

	void setPath( float distanceAlongPath );

	IVehicle * iVehiclePtr( ) const
	{
		return m_iVehiclePtr;
	}
	
	float manipulateDeltaTime( ) const
	{
		return m_manipulateDeltaTime;
	}

	float physicsAssistDeltaTime( ) const
	{
		return m_physicsAssistDeltaTime;
	}

	bool manipulatedPreviousStep( ) const
	{
		return m_manipulatedPreviousStep;
	}

	MAv4 const & previousVelocity( ) const
	{
		return m_previousVelocity;
	}

	float previousThrottle( ) const
	{
		return m_previousThrottle;
	}

	float distanceAlongPath( ) const
	{
		return m_distanceAlongPath;
	}

	float drawDistanceAlongPath( ) const
	{
		return m_distanceAlongPath;
	}
	
	AI::PIDClass const & physicsAssistLongitudinalPID( ) const
	{
		return m_physicsAssistLongitudinalPID;
	}

	AI::PIDClass const & physicsAssistLateralPID( ) const
	{
		return m_physicsAssistLateralPID;
	}

	float stunnedThrust( ) const
	{
		return m_stunnedThrust;
	}

	float stunnedSteer( ) const
	{
		return m_stunnedSteer;
	}
		
	void registerBulletHit(
		MAv4 const & contactpos, 
		MAv4 const & contactnorm,
		float hitdistance,
		float fBulletDamage, 
		bool bDoVisualDamage );
	
	bool dead( ) const;

	float damage( ) const;
	 
	void setDamage( float damage );

	CVehicleSoundSpecManager::ELod soundLOD( ) const;
	 
	void setSoundLOD( CVehicleSoundSpecManager::ELod soundLOD );

	eVehicleControl priority( ) const;
	 
	void setPriority( eVehicleControl priority );

	bool hasExploded( ) const;

	bool infiniteMass( ) const;
	 
	void setInfiniteMass( bool infiniteMass );

	bool invincibleTyres( ) const;
	 
	void setInvincibleTyres( bool invincibleTyres );

	bool wreckable( ) const;
	 
	void setWreckable( bool wreckable );

	bool graphicalDamage( ) const;
	 
	void setGraphicalDamage( bool graphicalDamage );

	bool VO3Damage( ) const;
	 
	void setVO3Damage( bool VO3Damage );

	uint16 burstTyres( ) const;
	
	void setBurstTyres( uint16 burstTyresMask );
	
	uint32 panelsLoose( ) const;
	
	void setPanelsLoose( uint32 panelsLooseMask );
	
	uint32 panelsDetached( ) const;
	
	void setPanelsDetached( uint32 panelsDetachedMask );
	
	float bulletSoftness( ) const;
	 
	void setBulletSoftness( float bulletSoftness );

	float impactSoftness( ) const;
	 
	void setImpactSoftness( float impactSoftness );

	float explosionSoftness( ) const;
	 
	void setExplosionSoftness( float explosionSoftness );

	float impactFragility( ) const;
	 
	void setImpactFragility( float impactFragility );

	void setSirens( bool sirens );
	 
	void setHeadLights( bool headLights );
	 
	void setLeftIndicatorLights( bool leftIndicatorLights );
	 
	void setRightIndicatorLights( bool rightIndicatorLights );

	void setManualOverride( bool applyManualOverride, GameVehicleManualOverrideClass const & manualOverride );
		 
	void setShouldStep( bool shouldStep );

	// State
	MAm4 transformation( float restHeight ) const;
	MAv4 velocity( ) const;


private:
	void basicManipulate( 
		float deltaTime, 
		IGameVehiclePath const & path,
		MAm4 const & offsetFromPath,
		float targetSpeed, 	
		bool applyManualOverride,
		GameVehicleManualOverrideClass const & manualOverride,
		float vehicleLength,
		float restHeight,
		SVehicleManipulationPacket & vehicleManipulationPacket );

	void manipulate( 
		float deltaTime, 
		IGameVehiclePath const & path,
		MAm4 const & offsetFromPath,
		float targetSpeed, 	
		bool applyManualOverride,
		GameVehicleManualOverrideClass const & manualOverride,
		bool alignToPath,		
		bool useLowSpeedForwardProjection,
		float restHeight,
		float vehicleLength,
		SVehicleManipulationPacket & vehicleManipulationPacket );
		
	void physicsAssist( 
		GameVehicleClass const & gameVehicle,
		float deltaTime,
		CAutoPtr< IVehicle > iVehiclePtr,
		IGameVehiclePath const & path,
		MAm4 const & offsetFromPath,
		float distanceAlongPath,
		float targetSpeed );

	void stun( bool bStunSteer );
	void unstun( );

};

#endif


