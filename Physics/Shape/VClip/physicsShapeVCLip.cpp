// Andrew Davies

#include "pch.h"
#include "Physics/Shape/VClip/physicsShapeVCLip.h"
#include "Physics/Shape/physicsShape.h"
#include "Physics/Feature/physicsFeature.h"

using namespace std;
using namespace DirectX;

namespace Physics
{

//void renderFeatureConnection(
//	IDirect3DDevice9* pd3dDevice,
//	ShapeClass const & shapeA,
//	FeatureClass const & shapeAFeature,
//	float const shapeADistanceAlongEdge,
//	ShapeClass const & shapeB,
//	FeatureClass const & shapeBFeature,
//	float const shapeBDistanceAlongEdge )
//{
//	XMVECTOR fromPosition = shapeA.dynamics( ).position( );	
//	switch( shapeAFeature.type( ) )
//	{
//	case VertexFeature :
//		{
//			fromPosition = shapeA.transformedVertexPosition( shapeAFeature.index( ) );
//		}
//		break;
//
//	case EdgeFeature :
//	case FaceFeature :
//	case numberOfFeatures :
//	default :
//		{
//			fromPosition = 
//				shapeA.transformedVertexPosition( shapeAFeature.index( ) ) +
//				( shapeA.transformedEdgeDirectionUnit( shapeAFeature.index( ) ) * shapeADistanceAlongEdge );
//				//( shapeA.transformedEdgeDirection( shapeAFeature.index( ) ) * 0.5f );
//		}
//		break;
//	}
//
//	XMVECTOR toPosition = shapeB.dynamics( ).position( );
//	switch( shapeBFeature.type( ) )
//	{
//	case VertexFeature :
//		{
//			toPosition = shapeB.transformedVertexPosition( shapeBFeature.index( ) );
//		}
//		break;
//
//	case EdgeFeature :
//	case FaceFeature :
//	case numberOfFeatures :
//	default :
//		{
//			toPosition = 
//				shapeB.transformedVertexPosition( shapeBFeature.index( ) ) +
//				( shapeB.transformedEdgeDirectionUnit( shapeBFeature.index( ) ) * shapeBDistanceAlongEdge );
//				//( shapeB.transformedEdgeDirection( shapeBFeature.index( ) ) * 0.5f );
//		}
//		break;
//	}
//
//	CUSTOMVERTEX vertexArray[ 2 ];
//	vertexArray[ 0 ].x = fromPosition.x;
//	vertexArray[ 0 ].y = fromPosition.y;
//	vertexArray[ 0 ].z = fromPosition.z;
//	vertexArray[ 0 ].colour = D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
//	vertexArray[ 1 ].x = toPosition.x;
//	vertexArray[ 1 ].y = toPosition.y;
//	vertexArray[ 1 ].z = toPosition.z;
//	vertexArray[ 1 ].colour = D3DCOLOR_COLORVALUE( 1.0f, 1.0f, 1.0f, 1.0f );
//		
//	D3DXMATRIXA16 matWorld;
//	D3DXMatrixIdentity( &matWorld );
//	pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
//	pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
//	pd3dDevice->DrawPrimitiveUP( 
//		D3DPT_LINELIST, // D3DPRIMITIVETYPE PrimitiveType,
//		1, // UINT PrimitiveCount,
//		vertexArray, // CONST void* pVertexStreamZeroData,
//		sizeof( CUSTOMVERTEX ) ); // UINT VertexStreamZeroStride
//}

//float shapeSignedDistanceFromPlaneToPoint(
//	XMVECTOR const & planePosition,
//	XMVECTOR const & planeUnitNormal,
//	XMVECTOR const & point )
//{
//	XMVECTOR const planePositionToPointDirection =  point - planePosition;
//	return D3DXVec4Dot( &planeUnitNormal, &planePositionToPointDirection );
//}
//
//bool shapeBHasSeperatingEdge( ShapeClass const & shapeA, ShapeClass const & shapeB )
//{	
//	unsigned int const shapeANumberOfVertices = shapeA.numberOfVertices( );
//	unsigned int const shapeBNumberOfEdges = shapeB.numberOfEdges( );
//
//	unsigned int shapeBEdgeIndex = 0;
//	while( shapeBEdgeIndex != shapeBNumberOfEdges )
//	{
//		XMVECTOR const shapeBEdgePosition = shapeB.transformedVertexPosition( shapeBEdgeIndex );
//		XMVECTOR const shapeBEdgeNormalDirectionUnit = shapeB.transformedEdgeNormalDirectionUnit( shapeBEdgeIndex );
//
//		unsigned int shapeAVertexIndex = 0;
//		while( shapeAVertexIndex != shapeANumberOfVertices )
//		{		
//			XMVECTOR const shapeAVertexPosition = shapeA.transformedVertexPosition( shapeAVertexIndex );
//			
//			float const perpDot = shapeSignedDistanceFromPlaneToPoint( shapeBEdgePosition, shapeBEdgeNormalDirectionUnit, shapeAVertexPosition );
//
//			if( perpDot > 0.0f ) 
//			{
//				++shapeAVertexIndex;
//				
//				if( shapeAVertexIndex == shapeANumberOfVertices )
//				{
//					// found an edge that all a's vertices are on the correct side of so we must not be colliding.
//					return true;
//				}
//			}
//			else
//			{
//				// We've found a vertex that's on the wrong side of this edge.
//				// That doesn't mean we're definatley intersecting but it does mean we don't need to test this edge anymore.
//				// Move on to the next edge by breaking out of the a vertex loop
//				shapeAVertexIndex = shapeANumberOfVertices;
//			}			
//		}
//
//		++shapeBEdgeIndex;
//	}
//	
//	return false;
//}

class PlaneClass
{
private:
    XMVECTOR m_position;
    XMVECTOR m_unitNormal;

    FeatureClass m_neighborFeature;

public:
    PlaneClass( 
        XMVECTOR const & position, 
        XMVECTOR const & unitNormal,
        FeatureClass const & neighborFeature )
        : m_position( position )
        , m_unitNormal( unitNormal )
        , m_neighborFeature( neighborFeature )
    {
    }

    XMVECTOR const & position( ) const
    {
        return m_position;
    }

    XMVECTOR const & unitNormal( ) const
    {
        return m_unitNormal;
    }

    FeatureClass const & neighborFeature( ) const
    {
        return m_neighborFeature;
    }
};

//typedef std::vector< PlaneClass > PlaneVectorType;

// Clip the edge from t to h against the Voronoi planes in S. Return FALSE if the edge is completely clipped, otherwise TRUE.
bool clipEdge(
    XMVECTOR const & tailPosition,	
    XMVECTOR const & headPosition,
    PlaneClass const * const planeArray,
    const int numberOfPlanes,
    //PlaneVectorType const & planeVector,	
    float & tailIntersectionParameter,
    float & headIntersectionParameter,
    FeatureClass & tailNeighborFeature,
    FeatureClass & headNeighborFeature )
{
    assert( planeArray != 0 );
    assert( numberOfPlanes > 0 );

    tailIntersectionParameter = 0.0f;
    headIntersectionParameter = 1.0f;
    tailNeighborFeature = FeatureClass( );
    headNeighborFeature = FeatureClass( );

    //for( PlaneVectorType::const_iterator planeItr = planeVector.begin( ); planeItr != planeVector.end( ); ++planeItr )	
    PlaneClass const * const endPlane = planeArray + numberOfPlanes;
    for( PlaneClass const * planePtr = planeArray; planePtr != endPlane; ++planePtr )
    {
        float const tailDistance = shapeSignedDistanceFromPlaneToPoint( planePtr->position( ), planePtr->unitNormal( ), tailPosition );
        float const headDistance = shapeSignedDistanceFromPlaneToPoint( planePtr->position( ), planePtr->unitNormal( ), headPosition );

        if( ( tailDistance < 0.0f ) && ( headDistance < 0.0f ) )
        {
            tailNeighborFeature = headNeighborFeature = planePtr->neighborFeature( );
            return false;
        }
        else if( tailDistance < 0.0f )
        {
            float const intersectionParameter = tailDistance / ( tailDistance - headDistance );
            if( intersectionParameter > tailIntersectionParameter )
            {
                tailIntersectionParameter = intersectionParameter;
                tailNeighborFeature = planePtr->neighborFeature( );

                if( tailIntersectionParameter > headIntersectionParameter )
                {
                    return false;
                }
            }
        }
        else if( headDistance < 0.0f )
        {
            float const intersectionParameter = tailDistance / ( tailDistance - headDistance );
            if( intersectionParameter < headIntersectionParameter )
            {
                headIntersectionParameter = intersectionParameter;
                headNeighborFeature = planePtr->neighborFeature( );

                if( tailIntersectionParameter > headIntersectionParameter )
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool shapeVertexYInVertexXRegion( 
    Shape const & shapeX, const int vertexIndexX, 
    Shape const & shapeY, const int vertexIndexY,
    FeatureClass & updateFeature )
{
    XMVECTOR const vertexY = shapeY.transformedVertexPosition( vertexIndexY );
    
    XMVECTOR const vertexXPlanePosition = shapeX.transformedVertexPosition( vertexIndexX );

    const int edgeIndexX = vertexIndexX;	
    XMVECTOR vertexXPlaneUnitNormal = -shapeX.transformedEdgeDirectionUnit( edgeIndexX );

    if( shapeSignedDistanceFromPlaneToPoint( vertexXPlanePosition, vertexXPlaneUnitNormal, vertexY ) < 0.0f )
    {
        updateFeature = FeatureClass( EdgeFeature, edgeIndexX );
        return false;
    }

    const int previousEdgeIndexX = ( ( vertexIndexX == 0 ) ? ( shapeX.numberOfVertices( ) - 1 ) : ( vertexIndexX - 1 ) );
    vertexXPlaneUnitNormal = shapeX.transformedEdgeDirectionUnit( previousEdgeIndexX );

    if( shapeSignedDistanceFromPlaneToPoint( vertexXPlanePosition, vertexXPlaneUnitNormal, vertexY ) < 0.0f )
    {
        updateFeature = FeatureClass( EdgeFeature, previousEdgeIndexX );
        return false;
    }

    return true;
}

VClipEnum vertexVertexState( 
    Shape const & shapeA, FeatureClass & shapeAFeature,
    Shape const & shapeB, FeatureClass & shapeBFeature )
{
    assert( shapeAFeature.type( ) == VertexFeature );
    assert( shapeAFeature.index( ) < shapeA.numberOfVertices( ) );
    assert( shapeBFeature.type( ) == VertexFeature );
    assert( shapeBFeature.index( ) < shapeB.numberOfVertices( ) );

    if( !shapeVertexYInVertexXRegion( shapeA, shapeAFeature.index( ), shapeB, shapeBFeature.index( ), shapeAFeature ) )
    {
        return VClipContinue;
    }

    if( !shapeVertexYInVertexXRegion( shapeB, shapeBFeature.index( ), shapeA, shapeAFeature.index( ), shapeBFeature ) )
    {
        return VClipContinue;
    }

    return VClipDone;
}

VClipEnum handleLocalMin( 
    Shape const & shapeA, FeatureClass const & shapeAFeature, // Vertex
    Shape const & shapeB, FeatureClass & shapeBFeature ) // Edge
{
    assert( shapeAFeature.type( ) == VertexFeature );
    assert( shapeAFeature.index( ) < shapeA.numberOfVertices( ) );
    assert( shapeBFeature.type( ) == EdgeFeature );
    assert( shapeBFeature.index( ) < shapeB.numberOfEdges( ) );

    float dMax = -FLT_MAX;
    FeatureClass updateFeature;

    int numberOfEdgesB = shapeB.numberOfEdges( );
    for( int edgeBIndex = 0; edgeBIndex != numberOfEdgesB; ++edgeBIndex )
    {
        float const signedDistanceToPlane = shapeSignedDistanceFromPlaneToPoint(
            shapeB.transformedVertexPosition( edgeBIndex ), // XMVECTOR const & planePosition,
            shapeB.transformedEdgeNormalDirectionUnit( edgeBIndex ), // XMVECTOR const & planeUnitNormal,
            shapeA.transformedVertexPosition( shapeAFeature.index( ) ) );

        if( signedDistanceToPlane > dMax )
        {
            dMax = signedDistanceToPlane;
            updateFeature = FeatureClass( EdgeFeature, edgeBIndex );
        }
    }

    if( dMax < 0.0f )
    {
        return VClipPenetration;
    }

    shapeBFeature = updateFeature;

    return VClipContinue;
}

VClipEnum vertexEdgeState( 
    Shape const & shapeA, FeatureClass & shapeAFeature, // Vertex
    Shape const & shapeB, FeatureClass & shapeBFeature ) // Edge
{
    assert( shapeAFeature.type( ) == VertexFeature );
    assert( shapeAFeature.index( ) < shapeA.numberOfVertices( ) );
    assert( shapeBFeature.type( ) == EdgeFeature );
    assert( shapeBFeature.index( ) < shapeB.numberOfEdges( ) );

    const int edgeBIndex = shapeBFeature.index( );
    const int edgeBTailVertexIndex = edgeBIndex;
    const int edgeBHeadVertexIndex = ( ( edgeBIndex == ( shapeB.numberOfEdges( ) - 1 ) ) ? 0 : ( edgeBIndex + 1 ) );	

    XMVECTOR const edgeBDirection = shapeB.transformedEdgeDirection( edgeBIndex );
    XMVECTOR const edgeBUnitDirection = shapeB.transformedEdgeDirectionUnit( edgeBIndex );

    XMVECTOR const edgeBTailPlanePosition = shapeB.transformedVertexPosition( edgeBTailVertexIndex );
    XMVECTOR const edgeBTailPlaneUnitNormal = edgeBUnitDirection;

    XMVECTOR const edgeBHeadPlanePosition = shapeB.transformedVertexPosition( edgeBHeadVertexIndex );
    XMVECTOR const edgeBHeadPlaneUnitNormal = -edgeBUnitDirection;

    const int vertexIndexA = shapeAFeature.index( );
    XMVECTOR const vertexA = shapeA.transformedVertexPosition( vertexIndexA );

    if( shapeSignedDistanceFromPlaneToPoint( edgeBTailPlanePosition, edgeBTailPlaneUnitNormal, vertexA ) < 0.0f )
    {
        shapeBFeature = FeatureClass( VertexFeature, edgeBTailVertexIndex );
        return VClipContinue;
    }

    if( shapeSignedDistanceFromPlaneToPoint( edgeBHeadPlanePosition, edgeBHeadPlaneUnitNormal, vertexA ) < 0.0f )
    {
        shapeBFeature = FeatureClass( VertexFeature, edgeBHeadVertexIndex );
        return VClipContinue;
    }

    float const vertexASignedDistanceToEdgeB = shapeSignedDistanceFromPlaneToPoint( 
        shapeB.transformedVertexPosition( edgeBIndex ), // XMVECTOR const & planePosition,
        shapeB.transformedEdgeNormalDirectionUnit( edgeBIndex ), // XMVECTOR const & planeUnitNormal,
        shapeA.transformedVertexPosition( shapeAFeature.index( ) ) );

    if( vertexASignedDistanceToEdgeB < 0.0f )
    {
        return handleLocalMin( 
            shapeA, shapeAFeature, // Vertex
            shapeB, shapeBFeature ); // Edge
    }
    
    const int previousVertexAIndex = ( ( vertexIndexA == 0 ) ? ( shapeA.numberOfEdges( ) - 1 ) : ( vertexIndexA - 1 ) );
    float const previousVertexASignedDistanceToEdgeB = shapeSignedDistanceFromPlaneToPoint( 
        shapeB.transformedVertexPosition( edgeBIndex ), // XMVECTOR const & planePosition,
        shapeB.transformedEdgeNormalDirectionUnit( edgeBIndex ), // XMVECTOR const & planeUnitNormal,
        shapeA.transformedVertexPosition( previousVertexAIndex ) );

    if( fabsf( previousVertexASignedDistanceToEdgeB ) < fabsf( vertexASignedDistanceToEdgeB ) )
    {
        shapeAFeature = FeatureClass( EdgeFeature, previousVertexAIndex );
        return VClipContinue;
    }

    const int nextVertexAIndex = ( ( vertexIndexA == ( shapeA.numberOfVertices( ) - 1 ) ) ? 0 : ( vertexIndexA + 1 ) );
    float const nextVertexASignedDistanceToEdgeB = shapeSignedDistanceFromPlaneToPoint( 
        shapeB.transformedVertexPosition( edgeBIndex ), // XMVECTOR const & planePosition,
        shapeB.transformedEdgeNormalDirectionUnit( edgeBIndex ), // XMVECTOR const & planeUnitNormal,
        shapeA.transformedVertexPosition( nextVertexAIndex ) );

    if( fabsf( nextVertexASignedDistanceToEdgeB ) < fabsf( vertexASignedDistanceToEdgeB ) )
    {
        shapeAFeature = FeatureClass( EdgeFeature, nextVertexAIndex );
        return VClipContinue;
    }


    const int previousEdgeAIndex = ( ( vertexIndexA == 0 ) ? ( shapeA.numberOfVertices( ) - 1 ) : ( vertexIndexA - 1 ) );
    const int nextEdgeAIndex = vertexIndexA;	
    
    XMVECTOR const previousEdgeAPlaneUnitNormal = shapeA.transformedEdgeDirectionUnit( previousEdgeAIndex );
    XMVECTOR const nextEdgeAPlaneUnitNormal = -shapeA.transformedEdgeDirectionUnit( nextEdgeAIndex );	
    
    const int numberOfPlanes = 2;
    PlaneClass const planeArray[ numberOfPlanes ] =
    {
        PlaneClass( 
            vertexA, // XMVECTOR const & position, 
            previousEdgeAPlaneUnitNormal, // XMVECTOR const & unitNormal,
            FeatureClass( EdgeFeature, previousEdgeAIndex ) ), // FeatureClass const & neighborFeature )	
        PlaneClass( 
            vertexA, // XMVECTOR const & position, 
            nextEdgeAPlaneUnitNormal, // XMVECTOR const & unitNormal,
            FeatureClass( EdgeFeature, nextEdgeAIndex ) ), // FeatureClass const & neighborFeature )
    };
    //PlaneVectorType vertexAPlaneVector;
    //vertexAPlaneVector.reserve( 2 );
    //vertexAPlaneVector.push_back( PlaneClass( 
    //	vertexA, // XMVECTOR const & position, 
    //	previousEdgeAPlaneUnitNormal, // XMVECTOR const & unitNormal,
    //	FeatureClass( EdgeFeature, previousEdgeAIndex ) ) ); // FeatureClass const & neighborFeature )	
    //vertexAPlaneVector.push_back( PlaneClass( 
    //	vertexA, // XMVECTOR const & position, 
    //	nextEdgeAPlaneUnitNormal, // XMVECTOR const & unitNormal,
    //	FeatureClass( EdgeFeature, nextEdgeAIndex ) ) ); // FeatureClass const & neighborFeature )

    float tailIntersectionParameter = 0.0f;
    float headIntersectionParameter = 1.0f;
    FeatureClass tailNeighborFeature;
    FeatureClass headNeighborFeature;
    clipEdge( 
        edgeBTailPlanePosition, // XMVECTOR const & tailPosition,
        edgeBHeadPlanePosition, // XMVECTOR const & headPosition,
        planeArray,
        numberOfPlanes,
        //vertexAPlaneVector,	
        tailIntersectionParameter,
        headIntersectionParameter,
        tailNeighborFeature,
        headNeighborFeature );
    


    if( ( headNeighborFeature.type( ) != numberOfFeatures ) && ( tailNeighborFeature.type( ) != numberOfFeatures ) &&
        ( tailNeighborFeature == headNeighborFeature ) )
    {
        shapeAFeature = tailNeighborFeature;
        return VClipContinue;
    }
    else
    {
        if( tailNeighborFeature.type( ) != numberOfFeatures )
        {
            XMVECTOR const tailIntersectionPosition = edgeBTailPlanePosition + ( edgeBDirection * tailIntersectionParameter );
            XMVECTOR const vertexAToTailIntersection = tailIntersectionPosition - vertexA;

            if( XMVectorGetX( XMVector4Dot( edgeBDirection, vertexAToTailIntersection ) ) > 0.0f )
            {
                shapeAFeature = tailNeighborFeature;
                return VClipContinue;
            }
        }
        
        if( headNeighborFeature.type( ) != numberOfFeatures )
        {
            XMVECTOR const headIntersectionPosition = edgeBTailPlanePosition + ( edgeBDirection * headIntersectionParameter );
            XMVECTOR const vertexAToHeadIntersection = headIntersectionPosition - vertexA;

            if( XMVectorGetX( XMVector4Dot( edgeBDirection, vertexAToHeadIntersection ) ) < 0.0f )
            {
                shapeAFeature = headNeighborFeature;
                return VClipContinue;
            }
        }
    }

    return VClipDone;
}

bool clipEdgeYAgainstVertexPlanesOfEdgeX(
    Shape const & shapeX, FeatureClass & shapeXFeature,
    Shape const & shapeY, FeatureClass const & shapeYFeature,
    XMVECTOR & tailIntersectionPosition,
    XMVECTOR & headIntersectionPosition )
{
    assert( shapeXFeature.type( ) == EdgeFeature );
    assert( shapeXFeature.index( ) < shapeX.numberOfEdges( ) );
    assert( shapeYFeature.type( ) == EdgeFeature );
    assert( shapeYFeature.index( ) < shapeY.numberOfEdges( ) );

    const int edgeXIndex = shapeXFeature.index( );
    const int edgeXTailVertexIndex = edgeXIndex;
    const int edgeXHeadVertexIndex = ( ( edgeXIndex == ( shapeX.numberOfEdges( ) - 1 ) ) ? 0 : ( edgeXIndex + 1 ) );	

    XMVECTOR const edgeXDirection = shapeX.transformedEdgeDirection( edgeXIndex );
    XMVECTOR const edgeXUnitDirection = shapeX.transformedEdgeDirectionUnit( edgeXIndex );

    XMVECTOR const edgeXTailPosition = shapeX.transformedVertexPosition( edgeXTailVertexIndex );
    XMVECTOR const edgeXTailPlaneUnitNormal = edgeXUnitDirection;

    XMVECTOR const edgeXHeadPosition = shapeX.transformedVertexPosition( edgeXHeadVertexIndex );
    XMVECTOR const edgeXHeadPlaneUnitNormal = -edgeXUnitDirection;
    
    const int edgeYIndex = shapeYFeature.index( );
    const int edgeYTailVertexIndex = edgeYIndex;
    const int edgeYHeadVertexIndex = ( ( edgeYIndex == ( shapeY.numberOfEdges( ) - 1 ) ) ? 0 : ( edgeYIndex + 1 ) );	

    XMVECTOR const edgeYDirection = shapeY.transformedEdgeDirection( edgeYIndex );
    XMVECTOR const edgeYUnitDirection = shapeY.transformedEdgeDirectionUnit( edgeYIndex );

    XMVECTOR const edgeYTailPosition = shapeY.transformedVertexPosition( edgeYTailVertexIndex );

    XMVECTOR const edgeYHeadPosition = shapeY.transformedVertexPosition( edgeYHeadVertexIndex );

    const int numberOfPlanes = 2;
    PlaneClass const planeArray[ numberOfPlanes ] =
    {
        PlaneClass( 
            edgeXTailPosition, // XMVECTOR const & position, 
            edgeXTailPlaneUnitNormal, // XMVECTOR const & unitNormal,
            FeatureClass( VertexFeature, edgeXTailVertexIndex ) ), // FeatureClass const & neighborFeature )	
        PlaneClass( 
            edgeXHeadPosition, // XMVECTOR const & position, 
            edgeXHeadPlaneUnitNormal, // XMVECTOR const & unitNormal,
            FeatureClass( VertexFeature, edgeXHeadVertexIndex ) ), // FeatureClass const & neighborFeature )
    };
    //PlaneVectorType edgeXPlaneVector;
    //edgeXPlaneVector.reserve( 2 );
    //edgeXPlaneVector.push_back( PlaneClass( 
    //	edgeXTailPosition, // XMVECTOR const & position, 
    //	edgeXTailPlaneUnitNormal, // XMVECTOR const & unitNormal,
    //	FeatureClass( VertexFeature, edgeXTailVertexIndex ) ) ); // FeatureClass const & neighborFeature )	
    //edgeXPlaneVector.push_back( PlaneClass( 
    //	edgeXHeadPosition, // XMVECTOR const & position, 
    //	edgeXHeadPlaneUnitNormal, // XMVECTOR const & unitNormal,
    //	FeatureClass( VertexFeature, edgeXHeadVertexIndex ) ) ); // FeatureClass const & neighborFeature )

    float tailIntersectionParameter = 0.0f;
    float headIntersectionParameter = 1.0f;
    FeatureClass tailNeighborFeature;
    FeatureClass headNeighborFeature;
    clipEdge( 
        edgeYTailPosition, // XMVECTOR const & tailPosition,	
        edgeYHeadPosition, // XMVECTOR const & headPosition,
        planeArray,
        numberOfPlanes,
        //edgeXPlaneVector,	
        tailIntersectionParameter,
        headIntersectionParameter,
        tailNeighborFeature,
        headNeighborFeature );

    if( ( headNeighborFeature.type( ) != numberOfFeatures ) && ( tailNeighborFeature.type( ) != numberOfFeatures ) &&
        ( tailNeighborFeature == headNeighborFeature ) )
    {
        shapeXFeature = tailNeighborFeature;
        return true;
    }
    
    tailIntersectionPosition = edgeYTailPosition + ( edgeYDirection * tailIntersectionParameter );
    headIntersectionPosition = edgeYTailPosition + ( edgeYDirection * headIntersectionParameter );

    return false;
}

VClipEnum edgeEdgeState( 
    Shape const & shapeA, FeatureClass & shapeAFeature,
    Shape const & shapeB, FeatureClass & shapeBFeature )
{
    assert( shapeAFeature.type( ) == EdgeFeature );
    assert( shapeAFeature.index( ) < shapeA.numberOfEdges( ) );
    assert( shapeBFeature.type( ) == EdgeFeature );
    assert( shapeBFeature.index( ) < shapeB.numberOfEdges( ) );

    const int edgeAIndex = shapeAFeature.index( );
    const int edgeATailVertexIndex = edgeAIndex;
    const int edgeAHeadVertexIndex = ( ( edgeAIndex == ( shapeA.numberOfEdges( ) - 1 ) ) ? 0 : ( edgeAIndex + 1 ) );	

    const int edgeBIndex = shapeBFeature.index( );
    const int edgeBTailVertexIndex = edgeBIndex;
    const int edgeBHeadVertexIndex = ( ( edgeBIndex == ( shapeB.numberOfEdges( ) - 1 ) ) ? 0 : ( edgeBIndex + 1 ) );	

    XMVECTOR edgeBTailIntersectionPosition;
    XMVECTOR edgeBHeadIntersectionPosition;
    if( clipEdgeYAgainstVertexPlanesOfEdgeX( 
            shapeA, shapeAFeature,	
            shapeB, shapeBFeature,
            edgeBTailIntersectionPosition, edgeBHeadIntersectionPosition ) )
    {
        return VClipContinue;
    }

    float const edgeBTailSignedDistanceToEdgeA = shapeSignedDistanceFromPlaneToPoint( 
        shapeA.transformedVertexPosition( edgeAIndex ), // XMVECTOR const & planePosition,
        shapeA.transformedEdgeNormalDirectionUnit( edgeAIndex ), // XMVECTOR const & planeUnitNormal,
        edgeBTailIntersectionPosition );

    float const edgeBHeadSignedDistanceToEdgeA = shapeSignedDistanceFromPlaneToPoint( 
        shapeA.transformedVertexPosition( edgeAIndex ), // XMVECTOR const & planePosition,
        shapeA.transformedEdgeNormalDirectionUnit( edgeAIndex ), // XMVECTOR const & planeUnitNormal,
        edgeBHeadIntersectionPosition );

    if( ( edgeBTailSignedDistanceToEdgeA * edgeBHeadSignedDistanceToEdgeA ) <= 0.0f )
    {
        return VClipPenetration;
    }

    XMVECTOR edgeATailIntersectionPosition;
    XMVECTOR edgeAHeadIntersectionPosition;
    if( clipEdgeYAgainstVertexPlanesOfEdgeX( 
            shapeB, shapeBFeature, 
            shapeA, shapeAFeature,
            edgeATailIntersectionPosition,
            edgeAHeadIntersectionPosition ) )
    {
        return VClipContinue;
    }

    float const edgeATailSignedDistanceToEdgeB = shapeSignedDistanceFromPlaneToPoint( 
        shapeB.transformedVertexPosition( edgeBIndex ), // XMVECTOR const & planePosition,
        shapeB.transformedEdgeNormalDirectionUnit( edgeBIndex ), // XMVECTOR const & planeUnitNormal,
        edgeATailIntersectionPosition );

    float const edgeAHeadSignedDistanceToEdgeB = shapeSignedDistanceFromPlaneToPoint( 
        shapeB.transformedVertexPosition( edgeBIndex ), // XMVECTOR const & planePosition,
        shapeB.transformedEdgeNormalDirectionUnit( edgeBIndex ), // XMVECTOR const & planeUnitNormal,
        edgeAHeadIntersectionPosition );

    if( ( edgeATailSignedDistanceToEdgeB * edgeAHeadSignedDistanceToEdgeB ) <= 0.0f )
    {
        return VClipPenetration;
    }
    
    float AToBDistance;
    FeatureClass AFeature;
    if( fabsf( edgeATailSignedDistanceToEdgeB ) < fabsf( edgeAHeadSignedDistanceToEdgeB ) )
    {
        AToBDistance = fabsf( edgeATailSignedDistanceToEdgeB );
        AFeature = FeatureClass( VertexFeature, edgeATailVertexIndex );
    }
    else
    {
        AToBDistance = fabsf( edgeAHeadSignedDistanceToEdgeB );
        AFeature = FeatureClass( VertexFeature, edgeAHeadVertexIndex );
    }	
    
    float BToADistance;
    FeatureClass BFeature;
    if( fabsf( edgeBTailSignedDistanceToEdgeA ) < fabsf( edgeBHeadSignedDistanceToEdgeA ) )
    {
        BToADistance = fabsf( edgeBTailSignedDistanceToEdgeA );
        BFeature = FeatureClass( VertexFeature, edgeBTailVertexIndex );
    }
    else
    {
        BToADistance = fabsf( edgeBHeadSignedDistanceToEdgeA );
        BFeature = FeatureClass( VertexFeature, edgeBHeadVertexIndex );
    }

    //if( fabsf( AToBDistance - BToADistance ) < 0.1f )
    //{
    //	// Parallel
    //	return VClipDone;
    //}

    if( AToBDistance < BToADistance )
    {
        shapeAFeature = AFeature;
        return VClipContinue;
    }
    else
    {
        shapeBFeature = BFeature;
        return VClipContinue;
    }
}

VClipEnum shapeSingleVClipIteration( 
    Shape const & shapeA, FeatureClass & shapeAFeature,
    Shape const & shapeB, FeatureClass & shapeBFeature )
{

    switch( shapeAFeature.type( ) )
    {
    case VertexFeature :
        {
            switch( shapeBFeature.type( ) )
            {
            case VertexFeature : 
                {
                    return vertexVertexState( shapeA, shapeAFeature, shapeB, shapeBFeature );
                }
                break;

            case EdgeFeature :
            case FaceFeature :
            case numberOfFeatures :
            default :
                {
                    return vertexEdgeState( 
                        shapeA, shapeAFeature, // Vertex
                        shapeB, shapeBFeature ); // Edge
                }
                break;
            }
        }
        break;

    case EdgeFeature :
    case FaceFeature :
    case numberOfFeatures :
    default :
        {
            switch( shapeBFeature.type( ) )
            {
            case VertexFeature :
                {
                    return vertexEdgeState( 
                        shapeB, shapeBFeature, // Vertex
                        shapeA, shapeAFeature ); // Edge
                }
                break;

            case EdgeFeature :
            case FaceFeature :
            case numberOfFeatures :
            default :
                {
                    // Edge-Edge state.
                    return edgeEdgeState( shapeA, shapeAFeature, shapeB, shapeBFeature );
                }
                break;
            }
        }
        break;
    }
}

bool shapeVClip( 
    Shape const & shapeA, FeatureClass & shapeAFeature,
    Shape const & shapeB, FeatureClass & shapeBFeature )
{
    VClipEnum state = VClipDone;

    //{
    //	char string[ 128 ];
    //	sprintf_s( string, sizeof( string ), "\nVCLIP START-------\n" );
    //	DXUTOutputDebugStringA( string );
    //}

    int loopCount = 0;
    //float const maxLoopCount = 10;
    
    do
    {
        //if( loopCount > maxLoopCount )		
        //{
        //	XMVECTOR const shapeAPosition = shapeA.dynamics( ).position( );
        //	XMVECTOR const shapeBPosition = shapeB.dynamics( ).position( );

        //	char string[ 128 ];
        //	sprintf_s( string, sizeof( string ), 
        //		"loop %d\n"\
        //		"Shape A ( %.5f, %.5f) feature %s index %d\n"\
        //		"Shape B ( %.5f, %.5f) feature %s index %d\n",
        //		loopCount, 
        //		shapeAPosition.x, shapeAPosition.y, featureTypeName( shapeAFeature.type( ) ), shapeAFeature.index( ),
        //		shapeBPosition.x, shapeBPosition.y, featureTypeName( shapeBFeature.type( ) ), shapeBFeature.index( ) );
        //	DXUTOutputDebugStringA( string );
        //}
        
        state = shapeSingleVClipIteration( shapeA, shapeAFeature, shapeB, shapeBFeature );

        //switch( shapeAFeature.type( ) )
        //{
        //case VertexFeature :
        //	{
        //		switch( shapeBFeature.type( ) )
        //		{
        //		case VertexFeature : 
        //			{
        //				state = vertexVertexState( shapeA, shapeAFeature, shapeB, shapeBFeature );
        //			}
        //			break;

        //		case EdgeFeature :
        //		case FaceFeature :
        //		case numberOfFeatures :
        //		default :
        //			{
        //				state = vertexEdgeState( 
        //					shapeA, shapeAFeature, // Vertex
        //					shapeB, shapeBFeature ); // Edge
        //			}
        //			break;
        //		}
        //	}
        //	break;

        //case EdgeFeature :
        //case FaceFeature :
        //case numberOfFeatures :
        //default :
        //	{
        //		switch( shapeBFeature.type( ) )
        //		{
        //		case VertexFeature :
        //			{
        //				state = vertexEdgeState( 
        //					shapeB, shapeBFeature, // Vertex
        //					shapeA, shapeAFeature ); // Edge
        //			}
        //			break;

        //		case EdgeFeature :
        //		case FaceFeature :
        //		case numberOfFeatures :
        //		default :
        //			{
        //				// Edge-Edge state.
        //				state = edgeEdgeState( shapeA, shapeAFeature, shapeB, shapeBFeature );
        //			}
        //			break;
        //		}
        //	}
        //	break;
        //}

        ++loopCount;
    } while( state == VClipContinue );
    
    //{
    //	char string[ 128 ];
    //	sprintf_s( string, sizeof( string ), "\nVCLIP END-------%s\n", ( state == VClipPenetration ) ? "PENETRATION" : "DONE" );
    //	DXUTOutputDebugStringA( string );
    //}
    

    return ( state == VClipPenetration );
}

//float closestDistanceAlongLine(
//	XMVECTOR const & lineStartPosition,
//	XMVECTOR const & lineUnitDirection,
//	float const lineLength,
//	XMVECTOR const & position )
//{
//	float const positionDotLineDirection = D3DXVec4Dot( &position, &lineUnitDirection );
//	float const lineStartPositionDotLineDirection = D3DXVec4Dot( &lineStartPosition, &lineUnitDirection );
//
//	return clamp( positionDotLineDirection - lineStartPositionDotLineDirection, 0.0f, lineLength );
//}	

}
