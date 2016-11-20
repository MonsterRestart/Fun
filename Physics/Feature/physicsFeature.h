// Andrew Davies

#if !defined( PHYSICS_FEATURE_H )
#define PHYSICS_FEATURE_H

#include "Physics/Feature/physicsFeatureFwd.h"

namespace Physics
{
	
enum FeatureTypeEnum
{
	VertexFeature,
	EdgeFeature,
	FaceFeature,

	numberOfFeatures
};

class FeatureClass
{
private:
	FeatureTypeEnum m_type;
	int m_index;

public:
	FeatureClass( );

	FeatureClass( FeatureTypeEnum type, int index );

	FeatureTypeEnum type( ) const
	{
		return m_type;
	}

	int index( ) const
	{
		return m_index;
	}
};

inline bool operator==( FeatureClass const & lhs, FeatureClass const & rhs )
{
	return ( lhs.type( ) == rhs.type( ) ) && ( lhs.index( ) == rhs.index( ) );
}

char const * featureTypeName( FeatureTypeEnum featureType );

}

#endif