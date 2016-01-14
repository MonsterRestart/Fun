#if !defined( AWAKENS_BLOB_RESOURCE_H )
#define AWAKENS_BLOB_RESOURCE_H

// Andrew Davies

#pragma once

#include <d3d10.h>

namespace Awakens
{

struct BlobResource
{
    ID3DBlob* m_pBlob;

public:
    BlobResource();
    ~BlobResource();
};

}

#endif