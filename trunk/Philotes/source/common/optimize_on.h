// CHB 2006.08.18 - Enable certain optimizations for this high-traffic
// function.
//	t = prefer fast code (vs. small)
//	y = omit frame pointer
#if (_MSC_VER >= 1400) && (!defined(_DEBUG))
#pragma optimize("ty", on)
#endif
