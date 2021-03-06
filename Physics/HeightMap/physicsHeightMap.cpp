// Andrew Davies

#include "pch.h"
#include "Physics/HeightMap/physicsHeightMap.h"
#include <iostream>
#include "Misc/misc.h"

using namespace std;
using namespace DirectX;

namespace Physics
{

HeightMap::HeightMap()
    : m_UID( 0 )
    , m_position{ 0.0f, 0.0f, 0.0f, 1.0f }
    , m_numTriangles( 0 )
    , m_numVertices( 0 )
    , m_numIndices( 0 )
    //, m_D3D9VertexBufferPtr( nullptr )
    //, m_D3D9IndexBufferPtr( nullptr )
    //, m_D3D9TexturePtr( nullptr )
{
}

void HeightMap::create(
    const int UID,
    const XMVECTOR& position,
    const int imageWidth,
    const int imageHeight,
    const vector< unsigned char >& imageRGBA,
    const int heightsStartX,
    const int heightsStartZ,
    const int numHeightsX,
    const int numHeightsZ )
    //IDirect3DDevice9& d3dDevice )
{
    m_UID = UID;
    m_position = position;
    assert( m_heights.empty() );
    assert( m_vertices.empty() );
    assert( m_edges.empty() );
    assert( m_triangles.empty() );
    //assert( m_d3d9Vertices.empty() );
    //assert( m_d3d9Indices.empty() );
    assert( m_numTriangles == 0 );
    assert( m_numVertices == 0 );
    assert( m_numIndices == 0 );
    //assert( m_D3D9VertexBufferPtr == nullptr );
    //assert( m_D3D9IndexBufferPtr == nullptr );
    //assert( m_D3D9TexturePtr == nullptr );

    // Need at least 2 points in both directions
    if( ( numHeightsX > 1 ) && ( numHeightsZ > 1 ) )
    {
        float minHeight = FLT_MAX;
        float maxHeight = -minHeight;

        // Convert image to heights and grab height range
        m_heights.reserve( numHeightsZ );
        for( int heightsZ = 0; heightsZ != numHeightsZ; ++heightsZ )
        {
            const int imageZ = min( ( heightsStartZ + heightsZ ), ( imageHeight - 1 ) );

            HeightRow heightRow;
            heightRow.reserve( numHeightsX );

            for( int heightsX = 0; heightsX != numHeightsX; ++heightsX )
            {
                const int imageX = min( ( heightsStartX + heightsX ), ( imageWidth - 1 ) );
                
                const int imageRPixelIndex = min( ( imageZ * imageWidth * 4 ) + ( imageX * 4 ), ( int )imageRGBA.size() );
                const unsigned char iHeight = imageRGBA[ imageRPixelIndex ];
                const float height = ( float )iHeight / 255.0f;
                
                heightRow.push_back( height );

                minHeight = min( minHeight, height );
                maxHeight = max( maxHeight, height );
            }
            m_heights.push_back( heightRow );
        }

        // Normalise heights
        for( int heightsZ = 0; heightsZ != numHeightsZ; ++heightsZ )
        {
            for( int heightsX = 0; heightsX != numHeightsX; ++heightsX )
            {
                const float fHeight = m_heights[ heightsZ ][ heightsX ];
                const float normalisedHeight = ( fHeight - minHeight ) / ( maxHeight - minHeight );
                assert( normalisedHeight <= 1.0f );
                assert( normalisedHeight >= 0.0f );
                
                m_heights[ heightsZ ][ heightsX ] = normalisedHeight;
             }
        }

        // Vertices
        for( int heightsZ = 0; heightsZ != numHeightsZ; ++heightsZ )
        {
            for( int heightsX = 0; heightsX != numHeightsX; ++heightsX )
            {
                const float size = 128.0f;
                const float height = 50.0f;
                
                m_vertices.push_back( Vertex( XMVECTOR{
                    ( -size / 2.0f ) + ( ( ( ( float )heightsX ) * size ) / ( float )( numHeightsX - 1 ) ),
                    m_heights[ heightsZ ][ heightsX ] * height,
                    ( -size / 2.0f ) + ( ( ( ( float )heightsZ ) * size ) / ( float )( numHeightsX - 1 ) ),
                    1.0f }, // const XMVECTOR& position,
                    XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f } ) ); // const XMVECTOR& normal,
                    //D3DCOLORVALUE{ 1.0f, 1.0f, 1.0f, 1.0f }, // const D3DCOLORVALUE& colour,
                    //0.0f, 0.0f ) );
                assert( XMVectorGetY( m_vertices.back().position() ) <= height );
            }
        }

        // 'Horizontal' Edges
        for( int heightsZ = 0; heightsZ != numHeightsZ; ++heightsZ )
        {
            for( int heightsX = 0; heightsX != ( numHeightsX - 1 ); ++heightsX )
            {
                const int startVertexIndex = ( heightsZ * numHeightsX ) + heightsX;
                const int endVertexIndex = startVertexIndex + 1;
                assert( startVertexIndex < ( int )m_vertices.size() );
                assert( endVertexIndex < ( int )m_vertices.size() );

                const int edgeIndex = ( int )m_edges.size();
                m_edges.push_back( Edge( XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f } ) );
                m_edges.back().addVertexIndex( startVertexIndex );
                m_edges.back().addVertexIndex( endVertexIndex );
                m_vertices[ startVertexIndex ].addEdgeIndex( edgeIndex );
                m_vertices[ endVertexIndex ].addEdgeIndex( edgeIndex );
            }
        }

        // 'Vertical' Edges
        for( int heightsZ = 0; heightsZ != ( numHeightsZ - 1 ); ++heightsZ )
        {
            for( int heightsX = 0; heightsX != numHeightsX; ++heightsX )
            {
                const int startVertexIndex = ( heightsZ * numHeightsX ) + heightsX;
                const int endVertexIndex = startVertexIndex + numHeightsX;
                assert( startVertexIndex < ( int )m_vertices.size() );
                assert( endVertexIndex < ( int )m_vertices.size() );
                
                const int edgeIndex = ( int )m_edges.size();
                m_edges.push_back( Edge( XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f } ) );
                m_edges.back().addVertexIndex( startVertexIndex );
                m_edges.back().addVertexIndex( endVertexIndex );
                m_vertices[ startVertexIndex ].addEdgeIndex( edgeIndex );
                m_vertices[ endVertexIndex ].addEdgeIndex( edgeIndex );
            }
        }

        // 'Diagonal' Edges
        for( int heightsZ = 0; heightsZ != ( numHeightsZ - 1 ); ++heightsZ )
        {
            for( int heightsX = 0; heightsX != ( numHeightsX - 1 ); ++heightsX )
            {
                const int startVertexIndex = ( ( heightsZ + 1 ) * numHeightsX ) + heightsX;
                const int endVertexIndex = ( heightsZ * numHeightsX ) + heightsX + 1;
                assert( startVertexIndex < ( int )m_vertices.size() );
                assert( endVertexIndex < ( int )m_vertices.size() );
                
                const int edgeIndex = ( int )m_edges.size();
                m_edges.push_back( Edge( XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f } ) );
                m_edges.back().addVertexIndex( startVertexIndex );
                m_edges.back().addVertexIndex( endVertexIndex );
                m_vertices[ startVertexIndex ].addEdgeIndex( edgeIndex );
                m_vertices[ endVertexIndex ].addEdgeIndex( edgeIndex );
            }
        }

        // Triangles
        for( int heightsZ = 0; heightsZ != ( numHeightsZ - 1 ); ++heightsZ )
        {
            for( int heightsX = 0; heightsX != ( numHeightsX - 1 ); ++heightsX )
            {
                const int vertexAIndex = ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 0 );
                const int vertexBIndex = ( ( heightsZ + 1 ) * numHeightsX ) + ( heightsX + 0 );
                const int vertexCIndex = ( ( heightsZ + 1 ) * numHeightsX ) + ( heightsX + 1 );
                const int vertexDIndex = ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 1 );
                assert( vertexAIndex < ( int )m_vertices.size() );
                assert( vertexBIndex < ( int )m_vertices.size() );
                assert( vertexCIndex < ( int )m_vertices.size() );
                assert( vertexDIndex < ( int )m_vertices.size() );
                const XMVECTOR vertexAPosition = m_vertices[ vertexAIndex ].position();
                const XMVECTOR vertexBPosition = m_vertices[ vertexBIndex ].position();
                const XMVECTOR vertexCPosition = m_vertices[ vertexCIndex ].position();
                const XMVECTOR vertexDPosition = m_vertices[ vertexDIndex ].position();

                const int edgeAIndex = ( ( heightsZ + 0 ) * ( numHeightsX - 1 ) ) + ( heightsX + 0 );
                const int edgeBIndex = ( ( heightsZ + 1 ) * ( numHeightsX - 1 ) ) + ( heightsX + 0 );
                const int edgeCIndex = ( numHeightsZ * ( numHeightsX - 1 ) ) + ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 0 );
                const int edgeDIndex = ( numHeightsZ * ( numHeightsX - 1 ) ) + ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 1 );
                const int edgeEIndex = ( ( numHeightsZ - 1 ) * numHeightsX ) + ( numHeightsZ * ( numHeightsX - 1 ) ) + ( ( heightsZ + 0 ) * ( numHeightsX - 1 ) ) + ( heightsX + 0 );
                assert( edgeAIndex < ( int )m_edges.size() );
                assert( edgeBIndex < ( int )m_edges.size() );
                assert( edgeCIndex < ( int )m_edges.size() );
                assert( edgeDIndex < ( int )m_edges.size() );
                assert( edgeEIndex < ( int )m_edges.size() );

                const XMVECTOR bToADirection = vertexAPosition - vertexBPosition;
                const XMVECTOR bToCDirection = vertexCPosition - vertexBPosition;
                const XMVECTOR triangleANormal = XMVector4Normalize( XMVector4Cross( bToCDirection, bToADirection, Misc::origin() ) );
                //D3DXVec4Cross( &triangleANormal, &bToCDirection, &bToADirection, &Misc::origin() );
                //triangleANormal = XMVector4Normalize( triangleANormal );
                //D3DXVec4Normalize( &triangleANormal, &triangleANormal );
                
                const int triangleAIndex = ( int )m_triangles.size();
                m_triangles.push_back( Triangle( triangleANormal ) );
                m_triangles.back().addVertexIndex( vertexAIndex );
                m_triangles.back().addVertexIndex( vertexBIndex );
                m_triangles.back().addVertexIndex( vertexCIndex );
                m_vertices[ vertexAIndex ].addTriangleIndex( triangleAIndex );
                m_vertices[ vertexBIndex ].addTriangleIndex( triangleAIndex );
                m_vertices[ vertexCIndex ].addTriangleIndex( triangleAIndex );
                m_triangles.back().addEdgeIndex( edgeAIndex );
                m_triangles.back().addEdgeIndex( edgeCIndex );
                m_triangles.back().addEdgeIndex( edgeEIndex );
                m_edges[ edgeAIndex ].addTriangleIndex( triangleAIndex );
                m_edges[ edgeCIndex ].addTriangleIndex( triangleAIndex );
                m_edges[ edgeEIndex ].addTriangleIndex( triangleAIndex );

                const XMVECTOR cToADirection = vertexAPosition - vertexCPosition;
                const XMVECTOR cToCDirection = vertexDPosition - vertexCPosition;
                const XMVECTOR triangleBNormal = XMVector4Normalize( XMVector4Cross( cToCDirection, cToADirection, Misc::origin() ) );
                //D3DXVec4Cross( &triangleBNormal, &cToCDirection, &cToADirection, &Misc::origin() );
                //triangleBNormal = XMVector4Normalize( triangleBNormal );
                //D3DXVec4Normalize( &triangleBNormal, &triangleBNormal );
                
                const int triangleBIndex = ( int )m_triangles.size();
                m_triangles.push_back( Triangle( triangleBNormal ) );
                m_triangles.back().addVertexIndex( vertexAIndex );
                m_triangles.back().addVertexIndex( vertexCIndex );
                m_triangles.back().addVertexIndex( vertexDIndex );
                m_vertices[ vertexAIndex ].addTriangleIndex( triangleBIndex );
                m_vertices[ vertexCIndex ].addTriangleIndex( triangleBIndex );
                m_vertices[ vertexDIndex ].addTriangleIndex( triangleBIndex );
                m_triangles.back().addEdgeIndex( edgeEIndex );
                m_triangles.back().addEdgeIndex( edgeBIndex );
                m_triangles.back().addEdgeIndex( edgeDIndex );
                m_edges[ edgeEIndex ].addTriangleIndex( triangleBIndex );
                m_edges[ edgeBIndex ].addTriangleIndex( triangleBIndex );
                m_edges[ edgeDIndex ].addTriangleIndex( triangleBIndex );
            }
        }

        // Edge Normals
        for( auto& edge : m_edges )
        {
            assert( ( int )edge.triangleIndices().size() > 0 );
            XMVECTOR edgeNormal = Misc::zero();
            for( auto triangleIndex : edge.triangleIndices() )
            {
                assert( triangleIndex < ( int )m_triangles.size() );
                edgeNormal += m_triangles[ triangleIndex ].normal();
            }
            edgeNormal /= ( float )edge.triangleIndices().size();
            edgeNormal = XMVector4Normalize( edgeNormal );
            //D3DXVec4Normalize( &edgeNormal, &edgeNormal );
            edge.setNormal( edgeNormal );
        }

        // Vertex Normals
        for( auto& vertex : m_vertices )
        {
            assert( ( int )vertex.triangleIndices().size() > 0 );
            XMVECTOR vertexNormal = Misc::zero();
            for( auto triangleIndex : vertex.triangleIndices() )
            {
                assert( triangleIndex < ( int )m_triangles.size() );
                vertexNormal += m_triangles[ triangleIndex ].normal();
            }
            vertexNormal /= ( float )vertex.triangleIndices().size();
            vertexNormal = XMVector4Normalize( vertexNormal );
            //D3DXVec4Normalize( &vertexNormal, &vertexNormal );
            vertex.setNormal( vertexNormal );
        }

        m_numVertices = numHeightsX * numHeightsZ;
        assert( m_numVertices == ( int )m_vertices.size() );
        m_numTriangles = ( numHeightsX - 1 ) * ( numHeightsZ - 1 ) * 2;
        assert( m_numTriangles == ( int )m_triangles.size() );
        m_numIndices = m_numTriangles * 3;
 
        //m_d3d9Vertices.reserve( m_numVertices );
        //m_d3d9Indices.reserve( m_numIndices );

    //    const HRESULT vhr = d3dDevice.CreateVertexBuffer(
    //        m_numVertices * sizeof( D3D9Vertex ),
    //        0, // DWORD Usage,
    //        PHYSICS_HEIGHT_MAP_D3D9_VERTEX_FORMAT,
    //        D3DPOOL_MANAGED,
    //        &m_D3D9VertexBufferPtr,
    //        nullptr);

    //    const HRESULT ihr = d3dDevice.CreateIndexBuffer(
    //        m_numIndices * sizeof( D3D9Index ),
    //        0, // DWORD Usage,
    //        PHYSICS_HEIGHT_MAP_D3D9_INDEX_FORMAT,
    //        D3DPOOL_MANAGED,
    //        &m_D3D9IndexBufferPtr,
    //        nullptr);

    //    const HRESULT thr = D3DXCreateTextureFromFile(
    //        &d3dDevice,
    //        L"ground_slope_0055_01_preview.jpg",
    //        &m_D3D9TexturePtr );

    //    if( SUCCEEDED( vhr ) && m_D3D9VertexBufferPtr &&
    //        SUCCEEDED( ihr ) && m_D3D9IndexBufferPtr &&
    //        SUCCEEDED( thr ) && m_D3D9TexturePtr )
    //    {
    //        D3D9Vertex* d3d9Vertices = nullptr;
    //        m_D3D9VertexBufferPtr->Lock(0, 0, (void**)&d3d9Vertices, 0);

    //        for( int heightsZ = 0; heightsZ != numHeightsZ; ++heightsZ )
    //        {
    //            for( int heightsX = 0; heightsX != numHeightsX; ++heightsX )
    //            {
    //                const int vertexIndex = ( heightsZ * numHeightsX ) + heightsX;
    //                
    //                d3d9Vertices[ vertexIndex ].x = m_vertices[ vertexIndex ].position().x;
    //                d3d9Vertices[ vertexIndex ].y = m_vertices[ vertexIndex ].position().y;
    //                d3d9Vertices[ vertexIndex ].z = m_vertices[ vertexIndex ].position().z;
    //                //d3d9Vertices[ vertexIndex ].w = 0.0f; // m_vertices[ vertexIndex ].position().w;
    //                d3d9Vertices[ vertexIndex ].n.x = m_vertices[ vertexIndex ].normal().x;
    //                d3d9Vertices[ vertexIndex ].n.y = m_vertices[ vertexIndex ].normal().y;
    //                d3d9Vertices[ vertexIndex ].n.z = m_vertices[ vertexIndex ].normal().z;
    //                d3d9Vertices[ vertexIndex ].colour = D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
    //                    //( float )rand() / ( float )RAND_MAX,
    //                    //( float )rand() / ( float )RAND_MAX,
    //                    //( float )rand() / ( float )RAND_MAX,
    //                    //1.0f );
    //                d3d9Vertices[ vertexIndex ].tu = ( float )heightsX / ( float )( numHeightsX - 1 );
    //                d3d9Vertices[ vertexIndex ].tv = ( float )heightsZ / ( float )( numHeightsZ - 1 );

    //                //m_d3d9Vertices.push_back( d3d9Vertices[ vertexIndex ] );
    //            }
    //        }

    //        m_D3D9VertexBufferPtr->Unlock();

    //        D3D9Index* d3d9Indices = nullptr;
    //        m_D3D9IndexBufferPtr->Lock(0, 0, (void**)&d3d9Indices, 0);

    //        for( int heightsZ = 0; heightsZ != ( numHeightsZ -  1 ); ++heightsZ )
    //        {
    //            for( int heightsX = 0; heightsX != ( numHeightsX - 1 ); ++heightsX )
    //            {
    //                const int indexIndex = ( heightsZ * ( numHeightsX - 1 ) * 6 ) + ( heightsX * 6 );
    //                assert( indexIndex + 5 < m_numIndices );
    //                    
    //                d3d9Indices[ indexIndex + 0 ] = ( D3D9Index )( ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 0 ) );
    //                //m_d3d9Indices.push_back( d3d9Indices[ indexIndex + 0 ] );
    //                d3d9Indices[ indexIndex + 1 ] = ( D3D9Index )( ( ( heightsZ + 1 ) * numHeightsX ) + ( heightsX + 0 ) );
    //                //m_d3d9Indices.push_back( d3d9Indices[ indexIndex + 1 ] );
    //                d3d9Indices[ indexIndex + 2 ] = ( D3D9Index )( ( ( heightsZ + 1 ) * numHeightsX ) + ( heightsX + 1 ) );
    //                //m_d3d9Indices.push_back( d3d9Indices[ indexIndex + 2 ] );
    //                    
    //                d3d9Indices[ indexIndex + 3 ] = ( D3D9Index )( ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 0 ) );
    //                //m_d3d9Indices.push_back( d3d9Indices[ indexIndex + 3 ] );
    //                d3d9Indices[ indexIndex + 4 ] = ( D3D9Index )( ( ( heightsZ + 1 ) * numHeightsX ) + ( heightsX + 1 ) );
    //                //m_d3d9Indices.push_back( d3d9Indices[ indexIndex + 4 ] );
    //                d3d9Indices[ indexIndex + 5 ] = ( D3D9Index )( ( ( heightsZ + 0 ) * numHeightsX ) + ( heightsX + 1 ) );
    //                //m_d3d9Indices.push_back( d3d9Indices[ indexIndex + 5 ] );
    //            }
    //        }

    //        m_D3D9IndexBufferPtr->Unlock();
    //    }
    //    
    //    //assert( ( int )m_d3d9Vertices.size() == m_numVertices );
    //    //assert( ( int )m_d3d9Indices.size() == m_numIndices );

    }
}

void HeightMap::destroy()
{
    //m_d3d9Vertices.clear();
    //m_d3d9Indices.clear();
    
    //if( m_D3D9TexturePtr )
    //{
    //    m_D3D9TexturePtr->Release();
    //}

    //if( m_D3D9IndexBufferPtr )
    //{
    //    m_D3D9IndexBufferPtr->Release();
    //}
    //
    //if( m_D3D9VertexBufferPtr )
    //{
    //    m_D3D9VertexBufferPtr->Release();
    //}
}

void HeightMap::draw( /*IDirect3DDevice9 & d3dDevice*/ ) const
{
    //assert( m_D3D9VertexBufferPtr != nullptr );
    //assert( m_D3D9IndexBufferPtr != nullptr );
    //assert( m_D3D9TexturePtr != nullptr );

    //XMMATRIX transform;
    //D3DXMatrixIdentity( &transform );
    //D3DXMatrixTranslation( &transform, m_position.x, m_position.y, m_position.z );
    //d3dDevice.SetTransform( D3DTS_WORLD, &transform );
    //d3dDevice.SetFVF( PHYSICS_HEIGHT_MAP_D3D9_VERTEX_FORMAT );
    //d3dDevice.SetStreamSource(0, m_D3D9VertexBufferPtr, 0, sizeof( D3D9Vertex ));
    //d3dDevice.SetIndices( m_D3D9IndexBufferPtr );
    //d3dDevice.SetTexture( 0, m_D3D9TexturePtr ); 
    //d3dDevice.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    //d3dDevice.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
    //d3dDevice.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    //d3dDevice.SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );   //Ignored    
    //d3dDevice.SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC );
    //d3dDevice.SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC );
    //d3dDevice.SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC );
    //d3dDevice.DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numVertices, 0, m_numTriangles );
}

} // namespace Physics

