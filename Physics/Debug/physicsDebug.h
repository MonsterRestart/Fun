// Andrew Davies

#if !defined( PHYSICS_DEBUG_H )
#define PHYSICS_DEBUG_H

#include "Physics/Debug/physicsDebugFwd.h"
#pragma warning (disable: 4530)
#include <deque>
#pragma warning (default: 4530)
#include "Physics/Engine/physicsEngine.h"
#include <DirectXMath.h>

namespace Physics
{

typedef std::deque< EngineClass > DebugEngineDequeType;

class DebugClass
{
private:
	DebugEngineDequeType m_engineDeque;

public:

	void create( EngineClass const & engine );
	void destroy( );

	void step( const EngineClass& engine, float deltaTime );
	void draw( /*IDirect3DDevice9 & d3dDevice,*/ EngineClass const & engine ) const;
	
	DebugEngineDequeType const & engineDeque( ) const
	{
		return m_engineDeque;
	}
};

}

#endif