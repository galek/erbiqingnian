//
// Particle Shader 
//

// transformations
#include "_common.fx"
#include "StateBlocks.fxh"




#ifdef ENGINE_TARGET_DX10
#		include "Dx10\\_SoftParticles.fxh"
#		include "Dx9\\_Particles.fxh"
#	undef ENGINE_TARGET_DX10
#		include "Dx10\\_SoftParticles.fxh"
#	define ENGINE_TARGET_DX10
#		include "Dx9\\_Particles.fxh"
#else
#		include "Dx10\\_SoftParticles.fxh"
#		include "Dx9\\_Particles.fxh"
#endif