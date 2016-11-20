//// Andrew Davies
//
//#if !defined( PHYSICS_SHAPE_PAIR_H )
//#define PHYSICS_SHAPE_PAIR_H
//
//#include "Physics/Shape/Pair/physicsShapePairFwd.h"
//#include "Physics/Shape/physicsShapeFwd.h"
//#include "Physics/Feature/physicsFeature.h"
//
//namespace Physics
//{
//	
//class ShapePairClass
//{
//private:
//	ShapeClass * m_shapeAPtr;
//	FeatureClass m_shapeAFeature;
//	ShapeClass * m_shapeBPtr;
//	FeatureClass m_shapeBFeature;
//
//public:
//	ShapePairClass(		
//		ShapeClass & shapeA,
//		FeatureClass const & shapeAFeature,
//		ShapeClass & shapeB,
//		FeatureClass const & shapeBFeature );
//
//	void step( );
//
//	ShapeClass * shapeAPtr( ) const 
//	{
//		return m_shapeAPtr;
//	}
//
//	FeatureClass const & shapeAFeature( ) const
//	{
//		return m_shapeAFeature;
//	}
//
//	ShapeClass * shapeBPtr( ) const 
//	{
//		return m_shapeBPtr;
//	}
//
//	FeatureClass const & shapeBFeature( ) const 
//	{
//		return m_shapeBFeature;
//	}
//
//};
//
//}
//
//#endif