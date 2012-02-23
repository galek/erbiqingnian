
#ifndef __d3dUtilityH__
#define __d3dUtilityH__

/// ´¿²âÊÔ£¡£¡£¡£¡

#include <d3dx9.h>
#include <string>

class d3d
{
public:
	static bool InitD3D(
		HWND hwnd,
		int width, int height,     
		bool windowed,             
		IDirect3DDevice9** device);


	static bool Display(IDirect3DDevice9* Device);
};

template<class T> void Release(T t)
{
	if( t )
	{
		t->Release();
		t = 0;
	}
}

template<class T> void Delete(T t)
{
	if( t )
	{
		delete t;
		t = 0;
	}
}

#endif // __d3dUtilityH__