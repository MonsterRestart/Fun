// Andrew Davies.

#if !defined( AI_H )
#define AI_H

#include "Modules/Maths/maTypes.h"

namespace AI
{

MAv4 directionUnit(
	MAv4 const & direction,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance );

MAv2 directionUnit(
	MAv2 const & direction,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance );

MAv4 directionUnit(
	MAv4 const & positionA,
	MAv4 const & positionB,
	MAv4 & returnDirection,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance );

MAv2 directionUnit(
	MAv2 const & positionA,
	MAv2 const & positionB,
	MAv2 & returnDirection,
	float & returnDistanceSquared,
	float & returnDistance,
	float & returnReciprocalDistance );

bool	PlayerHitVictim( MAv4* pPreColInfo );
float	SafeACos( float fCosine );
float 	GetUnitRandom();
float	XZSeparation( MAv4 const & vPosA, MAv4 const & vPosB );
float	XZSeparationSq( MAv4 const & vPosA, MAv4 const & vPosB );
bool	SpheresIntersect(MAv4 const& vPos0, float fRad0, MAv4 const& vPos1, float fRad1 );

//void	CheckLaneTrackLists();
//#define VerifyLaneTrackLists()			{ OSReport("%s:%d Verify :%s \n", __FILE__, __LINE__, __FUNCTION__ ); AI::CheckLaneTrackLists();}


struct PIDParametersStruct
{
	float p_gain;      /* 'P' proportional gain          */
	float i_gain;      /* 'I' integral gain              */
	float d_gain;      /* 'D' derivative gain            */
	float vel_ff;      /* 'V' velocity feed forward      */
	float bias;        /* 'B' bias                       */
	float setpt;       /* 'S' set point                  */
	float min;         /* 'N' minimum output value       */
	float max;         /* 'M' maximum output value       */
	float slew;        /* 'W' maximum slew rate          */
};

class PIDClass
{
private:
	float m_integral;
	float m_last_error;
	float m_last_output;
	
	PIDParametersStruct m_parameters;

public:
	PIDClass( );

	PIDClass( PIDParametersStruct const & parameters );

	void set( PIDParametersStruct const & parameters );

	void clear( );

	float integral( ) const
	{
		return m_integral;
	}

	float last_error( ) const
	{
		return m_last_error;
	}

	float last_output( ) const
	{
		return m_last_output;
	}

	float computePID( float current, float target );
};

}

#endif




