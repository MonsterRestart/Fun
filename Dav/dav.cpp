// Andrew Davies 2011

#include "pch.h"
#include "Dav/dav.h"

//using namespace std;
using namespace DirectX;

namespace Dav
{

VertexClass::VertexClass( const XMVECTOR& position )
    : m_position( position )
{
}

FaceClass::FaceClass()
    : m_vertexIndexVector()
{
}

FaceClass::FaceClass( const VertexIndexVectorType& vertexIndexVector )
    : m_vertexIndexVector( vertexIndexVector )
{
}

void FaceClass::addVertexIndex( const int vertexIndex )
{
    m_vertexIndexVector.push_back( vertexIndex );
}

ObjectClass::ObjectClass(
    const std::string& name,	
    const bool infiniteMass,
    const XMMATRIX& transformation,
    const VertexVectorType& vertexVector,
    const FaceVectorType& faceVector )
    : m_name( name )
    , m_infiniteMass( infiniteMass )
    , m_transformation( transformation )
    , m_vertexVector( vertexVector )
    , m_faceVector( faceVector )
{
}

ObjectClass::ObjectClass(
    const std::string& name,
    const bool infiniteMass,
    const XMMATRIX& transformation )
    : m_name( name )
    , m_infiniteMass( infiniteMass )
    , m_transformation( transformation )
    , m_vertexVector( )
    , m_faceVector( )
{
}

void ObjectClass::addVertex( const VertexClass& vertex )
{
    m_vertexVector.push_back( vertex );
}

void ObjectClass::addFace( const FaceClass& face )
{
    m_faceVector.push_back( face );
}


//struct ObjectVertexStruct
//{
//    FLOAT x, y, z; //, rhw; // The transformed position for the vertex
//    DWORD colour;        // The vertex colour
//};
//#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE) // (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
//
//void ObjectClass::draw( IDirect3DDevice9 & d3dDevice ) const
//{
//    DWORD const colour = D3DCOLOR_COLORVALUE( 0.0f, 1.0f, 0.0f, 1.0f );
//
//    for( FaceVectorType::const_iterator faceItr = m_faceVector.begin(); faceItr != m_faceVector.end(); ++faceItr )
//    {
//        assert( faceItr->vertexIndexVector().size() == 3 );
//        if( faceItr->vertexIndexVector().size() == 3 )
//        {
//            ObjectVertexStruct vertexArray[ 3 ];
//        
//            vertexArray[ 0 ].x = m_vertexVector[ faceItr->vertexIndexVector()[ 0 ] ].position().x;
//            vertexArray[ 0 ].y = m_vertexVector[ faceItr->vertexIndexVector()[ 0 ] ].position().y;
//            vertexArray[ 0 ].z = m_vertexVector[ faceItr->vertexIndexVector()[ 0 ] ].position().z;
//            vertexArray[ 0 ].colour = colour;
//            vertexArray[ 1 ].x = m_vertexVector[ faceItr->vertexIndexVector()[ 1 ] ].position().x;
//            vertexArray[ 1 ].y = m_vertexVector[ faceItr->vertexIndexVector()[ 1 ] ].position().y;
//            vertexArray[ 1 ].z = m_vertexVector[ faceItr->vertexIndexVector()[ 1 ] ].position().z;
//            vertexArray[ 1 ].colour = colour;
//            vertexArray[ 2 ].x = m_vertexVector[ faceItr->vertexIndexVector()[ 2 ] ].position().x;
//            vertexArray[ 2 ].y = m_vertexVector[ faceItr->vertexIndexVector()[ 2 ] ].position().y;
//            vertexArray[ 2 ].z = m_vertexVector[ faceItr->vertexIndexVector()[ 2 ] ].position().z;
//            vertexArray[ 2 ].colour = colour;
//
//            d3dDevice.DrawPrimitiveUP( 
//                D3DPT_TRIANGLELIST, // D3DPRIMITIVETYPE PrimitiveType,
//                1, // UINT PrimitiveCount,
//                vertexArray, // CONST void* pVertexStreamZeroData,
//                sizeof( ObjectVertexStruct ) ); // UINT VertexStreamZeroStride
//        }
//    }
//}


SceneClass::SceneClass()
    : m_objectVector( )
{
}

SceneClass::SceneClass( const ObjectVectorType& objectVector )
    : m_objectVector( objectVector )
{
}
    
void SceneClass::addObject( const ObjectClass& object )
{
    m_objectVector.push_back( object );
}

//MeshClass::MeshClass()
//	: m_numVertices( 0 )
//	, m_vertexBuffer( 0 )
//	, m_numTriangles( 0 )
//	, m_indexBuffer( 0 )
//{
//}
//
//struct MeshVertexStruct
//{
//    FLOAT x, y, z; //, rhw; // The transformed position for the vertex
//    DWORD colour;        // The vertex colour
//};
//#define DAV_MESH_D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE) // (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
//
//void MeshClass::create( 
//	const ObjectClass& object,
//	IDirect3DDevice9& d3dDevice )
//{
//	// Create mesh's vertex buffer. Only bother if the object has any vertices.
//	m_numVertices = ( int )object.vertexVector().size();
//	if( m_numVertices != 0 )
//	{
//		const int vertexBufferSize = m_numVertices * sizeof( MeshVertexStruct );
//
//		if( FAILED( d3dDevice.CreateVertexBuffer( 
//				vertexBufferSize,
//				0 /*Usage*/, 
//				DAV_MESH_D3DFVF_CUSTOMVERTEX, 
//				D3DPOOL_DEFAULT, 
//				&m_vertexBuffer, 
//				NULL ) ) )
//		{
//			assert( 0 );
//		}
//
//		// Copy vertices into the buffer
//		MeshVertexStruct* vertexPtr = 0;
//		if( FAILED( m_vertexBuffer->Lock( 0, vertexBufferSize, ( void ** )&vertexPtr, 0 ) ) )
//		{
//			assert( 0 );
//		}
//
//		for(	VertexVectorType::const_iterator objVertItr = object.vertexVector().begin();
//				objVertItr != object.vertexVector().end(); ++objVertItr, ++vertexPtr )
//		{
//			vertexPtr->x = objVertItr->position().x;
//			vertexPtr->y = objVertItr->position().y;
//			vertexPtr->z = objVertItr->position().z;
//			vertexPtr->colour = D3DCOLOR_COLORVALUE( 0.0f, 1.0f, 0.0f, 1.0f );
//		}
//
//		m_vertexBuffer->Unlock();
//	}
//	
//	// Create mesh's index buffer. Only bother if the object has any faces (which are assumed to be triangles using three verts each)
//	m_numTriangles = ( int )object.faceVector().size();
//	if( m_numTriangles != 0 )
//	{	
//		const int numIndices = m_numTriangles * 3;
//		const int indexBufferSize = numIndices * sizeof( WORD );
//
//		if( FAILED( d3dDevice.CreateIndexBuffer( 
//				indexBufferSize,
//				D3DUSAGE_WRITEONLY, 
//				D3DFMT_INDEX16, 
//				D3DPOOL_DEFAULT, 
//				&m_indexBuffer,
//				NULL ) ) )
//		{
//			assert( 0 );
//		}		
//
//		// Copy indices into the buffer
//		WORD* indexPtr = 0;
//		if( FAILED( m_indexBuffer->Lock( 0, indexBufferSize, ( void ** )&indexPtr, 0 ) ) )
//		{
//			assert( 0 );
//		}
//
//		for(	FaceVectorType::const_iterator faceItr = object.faceVector().begin();
//				faceItr != object.faceVector().end(); ++faceItr )
//		{
//			assert( faceItr->vertexIndexVector().size() == 3 );
//			*indexPtr = ( WORD )faceItr->vertexIndexVector()[ 0 ];
//			++indexPtr;			
//			*indexPtr = ( WORD )faceItr->vertexIndexVector()[ 1 ];
//			++indexPtr;
//			*indexPtr = ( WORD )faceItr->vertexIndexVector()[ 2 ];
//			++indexPtr;
//		}
//
//		m_indexBuffer->Unlock();
//	}
//
//}
//
//void MeshClass::destroy()
//{
//	// NULL safe and NULLs
//	SAFE_RELEASE( m_indexBuffer );
//	m_numTriangles = 0;
//	SAFE_RELEASE( m_vertexBuffer );
//	m_numVertices = 0;
//}
//
//void MeshClass::draw( IDirect3DDevice9& d3dDevice ) const
//{
//	d3dDevice.SetStreamSource( 0, m_vertexBuffer, 0, sizeof( MeshVertexStruct ) );
//	d3dDevice.SetIndices( m_indexBuffer );
//	d3dDevice.DrawIndexedPrimitive(
//		D3DPT_TRIANGLELIST,
//		0, // [in]  INT BaseVertexIndex,
//		0, // [in]  UINT MinIndex,
//		m_numVertices, 
//		0, // [in]  UINT StartIndex,
//		m_numTriangles ); // [in]  UINT PrimitiveCount
//
//
//}



} // namespace Dav


