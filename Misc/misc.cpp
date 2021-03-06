// Andrew Davies

#include "pch.h"
#include "Misc/misc.h"
#include <algorithm>
//#include <math.h>

//using namespace std;
using namespace DirectX;

#define MISC_TWO_PI ( 2.0f * ( float )XM_PI )

namespace Misc
{

const XMVECTOR g_origin{ 0.0f, 0.0f, 0.0f, 1.0f };
const XMVECTOR& origin()
{
    return g_origin;
}

const XMVECTOR  g_zero{ 0.0f, 0.0f, 0.0f, 0.0f };
const XMVECTOR& zero()
{
    return g_zero;
}

const XMVECTOR  g_xAxis{ 1.0f, 0.0f, 0.0f, 0.0f };
const XMVECTOR& xAxis()
{
    return g_xAxis;
}

const XMVECTOR  g_yAxis{ 0.0f, 1.0f, 0.0f, 0.0f };
const XMVECTOR& yAxis()
{
    return g_yAxis;
}

const XMVECTOR  g_zAxis{ 0.0f, 0.0f, 1.0f, 0.0f };
const XMVECTOR& zAxis()
{
    return g_zAxis;
}

//const D3DXVECTOR3  g_origin3( 0.0f, 0.0f, 0.0f );
//const D3DXVECTOR3& origin3()
//{
//    return g_origin3;
//}
//
//const D3DXVECTOR3  g_zero3( 0.0f, 0.0f, 0.0f );
//const D3DXVECTOR3& zero3()
//{
//    return g_zero3;
//}
//
//const D3DXVECTOR3  g_xAxis3( 1.0f, 0.0f, 0.0f );
//const D3DXVECTOR3& xAxis3()
//{
//    return g_xAxis3;
//}
//
//const D3DXVECTOR3  g_yAxis3( 0.0f, 1.0f, 0.0f );
//const D3DXVECTOR3& yAxis3()
//{
//    return g_yAxis3;
//}
//
//const D3DXVECTOR3  g_zAxis3( 0.0f, 0.0f, 1.0f );
//const D3DXVECTOR3& zAxis3()
//{
//    return g_zAxis3;
//}

XMMATRIX lookAt(
    const XMVECTOR& from,
    const XMVECTOR& to,
    const XMVECTOR& up ) // normal )
{
    XMMATRIX result;
    
    result.r[ 2 ] = to - from;
    result.r[ 2 ] = XMVector4Normalize( result.r[ 2 ] );

    result.r[ 0 ] = XMVector4Normalize( XMVector4Cross( up, result.r[ 2 ], origin() ) );

    result.r[ 1 ] = XMVector4Normalize( XMVector4Cross( result.r[ 2 ], result.r[ 0 ], origin() ) );

    result.r[ 3 ] = from;

    return result;
}

XMMATRIX inverseMatrix( const XMMATRIX& matrix )
{
    const XMMATRIX inverseTranslationMatrix(
        1.0f, 0.0f, 0.0f, -XMVectorGetW( matrix.r[ 0 ] ),
        0.0f, 1.0f, 0.0f, -XMVectorGetW( matrix.r[ 1 ] ),
        0.0f, 0.0f, 1.0f, -XMVectorGetW( matrix.r[ 2 ] ),
        0.0f, 0.0f, 0.0f, 1.0f );
    //const XMMATRIX inverseTranslationMatrix(
    //    1.0f, 0.0f, 0.0f, -matrix.r[ 0 ][ 3 ],
    //    0.0f, 1.0f, 0.0f, -matrix.r[ 1 ][ 3 ],
    //    0.0f, 0.0f, 1.0f, -matrix.r[ 2 ][ 3 ],
    //    0.0f, 0.0f, 0.0f, 1.0f );
    
    const XMMATRIX inverseRotationMatrix(
        XMVectorGetX( matrix.r[ 0 ] ), XMVectorGetX( matrix.r[ 1 ] ), XMVectorGetX( matrix.r[ 2 ] ), 0.0f, 
        XMVectorGetY( matrix.r[ 0 ] ), XMVectorGetY( matrix.r[ 1 ] ), XMVectorGetY( matrix.r[ 2 ] ), 0.0f, 
        XMVectorGetZ( matrix.r[ 0 ] ), XMVectorGetZ( matrix.r[ 1 ] ), XMVectorGetZ( matrix.r[ 2 ] ), 0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f );
    //const XMMATRIX inverseRotationMatrix(
    //    matrix.r[ 0 ][ 0 ], matrix.r[ 1 ][ 0 ], matrix.r[ 2 ][ 0 ], 0.0f, 
    //    matrix.r[ 0 ][ 1 ], matrix.r[ 1 ][ 1 ], matrix.r[ 2 ][ 1 ], 0.0f, 
    //    matrix.r[ 0 ][ 2 ], matrix.r[ 1 ][ 2 ], matrix.r[ 2 ][ 2 ], 0.0f, 
    //    0.0f, 0.0f, 0.0f, 1.0f );

    return inverseRotationMatrix * inverseTranslationMatrix;
}

float normaliseAngle( const float angle )
{
    const float unitAngle = angle * ( 1.0f / MISC_TWO_PI );
    const float unitAngleQuot = floorf( unitAngle );
    return angle - ( unitAngleQuot * MISC_TWO_PI );
}

float smallestAngleFromAToB( float angleA, float angleB )
{
    angleA = normaliseAngle( angleA );
    angleB = normaliseAngle( angleB );

    const float angleBetween = angleB - angleA;

    if( angleBetween > ( float )XM_PI )
    {
        return angleBetween - MISC_TWO_PI;
    }
    if( angleBetween < -( float )XM_PI )
    {
        return MISC_TWO_PI + angleBetween;
    }

    return angleBetween;
}

float quaternionToEulerRoll( const XMVECTOR& quaternion )
{
    return atan2f(
        2.0f * ( ( XMVectorGetByIndex( quaternion, 2 ) * XMVectorGetByIndex( quaternion, 3 ) ) + ( XMVectorGetByIndex( quaternion, 0 ) * XMVectorGetByIndex( quaternion, 1 ) ) ),
        ( XMVectorGetByIndex( quaternion, 0 ) * XMVectorGetByIndex( quaternion, 0 ) ) -
        ( XMVectorGetByIndex( quaternion, 1 ) * XMVectorGetByIndex( quaternion, 1 ) ) -
        ( XMVectorGetByIndex( quaternion, 2 ) * XMVectorGetByIndex( quaternion, 2 ) ) +
        ( XMVectorGetByIndex( quaternion, 3 ) * XMVectorGetByIndex( quaternion, 3 ) ) );
}

float quaternionToEulerPitch( const XMVECTOR& quaternion )
{
    return asinf( -2.0f * ( ( XMVectorGetByIndex( quaternion, 1 ) * XMVectorGetByIndex( quaternion,  3 ) ) + ( XMVectorGetByIndex( quaternion, 0 ) * XMVectorGetByIndex( quaternion, 2 ) ) ) );
}

float quaternionToEulerYaw( const XMVECTOR& quaternion )
{
    return atan2f(
        2.0f * ( ( XMVectorGetByIndex( quaternion, 1 ) * XMVectorGetByIndex( quaternion, 2 ) ) + ( XMVectorGetByIndex( quaternion, 0 ) * XMVectorGetByIndex( quaternion, 3 ) ) ),
        ( XMVectorGetByIndex( quaternion, 0 ) * XMVectorGetByIndex( quaternion, 0 ) ) +
        ( XMVectorGetByIndex( quaternion, 1 ) * XMVectorGetByIndex( quaternion, 1 ) ) -
        ( XMVectorGetByIndex( quaternion, 2 ) * XMVectorGetByIndex( quaternion, 2 ) ) -
        ( XMVectorGetByIndex( quaternion, 3 ) * XMVectorGetByIndex( quaternion, 3 ) ) );
}




#if 0
unsigned int lineIntersectHorizontalCircle(
    const D3DXVECTOR2& linePositionMinusCirclePosition,
    const D3DXVECTOR2& lineDirection,
    const float radiusSquared,
    float& returnIntersectionParameter0,
    float& returnIntersectionParameter1 )
{
    const float a = lineDirection.dot( lineDirection );
    const float b = 2.0f * linePositionMinusCirclePosition.dot( lineDirection );
    const float c = linePositionMinusCirclePosition.dot( linePositionMinusCirclePosition ) - radiusSquared;

    const float bSquaredMinusFourAC = ( b * b ) - ( 4.0f * a * c );

    if( ( bSquaredMinusFourAC < 0.0f ) || ( a == 0.0f ) )
    {
        return 0;
    }
    else
    {
        float reciprocalTwoA =
            1.0f / ( 2.0f * a );
        
        if( bSquaredMinusFourAC == 0.0f )
        {
            float root = -( b * reciprocalTwoA );

            if( ( root > 1.0f ) || ( root < 0.0f ) )
            {
                return 0;
            }
            else
            {
                returnIntersectionParameter0 = root;
                return 1;
            }

        }
        else
        {
            float squareRootBSquaredMinusFourAC =
                maSqrt( bSquaredMinusFourAC );

            float rootA = ( squareRootBSquaredMinusFourAC - b ) * reciprocalTwoA;
            float rootB = ( -squareRootBSquaredMinusFourAC - b ) * reciprocalTwoA;

            if( rootB < rootA )
            {
                float temp = rootA;
                rootA = rootB;
                rootB = temp;
            }

            if( !( ( rootA > 1.0f ) || ( rootA < 0.0f ) ) )
            {
                if( !( ( rootB > 1.0f ) || ( rootB < 0.0f ) ) )
                {
                    returnIntersectionParameter0 = rootA;
                    returnIntersectionParameter1 = rootB;
                    return 2;
                }
                else
                {
                    returnIntersectionParameter0 = rootA;
                    return 1;
                }
            }
            else
            {
                if( !( ( rootB > 1.0f ) || ( rootB < 0.0f ) ) )
                {
                    returnIntersectionParameter0 = rootB;
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
}

bool const
    parallel(
        const D3DXVECTOR2& lineDirection,
        const D3DXVECTOR2& otherLineDirection )
{
    const float
        denominator =
            ( lineDirection[ 0 ] * otherLineDirection[ 1 ] ) -
            ( lineDirection[ 1 ] * otherLineDirection[ 0 ] );

    if( maAbs( denominator ) > ZERO )
    {
        return false;
    }

    return true;
}

void
    intersectNonParallelLines(
        const D3DXVECTOR2& linePositionMinusOtherLinePosition,
        const D3DXVECTOR2& lineDirection,
        const D3DXVECTOR2& otherLineDirection,
        float & returnIntersectionParameter,
        float & returnOtherIntersectionParameter )
{
    RI_ASSERT(
        !parallel(
            lineDirection,
            otherLineDirection ) );	
        
    returnIntersectionParameter = maMaxReal;
    returnOtherIntersectionParameter = maMaxReal;

    const float
        denominator =
            ( lineDirection[ 0 ] * otherLineDirection[ 1 ] ) -
            ( lineDirection[ 1 ] * otherLineDirection[ 0 ] );

    if( 0.0f == denominator )
    {
        return;
    }

    const float
        reciprocalDenominator =
            1.0f / denominator;

    returnIntersectionParameter =
        (	( linePositionMinusOtherLinePosition[ 1 ] * otherLineDirection[ 0 ] ) -
            ( linePositionMinusOtherLinePosition[ 0 ] * otherLineDirection[ 1 ] ) ) * reciprocalDenominator;
    

    returnOtherIntersectionParameter = 
        (	( linePositionMinusOtherLinePosition[ 1 ] * lineDirection[ 0 ] ) -
            ( linePositionMinusOtherLinePosition[ 0 ] * lineDirection[ 1 ] ) ) * reciprocalDenominator;

    //if( ( returnIntersectionParameter > 1.0f ) || ( returnIntersectionParameter < 0.0f ) )
    //{
    //	return false;
    //}

    //if( ( returnOtherIntersectionParameter > 1.0f ) || ( returnOtherIntersectionParameter < 0.0f ) )
    //{
    //	return false;
    //}

    //return true;
}


//bool const
//	lineIntersectHorizontalLine(	
//		const D3DXVECTOR2& linePositionMinusOtherLinePosition,
//		const D3DXVECTOR2& lineDirection,
//		const D3DXVECTOR2& otherLineDirection,
//		float & returnIntersectionParameter,
//		float & returnOtherIntersectionParameter )
//{
//	const float
//		denominator =
//			( lineDirection[ 0 ] * otherLineDirection[ 1 ] ) -
//			( lineDirection[ 1 ] * otherLineDirection[ 0 ] );
//
//	if( 0.0f == denominator )
//	{
//		return false;
//	}
//
//	const float
//		reciprocalDenominator =
//			1.0f / denominator;
//
//	returnIntersectionParameter =
//		(	( linePositionMinusOtherLinePosition[ 1 ] * otherLineDirection[ 0 ] ) -
//			( linePositionMinusOtherLinePosition[ 0 ] * otherLineDirection[ 1 ] ) ) * reciprocalDenominator;
//	
//	if( ( returnIntersectionParameter > 1.0f ) || ( returnIntersectionParameter < 0.0f ) )
//	{
//		return false;
//	}
//
//	returnOtherIntersectionParameter = 
//		(	( linePositionMinusOtherLinePosition[ 1 ] * lineDirection[ 0 ] ) -
//			( linePositionMinusOtherLinePosition[ 0 ] * lineDirection[ 1 ] ) ) * reciprocalDenominator;
//
//	if( ( returnOtherIntersectionParameter > 1.0f ) || ( returnOtherIntersectionParameter < 0.0f ) )
//	{
//		return false;
//	}
//
//
//	return true;
//}


////#define mathsLibraryHandedness ( 1 )

XMVECTOR
    tangentDirectionToNormalDirection(
        const XMVECTOR& tangentDirection )
{
//#if defined( mathsLibraryHandedness )
    return
        XMVECTOR(
            tangentDirection[ 2 ],
            0.0f,
            -tangentDirection[ 0 ],
            0.0f );
//#else
//	return
//		XMVECTOR(
//			-tangentDirection[ 2 ],
//			0.0f,
//			tangentDirection[ 0 ],
//			0.0f );
//#endif
}

XMVECTOR
    tangentDirectionUnitAndReciprocalNormalLengthToNormalDirectionUnit(
        const XMVECTOR& tangentDirectionUnit,
        float const reciprocalNormalLength )
{
//#if defined( mathsLibraryHandedness )
    return
        XMVECTOR(
            -tangentDirectionUnit[ 2 ] * reciprocalNormalLength,
            0.0f,
            tangentDirectionUnit[ 0 ] * reciprocalNormalLength,
            0.0f );
//#else
//	return
//		XMVECTOR(
//			tangentDirectionUnit[ 2 ] * reciprocalNormalLength,
//			0.0f,
//			-tangentDirectionUnit[ 0 ] * reciprocalNormalLength,
//			0.0f );
//#endif
}

void
    tangentDirectionToNormalDirectionAndBinormalDirection(
        const XMVECTOR& tangentDirection,
        XMVECTOR& returnNormalDirection,
        XMVECTOR& returnBinormalDirection )
{
    returnNormalDirection =
        tangentDirectionToNormalDirection( 
            tangentDirection );

//#if defined( mathsLibraryHandedness )
    returnBinormalDirection =
        XMVECTOR(
            ( tangentDirection[ 1 ] * returnNormalDirection[ 2 ] ) /* - ( returnNormalDirection[ 1 ] * tangentDirection[ 2 ] )*/,
            ( tangentDirection[ 2 ] * returnNormalDirection[ 0 ] ) - ( returnNormalDirection[ 2 ] * tangentDirection[ 0 ] ),
            /*( tangentDirection[ 0 ] * returnNormalDirection[ 1 ] )*/ - ( returnNormalDirection[ 0 ] * tangentDirection[ 1 ] ),
            0.0f );
//#else
//	returnBinormalDirection =
//		XMVECTOR(
//			/*( returnNormalDirection[ 1 ] * tangentDirection[ 2 ] ) */ - ( tangentDirection[ 1 ] * returnNormalDirection[ 2 ] ),
//			( returnNormalDirection[ 2 ] * tangentDirection[ 0 ] ) - ( tangentDirection[ 2 ] * returnNormalDirection[ 0 ] ),
//			( returnNormalDirection[ 0 ] * tangentDirection[ 1 ] ) /* - ( tangentDirection[ 0 ] * returnNormalDirection[ 1 ] )*/,
//			0.0f );
//#endif

}

void
    tangentDirectionUnitAndReciprocalNormalLengthToNormalDirectionUnitAndBinormalDirectionUnit(
        const XMVECTOR& tangentDirectionUnit,
        float const reciprocalNormalLength,
        XMVECTOR& returnNormalDirectionUnit,
        XMVECTOR& returnBinormalDirectionUnit )
{
    returnNormalDirectionUnit =
        tangentDirectionUnitAndReciprocalNormalLengthToNormalDirectionUnit( 
            tangentDirectionUnit,
            reciprocalNormalLength );

//#if defined( mathsLibraryHandedness )
    returnBinormalDirectionUnit =
        XMVECTOR(
            ( tangentDirectionUnit[ 1 ] * returnNormalDirectionUnit[ 2 ] ) /* - ( returnNormalDirectionUnit[ 1 ] * tangentDirectionUnit[ 2 ] )*/,
            ( tangentDirectionUnit[ 2 ] * returnNormalDirectionUnit[ 0 ] ) - ( returnNormalDirectionUnit[ 2 ] * tangentDirectionUnit[ 0 ] ),
            /*( tangentDirectionUnit[ 0 ] * returnNormalDirectionUnit[ 1 ] )*/ - ( returnNormalDirectionUnit[ 0 ] * tangentDirectionUnit[ 1 ] ),
            0.0f );
//#else
//	returnBinormalDirectionUnit =
//		XMVECTOR(
//			/*( returnNormalDirectionUnit[ 1 ] * tangentDirectionUnit[ 2 ] ) */ - ( tangentDirectionUnit[ 1 ] * returnNormalDirectionUnit[ 2 ] ),
//			( returnNormalDirectionUnit[ 2 ] * tangentDirectionUnit[ 0 ] ) - ( tangentDirectionUnit[ 2 ] * returnNormalDirectionUnit[ 0 ] ),
//			( returnNormalDirectionUnit[ 0 ] * tangentDirectionUnit[ 1 ] ) /* - ( tangentDirectionUnit[ 0 ] * returnNormalDirectionUnit[ 1 ] )*/,
//			0.0f );
//#endif

}


void
    tangentDirectionToCoordinateFrame(
        const XMVECTOR& tangentDirection,
        XMVECTOR& returnTangentDirectionUnit,
        XMVECTOR& returnNormalDirectionUnit,
        XMVECTOR& returnBinormalDirectionUnit )
{
    XMVECTOR normalDirection( 1.0f, 0.0f, 0.0f, 0.0f );
    XMVECTOR binormalDirection( 0.0f, 1.0f, 0.0f, 0.0f );
    tangentDirectionToNormalDirectionAndBinormalDirection(
        tangentDirection,
        normalDirection,
        binormalDirection );

    returnTangentDirectionUnit.getNormal( tangentDirection );
    returnNormalDirectionUnit.getNormal( normalDirection );
    returnBinormalDirectionUnit.getNormal( binormalDirection );
}





XMVECTOR
    directionUnit(
        const XMVECTOR& positionA,
        const XMVECTOR& positionB,
        XMVECTOR& returnDirection,
        float & returnDistanceSquared,
        float & returnDistance,
        float & returnReciprocalDistance )
{
    returnDirection = positionB - positionA;

    returnDistanceSquared =
        returnDirection.dotWithoutW(
            returnDirection );
    
    returnDistance =
        ( returnDistanceSquared > ZERO ) ?
        maSqrt( returnDistanceSquared ) :
        0.0f;

    returnReciprocalDistance =
        ( returnDistance > ZERO ) ?
        ( 1.0f / returnDistance ) :
        0.0f;
        
    return returnDirection * returnReciprocalDistance;
}






float const
    reciprocalXZNormalLength(
        const XMVECTOR& direction )
{
    const D3DXVECTOR2
        XZNormalDirection(
            -direction[ 2 ],
            direction[ 0 ] );

    float const
        XZNormalLengthSquared(
            XZNormalDirection.dot( XZNormalDirection ) );

    float const
        XZNormalLength(
            ( XZNormalLengthSquared > ZERO ) ?
            maSqrt( XZNormalLengthSquared ) : 0.0f );

    return ( XZNormalLength > ZERO ) ? ( 1.0f / XZNormalLength ) : 0.0f;
}

float const
    radiansToSteerValueUnits(
        float const value )
{
    return value * ( 4.0f / ( 2.0f * PI ) );
}

float const
    modulatedBrake(
        float const speedDifference,
        float const modulation )
{
    const float zeroMinimum( 2.0f );

    if( speedDifference < zeroMinimum )
    {
        return 0.0f;
    }
    else
    {
        const float zeroRange( 4.0f );
        const float breakRange( 3.0f * modulation );
        const float range( zeroRange + breakRange );
        const float reciprocalRange( 1.0f / range );
        const float	breakRangeFraction( breakRange * reciprocalRange );
        const float temp( ( speedDifference - zeroMinimum ) * reciprocalRange );
        const float	fraction( temp - floorf( temp ) );

        if( fraction > breakRangeFraction )
        {
            return 0.0f;
        }
        else
        {
            return -1.0f; // AIVehicleMaximumSpeed;
        }

    }
}

#endif
























}
