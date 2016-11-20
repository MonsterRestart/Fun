// Andrew Davies

#if !defined( PHYSICS_DYNAMICS_H )
#define PHYSICS_DYNAMICS_H

#include "Physics/Dynamics/physicsDynamicsFwd.h"
#include <DirectXMath.h>

namespace Physics
{

class Dynamics
{
private:
    DirectX::XMVECTOR m_position;
    DirectX::XMVECTOR m_orientation;

    DirectX::XMVECTOR m_velocity;
    DirectX::XMVECTOR m_angularVelocity;
    
    DirectX::XMMATRIX m_transformation;

public:
    Dynamics( );

    Dynamics( 
        const DirectX::XMVECTOR& position,
        const DirectX::XMVECTOR& orientation,
        const DirectX::XMVECTOR& velocity,
        const DirectX::XMVECTOR& angularVelocity,
        const DirectX::XMMATRIX& transformation );

    Dynamics( 
        const DirectX::XMVECTOR& position,
        const DirectX::XMVECTOR& orientation,
        const DirectX::XMVECTOR& velocity,
        const DirectX::XMVECTOR& angularVelocity,
        const DirectX::XMVECTOR& centerOfMass );

    void set(
        const DirectX::XMVECTOR& position,
        const DirectX::XMVECTOR& orientation,
        const DirectX::XMVECTOR& velocity,
        const DirectX::XMVECTOR& angularVelocity,
        const DirectX::XMVECTOR& centerOfMass );

    void step(
        const DirectX::XMVECTOR& force,
        const DirectX::XMVECTOR& torque,
        float deltaTime,
        bool infiniteMass,
        float mass,
        const DirectX::XMMATRIX& inverseInertiaTensorWorld,
        const DirectX::XMVECTOR& centerOfMass );

    const DirectX::XMVECTOR& position() const
    {
        return m_position;
    }

    const DirectX::XMVECTOR& orientation() const
    {
        return m_orientation;
    }

    const DirectX::XMVECTOR& velocity() const
    {
        return m_velocity;
    }

    const DirectX::XMVECTOR& angularVelocity() const
    {
        return m_angularVelocity;
    }

    DirectX::FXMMATRIX transformation() const
    {
        return m_transformation;
    }
};


class Dynamics2D
{
private:
    DirectX::XMVECTOR m_position;
    float m_orientation;

    DirectX::XMVECTOR m_velocity;
    float m_angularVelocity;
    
    DirectX::XMMATRIX m_transformation;

public:
    Dynamics2D( );

    Dynamics2D( 
        DirectX::XMVECTOR const & position,
        float orientation,
        DirectX::XMVECTOR const & velocity,
        float angularVelocity,
        DirectX::XMMATRIX const & transformation );

    Dynamics2D( 
        const DirectX::XMVECTOR& position,
        float orientation,
        const DirectX::XMVECTOR& velocity,
        float angularVelocity,
        const DirectX::XMVECTOR& centerOfMass );

    void set(
        DirectX::XMVECTOR const & position,
        float orientation,
        DirectX::XMVECTOR const & velocity,
        float angularVelocity,
        DirectX::XMVECTOR const & centerOfMass );

    void step(
        const DirectX::XMVECTOR& force,
        float torque,
        float deltaTime,
        bool infiniteMass,
        float mass,
        float momentOfInertia,
        const DirectX::XMVECTOR& centerOfMass );

    DirectX::XMVECTOR const & position( ) const
    {
        return m_position;
    }

    float orientation( ) const
    {
        return m_orientation;
    }

    DirectX::XMVECTOR const & velocity( ) const
    {
        return m_velocity;
    }

    float angularVelocity( ) const
    {
        return m_angularVelocity;
    }
    
    DirectX::FXMMATRIX transformation() const
    {
        return m_transformation;
    }
};


}

#endif