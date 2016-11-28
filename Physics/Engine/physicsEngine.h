// Andrew Davies

#if !defined( PHYSICS_ENGINE_H )
#define PHYSICS_ENGINE_H

#include "Physics/Engine/physicsEngineFwd.h"
#include "Physics/Shape/physicsShape.h"
#include "Physics/Object/physicsObject.h"
#include "Physics/HeightMap/physicsHeightMap.h"
#include <DirectXMath.h>
#include "Dav/container/container.hpp"
#include "Math/Matrix4.h"
#include "CommandContext.h" // class GraphicsContext;

namespace Physics
{

#define PHYSICS_SHAPE_NULL_UID ( INT_MAX )
#define PHYSICS_OBJECT_NULL_UID ( INT_MAX )
#define PHYSICS_HEIGHT_MAP_NULL_UID ( INT_MAX )

#define PHYSICS_MAX_SHAPES ( ( int )128 )
#define PHYSICS_MAX_OBJECTS ( ( int )128 )
#define PHYSICS_MAX_HEIGHT_MAPS ( ( int )2 )
    
typedef Dav::List< Shape, PHYSICS_MAX_SHAPES > EngineShapeListType;
typedef Dav::List< Object, PHYSICS_MAX_OBJECTS > EngineObjectListType;
typedef Dav::List< HeightMap, PHYSICS_MAX_HEIGHT_MAPS > EngineHeightMapList;

class EngineClass
{
private:
    int m_nextShapeUID;
    EngineShapeListType m_shapeList;

    int m_nextObjectUID;
    EngineObjectListType m_objectList;

    int m_nextHeightMapUID;
    EngineHeightMapList m_heightMapList;

    int m_stepCount;

public:
    EngineClass();

    void create();
    void destroy();

    void step( float deltaTime );

    void draw(
        GraphicsContext& gfxContext,
        const Math::Matrix4& ViewProjMat ) const;

    int createShape(
        XMVECTORVectorType const & vertexVector,
        DirectX::XMVECTOR const & position,
        float orientation,
        DirectX::XMVECTOR const & velocity,
        float angularVelocity,
        DirectX::XMVECTOR const & force,
        float torque,
        bool infiniteMass );

    void destroyShape( int UID );

    Shape const * shape( int UID ) const;
    Shape * shape( int UID );
    
    bool anyShapesInCollision() const;

    int nextShapeUID() const
    {
        return m_nextShapeUID;
    }

    EngineShapeListType const & shapeList() const
    {
        return m_shapeList;
    }

    int createObject(
        float mass,
        bool infiniteMass,
        const DirectX::XMVECTOR& centerOfMassLocalPosition,
        const DirectX::XMVECTOR& inertia,
        const Dynamics& dynamics,
        const DirectX::XMVECTOR& force,
        const DirectX::XMVECTOR& torque,
        const Object::Vertices& vertices,
        const Object::Edges& edges,
        const Object::Triangles& triangles );

    void destroyObject( int UID );

    const Object* object( int UID ) const;
    Object* object( int UID );
    
    //bool anyObjectsInCollision() const;

    int nextObjectUID() const
    {
        return m_nextObjectUID;
    }
        
    EngineObjectListType const & objectList() const
    {
        return m_objectList;
    }

    int createHeightMap( 
        const DirectX::XMVECTOR& position,
        int imageWidth, int imageHeight, const std::vector< unsigned char >& imageRGBA,
        int heightsStartX, int heightsStartZ, int numHeightsX, int numHeightsZ );
        //IDirect3DDevice9& d3dDevice );

    void destroyHeightMap( int UID );

    const HeightMap* heightMap( int UID ) const;
    HeightMap* heightMap( int UID );
    
    bool anyHeightMapsInCollision() const;

    int nextHeightMapUID() const
    {
        return m_nextHeightMapUID;
    }

    const EngineHeightMapList& heightMapList() const
    {
        return m_heightMapList;
    }



    int stepCount() const
    {
        return m_stepCount;
    }

private:
    void stepObjects( float deltaTime );
    void stepShapes( float deltaTime );

};

}

#endif