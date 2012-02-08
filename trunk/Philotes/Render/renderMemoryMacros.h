

#ifndef RENDERER_MEMORY_MACROS_H
#define RENDERER_MEMORY_MACROS_H

// PT: if you don't like those macros, don't use them. But don't touch/remove them!
#define	DELETESINGLE(x)	if(x){ delete x; x = NULL;		}
#define	DELETEARRAY(x)	if(x){ delete []x; x = NULL;	}
#define	SAFE_RELEASE(x)	if(x){ x->release(); x = NULL;	}

#endif
