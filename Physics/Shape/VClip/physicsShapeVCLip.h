// Andrew Davies

#if !defined( PHYSICS_SHAPE_VCLIP_H )
#define PHYSICS_SHAPE_VCLIP_H

#include "Physics/Shape/physicsShapeFwd.h"
#include "Physics/Feature/physicsFeatureFwd.h"
#pragma warning (disable: 4530)
#include <vector>
#pragma warning (default: 4530)
#include <DirectXMath.h>
#include "Physics/Engine/physicsEngineFwd.h"
#include "Physics/Dynamics/physicsDynamics.h"

namespace Physics
{

enum VClipEnum
{
	VClipDone,
	VClipContinue,
	VClipPenetration
};

VClipEnum shapeSingleVClipIteration( 
	ShapeClass const & shapeA, FeatureClass & shapeAFeature,
	ShapeClass const & shapeB, FeatureClass & shapeBFeature );

bool shapeVClip( 
	ShapeClass const & shapeA, FeatureClass & featureA, 
	ShapeClass const & shapeB, FeatureClass & featureB );

}

#endif