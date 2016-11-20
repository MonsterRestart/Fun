// Andrew Davies 2011

#if !defined( DAV_H )
#define DAV_H

#include <string>
#include <vector>
#include <DirectXMath.h>

namespace Dav
{

class VertexClass
{
private:
    DirectX::XMVECTOR m_position;

public:
    VertexClass( const DirectX::XMVECTOR& position );

    const DirectX::XMVECTOR& position() const
    {
        return m_position;
    }
};

typedef std::vector< VertexClass > VertexVectorType;

typedef std::vector< int > VertexIndexVectorType;

class FaceClass
{
private:
    VertexIndexVectorType m_vertexIndexVector;

public:
    FaceClass();

    FaceClass( const VertexIndexVectorType& vertexIndexVector );

    void addVertexIndex( const int vertexIndex );

    const VertexIndexVectorType& vertexIndexVector() const
    {
        return m_vertexIndexVector;
    }

};

typedef std::vector< FaceClass > FaceVectorType;

class ObjectClass
{
private:
    std::string m_name;
    bool m_infiniteMass;

    DirectX::XMMATRIX m_transformation;

    VertexVectorType m_vertexVector;
    FaceVectorType m_faceVector;

public:
    ObjectClass(
        const std::string& name,
        const bool infiniteMass,
        const DirectX::XMMATRIX& transformation,
        const VertexVectorType& vertexVector,
        const FaceVectorType& faceVector );

    ObjectClass(
        const std::string& name,
        const bool infiniteMass,
        const DirectX::XMMATRIX& transformation );

    const std::string& name() const
    {
        return m_name;
    }

    bool infiniteMass() const
    {
        return m_infiniteMass;
    }

    const DirectX::XMMATRIX& transformation() const
    {
        return m_transformation;
    }

    void addVertex( const VertexClass& vertex );
    
    const VertexVectorType& vertexVector() const
    {
        return m_vertexVector;
    }

    void addFace( const FaceClass& face );

    const FaceVectorType& faceVector() const
    {
        return m_faceVector;
    }

    //void draw( IDirect3DDevice9 & d3dDevice ) const;

};

typedef std::vector< ObjectClass > ObjectVectorType;

class SceneClass
{
private:
    ObjectVectorType m_objectVector;

public:
    SceneClass();

    SceneClass( const ObjectVectorType& objectVector );

    void addObject( const ObjectClass& object );

    const ObjectVectorType& objectVector() const
    {
        return m_objectVector;
    }

};

//class MeshClass
//{
//private:
//    int m_numVertices;
//    IDirect3DVertexBuffer9* m_vertexBuffer;
//    int m_numTriangles;
//    IDirect3DIndexBuffer9* m_indexBuffer;
//
//public:
//    MeshClass();
//
//    void create(
//        const ObjectClass& object,
//        IDirect3DDevice9& d3dDevice );
//
//    void destroy();
//
//    void draw( IDirect3DDevice9& d3dDevice ) const;
//    
//    int numVertices() const
//    {
//        return m_numVertices;
//    }
//
//    const IDirect3DVertexBuffer9* vertexBuffer() const
//    {
//        return m_vertexBuffer;
//    }
//
//    int numTriangles() const
//    {
//        return m_numTriangles;
//    }
//
//    const IDirect3DIndexBuffer9* indexBuffer() const
//    {
//        return m_indexBuffer;
//    }
//};



}

#endif
