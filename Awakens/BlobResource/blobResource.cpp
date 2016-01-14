// Andrew Davies

#include "stdafx.h"
#include "blobResource.h"

using namespace std;

namespace Awakens
{

BlobResource::BlobResource()
    : m_pBlob( nullptr )
{
}

BlobResource::~BlobResource()
{
    if( m_pBlob )
    {
        m_pBlob->Release();
    }
}

}


