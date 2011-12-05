// CHB 2007.08.13
//	The animated vertex formats are different for 1.x shaders
//	and 2.0 shaders.  Thus we can't mix 1.x and 2.0 shaders
//	for animated models.  For FXTGT_SM_20_LOW, where we want
//	to use specific 1.x shaders on 2.0 hardware for performance,
//	it's necessary to compile the animated 1.x shader code
//	specially, rather than just use the 1.x shaders directly.

#define DXC_SHADERLIMITS_COMPILE_11_AS_20
#include "../Hellgate/Dx9/1x/ActorOutdoor1x.fx1"
