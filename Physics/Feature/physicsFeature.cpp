// Andrew Davies

#include "pch.h"
#include "Physics/Feature/physicsFeature.h"
#include <math.h>

namespace Physics
{

// Default construct a feature in a NULL state
FeatureClass::FeatureClass( )
	: m_type( numberOfFeatures )
	, m_index( 0 )
{
}

FeatureClass::FeatureClass( const FeatureTypeEnum type, const int index )
	: m_type( type )
	, m_index( index )
{
}

char const * const g_FeatureNameArray[ numberOfFeatures + 1 ] =
{
	"Vertex",
	"Edge",
	"Face",
	"NumberOf or NULL",
};

char const * featureTypeName( FeatureTypeEnum const featureType )
{
	return g_FeatureNameArray[ featureType ];
}



}
