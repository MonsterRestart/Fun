#if !defined( PHYSICS_HEIGHT_MAP_H )
#define PHYSICS_HEIGHT_MAP_H

// Andrew Davies

#include "Physics/HeightMap/physicsHeightMapFwd.h"
#include <DirectXMath.h>
#include <vector>
#include "Physics/Vertex/physicsVertex.h"
#include "Physics/Edge/physicsEdge.h"
#include "Physics/Triangle/physicsTriangle.h"

namespace Physics
{


class HeightMap
{
public:
    typedef std::vector< float > HeightRow;
    typedef std::vector< HeightRow > Heights;

    typedef std::vector< Vertex > Vertices;
    typedef std::vector< Edge > Edges;
    typedef std::vector< Triangle > Triangles;

    struct D3D9Vertex
    {
        FLOAT x, y, z;//, w;
        FLOAT nx, ny, nz;
        //D3DVECTOR n;
        DWORD colour;
        FLOAT tu, tv;
    };
    #define PHYSICS_HEIGHT_MAP_D3D9_VERTEX_FORMAT ( D3DFVF_XYZ |  D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
    typedef std::vector< D3D9Vertex > D3D9Vertices;

    typedef short D3D9Index;
    #define PHYSICS_HEIGHT_MAP_D3D9_INDEX_FORMAT D3DFMT_INDEX16 // if HeightMapIndex is short
    //#define PHYSICS_HEIGHT_MAP_D3D9_INDEX_FORMAT D3DFMT_INDEX32 // if HeightMapIndex is int    
    typedef std::vector< D3D9Index > D3D9Indices;

private:
    int m_UID;
    DirectX::XMVECTOR m_position;
    Heights m_heights;

    Vertices m_vertices;
    Edges m_edges;
    Triangles m_triangles;

    //D3D9Vertices m_d3d9Vertices;
    //D3D9Indices m_d3d9Indices;
    int m_numTriangles;
    int m_numVertices;
    int m_numIndices;

    //IDirect3DVertexBuffer9* m_D3D9VertexBufferPtr;
    //IDirect3DIndexBuffer9* m_D3D9IndexBufferPtr;
    //IDirect3DTexture9* m_D3D9TexturePtr;

public:
    HeightMap();

    void create( 
        int UID, const DirectX::XMVECTOR& position,
        int imageWidth, int imageHeight, const std::vector< unsigned char >& imageRGBA,
        int heightsStartX, int heightsStartZ, int numHeightsX, int numHeightsZ );
        //IDirect3DDevice9& d3dDevice );

    void destroy();

    void draw( /*IDirect3DDevice9 & d3dDevice*/ ) const;
    
    int UID() const
    {
        return m_UID;
    }
    
    const DirectX::XMVECTOR& position() const
    {
        return m_position;
    }

    const Heights& heights() const
    {
        return m_heights;
    }

    const Vertices& vertices() const
    {
        return m_vertices;
    }

    const Edges& edges() const
    {
        return m_edges;
    }

    const Triangles& triangles() const
    {
        return m_triangles;
    }

    //const D3D9Vertices& d3d9Vertices() const
    //{
    //    return m_d3d9Vertices;
    //}

    //const D3D9Indices& d3d9Indices() const
    //{
    //    return m_d3d9Indices;
    //}

    //const IDirect3DVertexBuffer9* D3D9VertexBufferPtr() const
    //{
    //    return m_D3D9VertexBufferPtr;
    //}

    //const IDirect3DIndexBuffer9* D3D9IndexBufferPtr() const
    //{
    //    return m_D3D9IndexBufferPtr;
    //}

    //const IDirect3DTexture9* D3D9TexturePtr() const
    //{
    //    return m_D3D9TexturePtr;
    //}

    int numTriangles() const
    {
        return m_numTriangles;
    }

    int numVertices() const
    {
        return m_numVertices;
    }

    int numIndices() const
    {
        return m_numIndices;
    }
};

}

#endif
