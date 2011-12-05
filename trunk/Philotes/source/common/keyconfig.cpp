//----------------------------------------------------------------------------
// keyconfig.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "keyconfig.h"
#include "dictionary.h"
#include "globalindex.h"
#include "c_input.h"

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct KEYNAME
{
	GLOBAL_STRING eString;
	int nKeyCode;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static KEYNAME keytable[] =
{

	//global string				virtual key
	{ GS_KEY_MOUSE0,			VK_LBUTTON },            	
	{ GS_KEY_MOUSE1,			VK_RBUTTON },            	
	{ GS_KEY_MOUSE2,			VK_MBUTTON },            	
	{ GS_KEY_MOUSE3,			VK_XBUTTON1 },           	
	{ GS_KEY_MOUSE4,			VK_XBUTTON2 },           	
	{ GS_KEY_BACKSPACE,			VK_BACK },               		
	{ GS_KEY_TAB,				VK_TAB },                
	{ GS_KEY_CLEAR,				VK_CLEAR },              
	{ GS_KEY_ENTER,				VK_RETURN },             
	{ GS_KEY_SHIFT,				VK_SHIFT },              
	{ GS_KEY_CTRL,				VK_CONTROL },            
	{ GS_KEY_ALT,				VK_MENU },               
	{ GS_KEY_PAUSE,				VK_PAUSE },              
	{ GS_KEY_CAPSLOCK,			VK_CAPITAL },            		
	{ GS_KEY_KANA,				VK_KANA },               
	{ GS_KEY_HANGUEL,			VK_HANGUL },             	
	{ GS_KEY_HANGUL,			VK_HANGUL },             	
	{ GS_KEY_JUNJA,				VK_JUNJA },              
	{ GS_KEY_FINAL,				VK_FINAL },              
	{ GS_KEY_HANJA,				VK_HANJA },              
	{ GS_KEY_KANJI,				VK_KANJI },              
	{ GS_KEY_ESC,				VK_ESCAPE },             
	{ GS_KEY_CONVERT,			VK_CONVERT },            	
	{ GS_KEY_NONCONVERT,		VK_NONCONVERT },         			
	{ GS_KEY_ACCEPT,			VK_ACCEPT },             	
	{ GS_KEY_MODECHANGE,		VK_MODECHANGE },         			
	{ GS_KEY_SPACE,				VK_SPACE },              
	{ GS_KEY_PAGE_UP,			VK_PRIOR },              	
	{ GS_KEY_PAGE_DOWN,			VK_NEXT },               		
	{ GS_KEY_END,				VK_END },                
	{ GS_KEY_HOME,				VK_HOME },               
	{ GS_KEY_LEFT_ARROW,		VK_LEFT },               			
	{ GS_KEY_UP_ARROW,			VK_UP },                 		
	{ GS_KEY_RIGHT_ARROW,		VK_RIGHT },              			
	{ GS_KEY_DOWN_ARROW,		VK_DOWN },               			
	{ GS_KEY_SELECT,			VK_SELECT },             	
	{ GS_KEY_PRINT,				VK_PRINT },              
	{ GS_KEY_EXECUTE,			VK_EXECUTE },            	
	{ GS_KEY_PRINT_SCREEN,		VK_SNAPSHOT },           				
	{ GS_KEY_INS,				VK_INSERT },             
	{ GS_KEY_DEL,				VK_DELETE },             
	{ GS_KEY_HELP,				VK_HELP },               
	{ GS_KEY_LEFT_WIN,			VK_LWIN },               		
	{ GS_KEY_RIGHT_WIN,			VK_RWIN },               		
	{ GS_KEY_APPLICATION,		VK_APPS },               			
	{ GS_KEY_SLEEP,				VK_SLEEP },              
	{ GS_KEY_NUMPAD_0,			VK_NUMPAD0 },            		
	{ GS_KEY_NUMPAD_1,			VK_NUMPAD1 },            		
	{ GS_KEY_NUMPAD_2,			VK_NUMPAD2 },            		
	{ GS_KEY_NUMPAD_3,			VK_NUMPAD3 },            		
	{ GS_KEY_NUMPAD_4,			VK_NUMPAD4 },            		
	{ GS_KEY_NUMPAD_5,			VK_NUMPAD5 },            		
	{ GS_KEY_NUMPAD_6,			VK_NUMPAD6 },            		
	{ GS_KEY_NUMPAD_7,			VK_NUMPAD7 },            		
	{ GS_KEY_NUMPAD_8,			VK_NUMPAD8 },            		
	{ GS_KEY_NUMPAD_9,			VK_NUMPAD9 },            		
	{ GS_KEY_MULTIPLY,			VK_MULTIPLY },           		
	{ GS_KEY_ADD,				VK_ADD },                
	{ GS_KEY_SEPARATOR,			VK_SEPARATOR },          		
	{ GS_KEY_SUBTRACT,			VK_SUBTRACT },           		
	{ GS_KEY_DECIMAL,			VK_DECIMAL },            	
	{ GS_KEY_DIVIDE,			VK_DIVIDE },             	
	{ GS_KEY_F1,				VK_F1 },                 
	{ GS_KEY_F2,					VK_F2 },                 
	{ GS_KEY_F3,					VK_F3 },                 
	{ GS_KEY_F4,					VK_F4 },                 
	{ GS_KEY_F5,					VK_F5 },                 
	{ GS_KEY_F6,					VK_F6 },                 
	{ GS_KEY_F7,					VK_F7 },                 
	{ GS_KEY_F8,					VK_F8 },                 
	{ GS_KEY_F9,					VK_F9 },                 
	{ GS_KEY_F10,					VK_F10 },                
	{ GS_KEY_F11,					VK_F11 },                
	{ GS_KEY_F12,					VK_F12 },                
	{ GS_KEY_F13,					VK_F13 },                
	{ GS_KEY_F14,					VK_F14 },                
	{ GS_KEY_F15,					VK_F15 },                
	{ GS_KEY_F16,					VK_F16 },                
	{ GS_KEY_F17,					VK_F17 },                
	{ GS_KEY_F18,					VK_F18 },                
	{ GS_KEY_F19,					VK_F19 },                
	{ GS_KEY_F20,					VK_F20 },                
	{ GS_KEY_F21,					VK_F21 },                
	{ GS_KEY_F22,					VK_F22 },                
	{ GS_KEY_F23,					VK_F23 },                
	{ GS_KEY_F24,					VK_F24 },                
	{ GS_KEY_NUMLOCK,				VK_NUMLOCK },            	
	{ GS_KEY_SCROLL,				VK_SCROLL },             	
	{ GS_KEY_LEFT_SHIFT,			VK_LSHIFT },             			
	{ GS_KEY_RIGHT_SHIFT,			VK_RSHIFT },             			
	{ GS_KEY_LEFT_CTRL,			VK_LCONTROL },           		
	{ GS_KEY_RIGHT_CTRL,			VK_RCONTROL },           			
	{ GS_KEY_LEFT_ALT,			VK_LMENU },              		
	{ GS_KEY_RIGHT_ALT,			VK_RMENU },              		
	{ GS_KEY_BROWSER_BACK,		VK_BROWSER_BACK },       				
	{ GS_KEY_BROWSER_FORWARD,		VK_BROWSER_FORWARD },    					
	{ GS_KEY_BROWSER_REFRESH,		VK_BROWSER_REFRESH },    					
	{ GS_KEY_BROWSER_STOP,		VK_BROWSER_STOP },       				
	{ GS_KEY_BROWSER_SEARCH,		VK_BROWSER_SEARCH },     					
	{ GS_KEY_BROWSER_FAVORITES,	VK_BROWSER_FAVORITES },
	{ GS_KEY_BROWSER_HOME,		VK_BROWSER_HOME },       				
	{ GS_KEY_VOLUME_MUTE,			VK_VOLUME_MUTE },        			
	{ GS_KEY_VOLUME_DOWN,			VK_VOLUME_DOWN },        			
	{ GS_KEY_VOLUME_UP,			VK_VOLUME_UP },          		
	{ GS_KEY_MEDIA_NEXT_TRACK,	VK_MEDIA_NEXT_TRACK },   						
	{ GS_KEY_MEDIA_PREV_TRACK,	VK_MEDIA_PREV_TRACK },   						
	{ GS_KEY_MEDIA_STOP,			VK_MEDIA_STOP },         			
	{ GS_KEY_MEDIA_PLAY_PAUSE,	VK_MEDIA_PLAY_PAUSE },   						
	{ GS_KEY_LAUNCH_MAIL,			VK_LAUNCH_MAIL },        			
	{ GS_KEY_MEDIA_SELECT,		VK_LAUNCH_MEDIA_SELECT },				
	{ GS_KEY_APP_1,				VK_LAUNCH_APP1 },        
	{ GS_KEY_APP_2,				VK_LAUNCH_APP2 },        
//	{ GS_KEY_SEMICOLON,			VK_OEM_1 },              		
//	{ GS_KEY_PLUS,				VK_OEM_PLUS },           
//	{ GS_KEY_COMMA,				VK_OEM_COMMA },          
//	{ GS_KEY_MINUS,				VK_OEM_MINUS },          
//	{ GS_KEY_PERIOD,				VK_OEM_PERIOD },         	
//	{ GS_KEY_SLASH,				VK_OEM_2 },              
//	{ GS_KEY_TILDE,				VK_OEM_3 },              
//	{ GS_KEY_LEFT_BRACKET,		VK_OEM_4 },              				
//	{ GS_KEY_BACKSLASH,			VK_OEM_5 },              		
//	{ GS_KEY_RIGHT_BRACKET,		VK_OEM_6 },              				
//	{ GS_KEY_QUOTE,				VK_OEM_7 },              
	{ GS_KEY_PROCESS,				VK_PROCESSKEY },         	
	{ GS_KEY_ATTN,				VK_ATTN },               
	{ GS_KEY_CRSEL,				VK_CRSEL },              
	{ GS_KEY_EXSEL,				VK_EXSEL },              
	{ GS_KEY_EOF,					VK_EREOF },              
	{ GS_KEY_PLAY,				VK_PLAY },               
	{ GS_KEY_ZOOM,				VK_ZOOM },               
	{ GS_KEY_PA1,					VK_PA1 },                
	{ GS_KEY_CLEAR,				VK_OEM_CLEAR },
	
	//custom	
	{ GS_KEY_WHEEL_UP,			HGVK_MOUSE_WHEEL_UP },		
	{ GS_KEY_WHEEL_DOWN,			HGVK_MOUSE_WHEEL_DOWN },			
	{ GS_KEY_WHEEL,				0x80fd },

};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
INT_DICTIONARY* gKeyToStrDictionary = NULL;
STR_DICTIONARY* gStrToKeyDictionary = NULL;


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// create an alphabetized dictionary going from keyname to keycode
// and an ordered dictionary going from keycode to keyname
//----------------------------------------------------------------------------
void KeyConfigInit(
	void)
{
	KeyConfigFree();

	gKeyToStrDictionary = IntDictionaryInit();
	for (int ii = 0; ii < arrsize(keytable); ii++)
	{
		IntDictionaryAdd(gKeyToStrDictionary, keytable[ii].nKeyCode, keytable+ii);
	}
	gStrToKeyDictionary = StrDictionaryInit();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void KeyConfigFree(
	void)
{
	if (gKeyToStrDictionary)
	{
		IntDictionaryFree(gKeyToStrDictionary);
		gKeyToStrDictionary = NULL;
	}
	if (gStrToKeyDictionary)
	{
		StrDictionaryFree(gStrToKeyDictionary);
		gStrToKeyDictionary = NULL;
	}
}


//----------------------------------------------------------------------------
// return keycode given a keyname
//----------------------------------------------------------------------------
int KeyConfigGetKeyCodeFromString(
	char* str)
{

	// we could make this faster, but do we care?
	
	KEYNAME* search = (KEYNAME*)StrDictionaryFind(gStrToKeyDictionary, str);
	if (!search)
	{
		return 0;
	}
	return search->nKeyCode;
}


//----------------------------------------------------------------------------
// return keyname given a keycode
//----------------------------------------------------------------------------
const WCHAR *KeyConfigGetNameFromKeyCode(
	int keycode)
{
	GLOBAL_STRING eString = GS_INVALID;
	// the init isn't called right now because we really don't use it anymore
	//  but I need 3 values from the table so I'm going to put an alternate method
	//  in this function.
	if (gKeyToStrDictionary)		
	{
		KEYNAME* search = (KEYNAME*)IntDictionaryFind(gKeyToStrDictionary, keycode);
		if (!search)
		{
			return NULL;
		}
		eString = search->eString;
	}
	else
	{
		int nCount = arrsize(keytable);
		for (int i = 0; i < nCount; i++)
		{
			if (keytable[i].nKeyCode == (int)keycode)
			{
				eString = keytable[i].eString;
				break;
			}
		}
	}

	return eString != GS_INVALID ? GlobalStringGet( eString ) : NULL;
}


//----------------------------------------------------------------------------
// return string table keyname given a keycode
//----------------------------------------------------------------------------
const WCHAR* KeyConfigGetUniNameFromKeyCode(
	int keycode)
{
	UNREFERENCED_PARAMETER(keycode);
	return NULL;
}
