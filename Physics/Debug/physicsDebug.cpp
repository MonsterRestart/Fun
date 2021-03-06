// Andrew Davies

#include "pch.h"
#include "Physics/Debug/physicsDebug.h"

namespace Physics
{

void DebugClass::create( EngineClass const & engine )
{
	assert( m_engineDeque.empty( ) );
	m_engineDeque.push_back( engine );
}

void DebugClass::destroy( )
{
	m_engineDeque.clear( );
}

void DebugClass::step( const EngineClass& engine, const float deltaTime )
{
	//// It's often worthwile to be able to step over the frame that has just encountered a problem
	//if( m_engineDeque.empty( ) == false )
	//{
	//	m_engineDeque.back().step( deltaTime );
	//}

	if( m_engineDeque.size( ) > 30 )
	{
		m_engineDeque.pop_front( );
	}

	m_engineDeque.push_back( engine );
}
	
void DebugClass::draw( /*IDirect3DDevice9 & d3dDevice,*/ EngineClass const & engine ) const
{
	//for( DebugEngineDequeType::const_iterator engineItr = m_engineDeque.begin( ); engineItr != m_engineDeque.end( ); ++engineItr )
	//{
	//	engineItr->draw( d3dDevice );
	//}

}

}
