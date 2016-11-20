// Andrew Davies

#if !defined( MISC_H )
#define MISC_H

#include <algorithm>
#include <DirectXMath.h>

namespace Misc
{

#define FLT_ZERO ( 1.0e-06f )

const DirectX::XMVECTOR& origin();
const DirectX::XMVECTOR& zero();
const DirectX::XMVECTOR& xAxis();
const DirectX::XMVECTOR& yAxis();
const DirectX::XMVECTOR& zAxis();

//const D3DXVECTOR3& origin3();
//const D3DXVECTOR3& zero3();
//const D3DXVECTOR3& xAxis3();
//const D3DXVECTOR3& yAxis3();
//const D3DXVECTOR3& zAxis3();

template< class T >
inline T clamp( const T value, const T minimum, const T maximum )
{
    assert( minimum <= maximum );
    return (std::max)( minimum, (std::min)( value, maximum ) );
}

DirectX::XMMATRIX lookAt( const DirectX::XMVECTOR& from, const DirectX::XMVECTOR& to, const DirectX::XMVECTOR& up );

DirectX::XMMATRIX inverseMatrix( const DirectX::XMMATRIX& matrix );

float normaliseAngle( float angle );

float smallestAngleFromAToB( float angleA, float angleB );

float quaternionToEulerRoll( const DirectX::XMVECTOR& quaternion );
float quaternionToEulerPitch( const DirectX::XMVECTOR& quaternion );
float quaternionToEulerYaw( const DirectX::XMVECTOR& quaternion );

}

#endif