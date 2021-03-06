// Andrew Davies

#include "pch.h"
#include "Physics/Dynamics/physicsDynamics.h"
#include "Misc/misc.h"

//using namespace std;
using namespace DirectX;

namespace Physics
{

Dynamics::Dynamics( )
    : m_position{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_orientation{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_velocity{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_angularVelocity{ 0.0f, 0.0f, 0.0f, 0.0f }
{
    m_transformation = XMMatrixIdentity();
}

Dynamics::Dynamics( 
    const XMVECTOR& position,
    const XMVECTOR& orientation,
    const XMVECTOR& velocity,
    const XMVECTOR& angularVelocity,
    const XMMATRIX& transformation )
    : m_position( position )
    , m_orientation( orientation )
    , m_velocity( velocity )
    , m_angularVelocity( angularVelocity )
    , m_transformation( transformation )
{
}

Dynamics::Dynamics( 
    const XMVECTOR& position,
    const XMVECTOR& orientation,
    const XMVECTOR& velocity,
    const XMVECTOR& angularVelocity,
    const XMVECTOR& centerOfMass )
    : m_position( position )
    , m_orientation( orientation )
    , m_velocity( velocity )
    , m_angularVelocity( angularVelocity )
{
    set( position, orientation, velocity, angularVelocity, centerOfMass );
}

void Dynamics::set(
    const XMVECTOR& position,
    const XMVECTOR& orientation,
    const XMVECTOR& velocity,
    const XMVECTOR& angularVelocity,
    const XMVECTOR& centerOfMass )
{
    m_position = position;
    m_orientation = orientation;
    m_velocity = velocity;
    m_angularVelocity = angularVelocity;

    m_transformation = XMMatrixAffineTransformation(
        XMVECTOR{ 1.0f, 1.0f, 1.0f, 0.0f }, // FXMVECTOR Scaling, 
        centerOfMass, // FXMVECTOR RotationOrigin, 
        m_orientation, // FXMVECTOR RotationQuaternion, 
        m_position ); // GXMVECTOR Translation);
    
    //D3DXVECTOR3 positionVec3 = m_position;
    ////XMVECTOR orientationQuat;
    ////const XMVECTOR orientation = XMQuaternionRotationRollPitchYaw( 0.0f, /* float Pitch, */ 3.14f, /* float Yaw, */ 0.0f ); /* float Roll); */
    ////D3DXQuaternionRotationYawPitchRoll( &orientationQuat, 0.0f, 0.0, m_orientation );
    //D3DXVECTOR3 const rotationCenter = centerOfMass;
    //D3DXMatrixAffineTransformation( 
    //    &m_transformation, // XMMATRIX *pOut, 
    //    1.0f, // FLOAT Scaling, 
    //    &rotationCenter,
    //    &m_orientation, // &orientationQuat, // CONST XMVECTOR *pRotation, 
    //    &positionVec3 ); // CONST D3DXVECTOR3 *pTranslation);
}

void Dynamics::step(
    const XMVECTOR& force,
    const XMVECTOR& torque,
    const float deltaTime,
    const bool infiniteMass,
    const float mass,
    const XMMATRIX& inverseInertiaTensorWorld,
    const XMVECTOR& centerOfMass )
{
    XMVECTOR acceleration = Misc::zero();
    XMVECTOR angularAcceleration = Misc::zero();
    if( !infiniteMass )
    {
        assert( mass > FLT_EPSILON );
        assert( mass < FLT_MAX );
        acceleration = force / mass;

        angularAcceleration = XMVector4Transform( torque, inverseInertiaTensorWorld );
        //D3DXVec4Transform( &angularAcceleration, &torque, &inverseInertiaTensorWorld );
    }
    
    m_velocity += ( acceleration * deltaTime );
    m_angularVelocity += ( angularAcceleration * deltaTime );

    m_position += ( m_velocity * deltaTime );
    //angular velocity
    {
        XMVECTOR axis;
        const float angle = XMVectorGetX( XMVector4Length( m_angularVelocity ) );
        //const float angle = D3DXVec4Length( &m_angularVelocity );
        //const float angle = D3DXVec3Length( &m_angularVelocity );
        if( angle > 0.0001f )
        {
            // sync(fAngle) = sin(c*fAngle)/t
            axis = m_angularVelocity * ( sinf( 0.5f * angle * deltaTime ) / angle );
        }
        else
        {
            // use Taylor's expansions of sync function
            axis = m_angularVelocity * ( ( 0.5f * deltaTime ) - ( ( deltaTime * deltaTime * deltaTime ) * 0.020833333333f * angle * angle ) );
        }

        const XMVECTOR dorn{ XMVectorGetX( axis ), XMVectorGetY( axis ), XMVectorGetZ( axis ), cosf( angle * deltaTime * 0.5f ) };

        m_orientation *= dorn;
        m_orientation = XMVector4Normalize( m_orientation );
    }
    //m_orientation += ( m_angularVelocity * deltaTime );

    m_transformation = XMMatrixAffineTransformation(
        XMVECTOR{ 1.0f, 1.0f, 1.0f, 0.0f }, // FXMVECTOR Scaling, 
        centerOfMass, // FXMVECTOR RotationOrigin, 
        m_orientation, // FXMVECTOR RotationQuaternion, 
        m_position ); // GXMVECTOR Translation);

    //D3DXVECTOR3 position = m_position;
    ////XMVECTOR orientation;
    ////const XMVECTOR orientation = XMQuaternionRotationRollPitchYaw( 0.0f, /* float Pitch, */ 3.14f, /* float Yaw, */ 0.0f ); /* float Roll); */
    ////D3DXQuaternionRotationYawPitchRoll( &orientation, 0.0f, 0.0, m_orientation );
    //D3DXVECTOR3 const rotationCenter = centerOfMass;
    //D3DXMatrixAffineTransformation( 
    //    &m_transformation, // XMMATRIX *pOut, 
    //    1.0f, // FLOAT Scaling, 
    //    &rotationCenter,
    //    &m_orientation, // &orientation, // CONST XMVECTOR *pRotation, 
    //    &position ); // CONST D3DXVECTOR3 *pTranslation);
}

Dynamics2D::Dynamics2D()
    : m_position{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_orientation( 0.0f )
    , m_velocity{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_angularVelocity( 0.0f )
{
    m_transformation = XMMatrixIdentity();
}

Dynamics2D::Dynamics2D( 
    XMVECTOR const & position,
    float const orientation,
    XMVECTOR const & velocity,
    float const angularVelocity,
    XMMATRIX const & transformation )
    : m_position( position )
    , m_orientation( orientation )
    , m_velocity( velocity )
    , m_angularVelocity( angularVelocity )
    , m_transformation( transformation )
{
}

Dynamics2D::Dynamics2D( 
    const XMVECTOR& position,
    const float orientation,
    const XMVECTOR& velocity,
    const float angularVelocity,
    const XMVECTOR& centerOfMass )
    : m_position( position )
    , m_orientation( orientation )
    , m_velocity( velocity )
    , m_angularVelocity( angularVelocity )
{
    set( position, orientation, velocity, angularVelocity, centerOfMass );
}

void Dynamics2D::set(
    XMVECTOR const & position,
    float const orientation,
    XMVECTOR const & velocity,
    float const angularVelocity,
    XMVECTOR const & centerOfMass )
{
    m_position = position;
    m_orientation = orientation;
    m_velocity = velocity;
    m_angularVelocity = angularVelocity;
    
    m_transformation = XMMatrixAffineTransformation(
        XMVECTOR{ 1.0f, 1.0f, 1.0f, 0.0f }, // FXMVECTOR Scaling, 
        centerOfMass, // FXMVECTOR RotationOrigin, 
        XMQuaternionRotationRollPitchYaw( 0.0f, /* float Pitch, */ 0.0f, /* float Yaw, */ m_orientation ), // FXMVECTOR RotationQuaternion, 
        m_position ); // GXMVECTOR Translation);
    
    //D3DXVECTOR3 positionVec3 = m_position;
    //const XMVECTOR orientationQuat = XMQuaternionRotationRollPitchYaw( 0.0f, /* float Pitch, */ 0.0f, /* float Yaw, */ m_orientation ); /* float Roll); */
    ////D3DXQuaternionRotationYawPitchRoll( &orientationQuat, 0.0f, 0.0, m_orientation );
    //D3DXVECTOR3 const rotationCenter = centerOfMass;
    //D3DXMatrixAffineTransformation( 
    //    &m_transformation, // XMMATRIX *pOut, 
    //    1.0f, // FLOAT Scaling, 
    //    &rotationCenter,
    //    &orientationQuat, // CONST XMVECTOR *pRotation, 
    //    &positionVec3 ); // CONST D3DXVECTOR3 *pTranslation);
}

void Dynamics2D::step(
    const XMVECTOR& force,
    const float torque,
    const float deltaTime,
    const bool infiniteMass,
    const float mass,
    const float momentOfInertia,
    const XMVECTOR& centerOfMass )
{
    XMVECTOR acceleration = Misc::zero();
    float angularAcceleration = 0.0f;
    if( !infiniteMass )
    {
        assert( mass > FLT_EPSILON );
        assert( mass < FLT_MAX );
        acceleration = force / mass;
        
        assert( momentOfInertia > FLT_EPSILON );
        assert( momentOfInertia < FLT_MAX );
        angularAcceleration = torque / momentOfInertia;
    }

    m_velocity += ( acceleration * deltaTime );
    m_angularVelocity += ( angularAcceleration * deltaTime );

    m_position += ( m_velocity * deltaTime );
    m_orientation += ( m_angularVelocity * deltaTime );
    
    m_transformation = XMMatrixAffineTransformation(
        XMVECTOR{ 1.0f, 1.0f, 1.0f, 0.0f }, // FXMVECTOR Scaling, 
        centerOfMass, // FXMVECTOR RotationOrigin, 
        XMQuaternionRotationRollPitchYaw( 0.0f, /* float Pitch, */ 0.0f, /* float Yaw, */ m_orientation ), // FXMVECTOR RotationQuaternion, 
        m_position ); // GXMVECTOR Translation);

    //D3DXVECTOR3 position = m_position;
    //const XMVECTOR orientation = XMQuaternionRotationRollPitchYaw( 0.0f, /* float Pitch, */ 0.0f, /* float Yaw, */ m_orientation ); /* float Roll); */
    ////D3DXQuaternionRotationYawPitchRoll( &orientation, 0.0f, 0.0, m_orientation );
    //D3DXVECTOR3 const rotationCenter = centerOfMass;
    //D3DXMatrixAffineTransformation( 
    //    &m_transformation, // XMMATRIX *pOut, 
    //    1.0f, // FLOAT Scaling, 
    //    &rotationCenter,
    //    &orientation, // CONST XMVECTOR *pRotation, 
    //    &position ); // CONST D3DXVECTOR3 *pTranslation);
}


}
