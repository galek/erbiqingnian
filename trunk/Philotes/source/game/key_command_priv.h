// key_command_priv.h

#ifdef KEY_COMMAND_ENUM
#define KEY_CMD(c, k1, km1, k2, km2, re, cf, prm, s)				c,
#else 
#define KEY_CMD(c, k1, km1, k2, km2, re, cf, prm, s)				{ c, k1, km1, k2, km2, re, cf, prm, s, INVALID_LINK, #c },
#endif

#ifndef KEY_COMMAND_VERSION
#define KEY_COMMAND_VERSION		2
#endif

		// COMMAND					// KEY 1				// MOD 1	// KEY 2				// MOD 2	// remap	// confl
KEY_CMD(CMD_GAME_MENU,				VK_ESCAPE,				0,			0,						0,			FALSE,		TRUE,		0, "key command game menu" )
KEY_CMD(CMD_MOVE_LEFT,				'A',					0,			VK_LEFT,				0,			TRUE,		TRUE,		0, "key command left" )
KEY_CMD(CMD_MOVE_RIGHT,				'D',					0,			VK_RIGHT,				0,			TRUE,		TRUE,		0, "key command right" )
KEY_CMD(CMD_MOVE_FORWARD,			'W',					0,			VK_UP,					0,			TRUE,		TRUE,		0, "key command forward" )
KEY_CMD(CMD_MOVE_BACK,				'S',					0,			VK_DOWN,				0,			TRUE,		TRUE,		0, "key command back" )
KEY_CMD(CMD_TURN_LEFT,				0,						0,			0,						0,			TRUE,		TRUE,		0, "key command turn left" )
KEY_CMD(CMD_TURN_RIGHT,				0,						0,			0,						0,			TRUE,		TRUE,		0, "key command turn right" )
KEY_CMD(CMD_JUMP,					VK_SPACE,				0,			0,						0,			TRUE,		TRUE,		0, "key command jump" )
KEY_CMD(CMD_AUTORUN,				VK_NUMLOCK,				0,			VK_XBUTTON1,			0,			TRUE,		TRUE,		0, "key command autorun" )

KEY_CMD(CMD_ZOOM_IN,				HGVK_MOUSE_WHEEL_UP,	0,			0,						0,			TRUE,		TRUE,		0, "key command zoom in" )
KEY_CMD(CMD_ZOOM_OUT,				HGVK_MOUSE_WHEEL_DOWN,	0,			0,						0,			TRUE,		TRUE,		0, "key command zoom out" )
KEY_CMD(CMD_CAMERAORBIT,			VK_OEM_3,				0,			VK_MBUTTON,				0,			TRUE,		TRUE,		0, "key command camera orbit" )
KEY_CMD(CMD_RESETCAMERAORBIT,		VK_INSERT,				0,			0,						0,			TRUE,		TRUE,		0, "key command reset camera orbit" )
KEY_CMD(CMD_CAMERA_LEFT,			VK_LEFT,				0,			'A',						0,			TRUE,		TRUE,		0, "key command move left" )
KEY_CMD(CMD_CAMERA_RIGHT,			VK_RIGHT,				0,			'D',						0,			TRUE,		TRUE,		0, "key command move right" )
KEY_CMD(CMD_CAMERA_FORWARD,			VK_UP,					0,			'W',						0,			TRUE,		TRUE,		0, "key command move forward" )
KEY_CMD(CMD_CAMERA_BACK,			VK_DOWN,				0,			'S',						0,			TRUE,		TRUE,		0, "key command move back" )
KEY_CMD(CMD_CAMERA_ORBITLEFT,		VK_NUMPAD7,				0,			'Q',						0,			TRUE,		TRUE,		0, "key command camera left" )
KEY_CMD(CMD_CAMERA_ORBITRIGHT,		VK_NUMPAD9,				0,			'E',						0,			TRUE,		TRUE,		0, "key command camera right" )

KEY_CMD(CMD_SKILL_SHIFT,			VK_SHIFT,				VK_SHIFT,	0,						0,			TRUE,		TRUE,		0, "key command shift skill" )
KEY_CMD(CMD_SKILL_POTION,			VK_CONTROL,				VK_CONTROL,	0,						0,			TRUE,		TRUE,		0, "key command use potion" )
KEY_CMD(CMD_SHOW_ITEMS,				VK_MENU,				VK_MENU,	0,						0,			TRUE,		TRUE,		0, "key command toggle menu" )
KEY_CMD(CMD_FORCEMOVE,				'Z',					0,			0,						0,			TRUE,		TRUE,		0, "key command force move" )

KEY_CMD(CMD_ENGAGE_PVP,				VK_CONTROL,				0,			0,						0,			TRUE,		TRUE,		0, "key command engage pvp" )

KEY_CMD(CMD_WEAPON_SWAP,			'X',					0,			0,						0,			TRUE,		TRUE,		0, "key command weapon swap" )

KEY_CMD(CMD_OPEN_INVENTORY,			'I',					0,			0,						0,			TRUE,		TRUE,		0, "key command open inventory" )
KEY_CMD(CMD_OPEN_INVENTORY_TUGBOAT,	'I',					0,			'B',					0,			TRUE,		TRUE,		0, "key command open inventory" )

KEY_CMD(CMD_OPEN_CHARACTER,			'C',					0,			0,						0,			TRUE,		TRUE,		0, "key command open character" )
//KEY_CMD(CMD_AUTO_PARTY,			'P',					0,			0,						0,			TRUE,		TRUE,		0, "key command auto party" )
																																	
KEY_CMD(CMD_OPEN_MAP,				VK_TAB,					0,			0,						0,			TRUE,		TRUE,		0, "key command automap" )
KEY_CMD(CMD_OPEN_WORLD_MAP,			'M',					0,			0,						0,			TRUE,		TRUE,		0, "key command world map" )
KEY_CMD(CMD_MAP_ZOOM_IN,			VK_ADD,					0,			VK_OEM_4,				0,			TRUE,		TRUE,		0, "key command map zoom in" )
KEY_CMD(CMD_MAP_ZOOM_OUT,			VK_SUBTRACT,			0,			VK_OEM_6,				0,			TRUE,		TRUE,		0, "key command map zoom out" )
KEY_CMD(CMD_MYTHOS_MAP_ZOOM_OUT,	VK_OEM_4,				0,			0,						0,			TRUE,		TRUE,		0, "key command map zoom out" )
KEY_CMD(CMD_MYTHOS_MAP_ZOOM_IN,		VK_OEM_6,				0,			0,						0,			TRUE,		TRUE,		0, "key command map zoom in" )

KEY_CMD(CMD_OPEN_QUESTLOG,			'L',					0,			0,						0,			TRUE,		TRUE,		0, "key command quest log" )
																																	
KEY_CMD(CMD_OPEN_SKILLS,			'K',					0,			0,						0,			TRUE,		TRUE,		0, "key command open skills" )
KEY_CMD(CMD_OPEN_PERKS,				'U',					0,			0,						0,			TRUE,		TRUE,		0, "key command open perks" )
KEY_CMD(CMD_REMOVE_SKILL,			VK_DELETE,				0,			0,						0,			FALSE,		FALSE,		0, "key command discard skill" )
KEY_CMD(CMD_OPEN_TALENTS,			'T',					0,			0,					0,			TRUE,		TRUE,		0, "key command open spells" )
KEY_CMD(CMD_OPEN_ACHIEVEMENTS,		'J',					0,			0,						0,			TRUE,		TRUE,		0, "key command open achievements" )
KEY_CMD(CMD_OPEN_ACHIEVEMENTS_MYTHOS,'J',					0,			0,						0,			TRUE,		TRUE,		0, "key command open achievements" )
KEY_CMD(CMD_OPEN_CRAFTING_MYTHOS,	'R',					0,			0,						0,			TRUE,		TRUE,		0, "key command open crafting" )
																																	
KEY_CMD(CMD_OPEN_BUDDYLIST,			'B',					0,			0,						0,			TRUE,		TRUE,		0, "key command open friend list" )
KEY_CMD(CMD_OPEN_FRIENDLIST,		'F',					0,			0,						0,			TRUE,		TRUE,		0, "key command open friend list" )
KEY_CMD(CMD_OPEN_GUILDPANEL,		'G',					0,			0,						0,			TRUE,		TRUE,		0, "key command open guild panel" )
KEY_CMD(CMD_OPEN_PARTYPANEL,		'P',					0,			0,						0,			TRUE,		TRUE,		0, "key command open party panel" )
KEY_CMD(CMD_OPEN_EMAIL,				'Z',					0,			0,						0,			TRUE,		TRUE,		0, "key command open email" )
KEY_CMD(CMD_OPEN_EMAIL_TUGBOAT,		'N',					0,			0,						0,			TRUE,		TRUE,		0, "key command open email" )

KEY_CMD(CMD_ITEM_PICKUP,			'F',					0,			0,						0,			TRUE,		TRUE,		0, "key command pickup item" )
																																	 
KEY_CMD(CMD_VANITY_MODE,			'V',					0,			0,						0,			TRUE,		TRUE,		0, "key command vanity camera" )
KEY_CMD(CMD_CHAT_TOGGLE,			VK_OEM_3,				0,			0,						0,			TRUE,		TRUE,		0, "key command toggle chat" )
																															 		 
#if ISVERSION(DEVELOPMENT)	// debug features
KEY_CMD(CMD_CAM_SINGLE_STEP,		VK_OEM_5,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CAM_RESUME_SPEED,		'O',					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CAM_SLOWER_SPEED,		VK_OEM_4,				VK_SHIFT,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CAM_FASTER_SPEED,		VK_OEM_6,				VK_SHIFT,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_TUG_CAM_SLOWER_SPEED,	VK_SUBTRACT,			0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_TUG_CAM_FASTER_SPEED,	VK_ADD,					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CAM_DETATCH,			VK_OEM_1,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_SELECT_DEBUG_UNIT,		VK_OEM_COMMA,			VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_SELECT_DEBUG_SELF,		VK_OEM_PERIOD,			VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_SIMULATE_BAD_FPS,		VK_OEM_5,				VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
																															 		 
KEY_CMD(CMD_DEBUG_TEXT_LENGTH,		'G',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DEBUG_TEXT_LABELS,		'L',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DEBUG_TEXT_DEVELOPER,	'D',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DEBUG_TEXT_ENGLISH,		'E',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DEBUG_UI_CLIPPING,		'C',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DEBUG_FONT_POINT_SIZE,	'P',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
																																	 
KEY_CMD(CMD_DEBUG_UI_EDIT,			'Z',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DEBUG_UI_RELOAD,		'R',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
																																	 
KEY_CMD(CMD_DET_CAM_STRAFE_LEFT,	'A',					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DET_CAM_BACK,			'S',					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DET_CAM_FORWARD,		'W',					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DET_CAM_STRAFE_RIGHT,	'D',					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DET_CAM_UP,				VK_UP,					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DET_CAM_DOWN,			VK_DOWN,				0,			0,						0,			FALSE,		FALSE,		0, "" )

KEY_CMD(CMD_EXIT,					VK_ESCAPE,				VK_SHIFT,	0,						0,			TRUE,		FALSE,		0, "ui menu exit" )

KEY_CMD(CMD_OPEN_FULLLOG,			VK_ADD,					VK_SHIFT,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CLOSE_FULLLOG,			VK_SUBTRACT,			VK_SHIFT,	0,						0,			FALSE,		FALSE,		0, "" )

KEY_CMD(CMD_OPEN_AUCTION,			'K',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
#endif																																 

KEY_CMD(CMD_CONSOLE_TOGGLE,			VK_OEM_3,				VK_SHIFT,	0,						0,			FALSE,		TRUE,		0, "key command toggle console" )

KEY_CMD(CMD_GAME_MENU_RETURN,		VK_ESCAPE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_GAME_MENU_DOSELECTED,	VK_RETURN,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_GAME_MENU_UP,			VK_UP,					0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_GAME_MENU_DOWN,			VK_DOWN,				0,			0,						0,			FALSE,		FALSE,		0, "" )
																															 		 
KEY_CMD(CMD_UI_ELEMENT_CLOSE,		VK_ESCAPE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_UI_TIP_CLOSE,			VK_ESCAPE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_VIDEO_INCOMING,			VK_RETURN,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_NPC_DIALOG_ACCEPT,		VK_RETURN,				0,			VK_SPACE,				0,			FALSE,		FALSE,		0, "" )
																															 		 
KEY_CMD(CMD_ACTIVESLOT_1,			'1',					0,			0,						0,			TRUE,		TRUE,		0, "key command activeslot 1" )
KEY_CMD(CMD_ACTIVESLOT_2,			'2',					0,			0,						0,			TRUE,		TRUE,		1, "key command activeslot 2" )
KEY_CMD(CMD_ACTIVESLOT_3,			'3',					0,			0,						0,			TRUE,		TRUE,		2, "key command activeslot 3" )
KEY_CMD(CMD_ACTIVESLOT_4,			'4',					0,			0,						0,			TRUE,		TRUE,		3, "key command activeslot 4" )
KEY_CMD(CMD_ACTIVESLOT_5,			'5',					0,			0,						0,			TRUE,		TRUE,		4, "key command activeslot 5" )
KEY_CMD(CMD_ACTIVESLOT_6,			'6',					0,			0,						0,			TRUE,		TRUE,		5, "key command activeslot 6" )
KEY_CMD(CMD_ACTIVESLOT_7,			'7',					0,			0,						0,			TRUE,		TRUE,		6, "key command activeslot 7" )
KEY_CMD(CMD_ACTIVESLOT_8,			'8',					0,			0,						0,			TRUE,		TRUE,		7, "key command activeslot 8" )
KEY_CMD(CMD_ACTIVESLOT_9,			'9',					0,			0,						0,			TRUE,		TRUE,		8, "key command activeslot 9" )
KEY_CMD(CMD_ACTIVESLOT_10,			'0',					0,			0,						0,			TRUE,		TRUE,		9, "key command activeslot 10" )
KEY_CMD(CMD_ACTIVESLOT_11,			'Q',					0,			0,						0,			TRUE,		TRUE,		10, "key command activeslot 11" )
KEY_CMD(CMD_ACTIVESLOT_12,			'E',					0,			0,						0,			TRUE,		TRUE,		11, "key command activeslot 12" )
KEY_CMD(CMD_ACTIVESLOT_11_T,		VK_OEM_MINUS,			0,			0,						0,			TRUE,		TRUE,		10, "key command activeslot 11" )
KEY_CMD(CMD_ACTIVESLOT_12_T,		VK_OEM_PLUS,			0,			0,						0,			TRUE,		TRUE,		11, "key command activeslot 12" )

// tugboat
KEY_CMD(CMD_ACTIVESLOT_1CTRL,		'1',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		0, "key command hireling activeslot 1" )
KEY_CMD(CMD_ACTIVESLOT_2CTRL,		'2',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		1, "key command hireling activeslot 2" )
KEY_CMD(CMD_ACTIVESLOT_3CTRL,		'3',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		2, "key command hireling activeslot 3" )
KEY_CMD(CMD_ACTIVESLOT_4CTRL,		'4',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		3, "key command hireling activeslot 4" )
KEY_CMD(CMD_ACTIVESLOT_5CTRL,		'5',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		4, "key command hireling activeslot 5" )
KEY_CMD(CMD_ACTIVESLOT_6CTRL,		'6',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		5, "key command hireling activeslot 6" )
KEY_CMD(CMD_ACTIVESLOT_7CTRL,		'7',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		6, "key command hireling activeslot 7" )
KEY_CMD(CMD_ACTIVESLOT_8CTRL,		'8',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		7, "key command hireling activeslot 8" )
KEY_CMD(CMD_ACTIVESLOT_9CTRL,		'9',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		8, "key command hireling activeslot 9" )
KEY_CMD(CMD_ACTIVESLOT_10CTRL,		'0',					VK_CONTROL,	0,						0,			TRUE,		TRUE,		9, "key command hireling activeslot 10" )
KEY_CMD(CMD_ACTIVESLOT_11_CTRL,		VK_OEM_MINUS,			VK_CONTROL,	0,						0,			TRUE,		TRUE,		10, "key command hireling activeslot 11" )
KEY_CMD(CMD_ACTIVESLOT_12_CTRL,		VK_OEM_PLUS,			VK_CONTROL,	0,						0,			TRUE,		TRUE,		11, "key command hireling activeslot 12" )

// For the RTS																														
KEY_CMD(CMD_ACTIVESLOT_13,			'1',					0,			0,						0,			FALSE,		FALSE,		12, "" )
KEY_CMD(CMD_ACTIVESLOT_14,			'2',					0,			0,						0,			FALSE,		FALSE,		13, "" )
KEY_CMD(CMD_ACTIVESLOT_15,			'3',					0,			0,						0,			FALSE,		FALSE,		14, "" )
KEY_CMD(CMD_ACTIVESLOT_16,			'4',					0,			0,						0,			FALSE,		FALSE,		15, "" )
KEY_CMD(CMD_ACTIVESLOT_17,			'5',					0,			0,						0,			FALSE,		FALSE,		16, "" )
KEY_CMD(CMD_ACTIVESLOT_18,			'6',					0,			0,						0,			FALSE,		FALSE,		17, "" )
KEY_CMD(CMD_ACTIVESLOT_19,			'7',					0,			0,						0,			FALSE,		FALSE,		18, "" )
KEY_CMD(CMD_ACTIVESLOT_20,			'8',					0,			0,						0,			FALSE,		FALSE,		19, "" )
KEY_CMD(CMD_ACTIVESLOT_21,			'9',					0,			0,						0,			FALSE,		FALSE,		20, "" )
KEY_CMD(CMD_ACTIVESLOT_22,			'0',					0,			0,						0,			FALSE,		FALSE,		21, "" )
KEY_CMD(CMD_ACTIVESLOT_23,			'Q',					0,			0,						0,			FALSE,		FALSE,		22, "" )
KEY_CMD(CMD_ACTIVESLOT_24,			'E',					0,			0,						0,			FALSE,		FALSE,		23, "" )
																																	 
// tugboat																															 
KEY_CMD(CMD_HOTSPELL_1,				VK_F1,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 1" )
KEY_CMD(CMD_HOTSPELL_2,				VK_F2,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 2" )
KEY_CMD(CMD_HOTSPELL_3,				VK_F3,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 3" )
KEY_CMD(CMD_HOTSPELL_4,				VK_F4,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 4" )
KEY_CMD(CMD_HOTSPELL_5,				VK_F5,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 5" )
KEY_CMD(CMD_HOTSPELL_6,				VK_F6,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 6" )
KEY_CMD(CMD_HOTSPELL_7,				VK_F7,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 7" )
KEY_CMD(CMD_HOTSPELL_8,				VK_F8,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 8" )
KEY_CMD(CMD_HOTSPELL_9,				VK_F9,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 9" )
KEY_CMD(CMD_HOTSPELL_10,			VK_F10,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 10" )
KEY_CMD(CMD_HOTSPELL_11,			VK_F11,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 11" )
KEY_CMD(CMD_HOTSPELL_12,			VK_F12,					0,			0,						0,			TRUE,		TRUE,		0, "key command hotspell 12" )
//end tugboat																														 
KEY_CMD(CMD_WEAPONCONFIG_1,			VK_F1,					0,			0,						0,			TRUE,		TRUE,		0, "key command weapon config 1" )
KEY_CMD(CMD_WEAPONCONFIG_2,			VK_F2,					0,			0,						0,			TRUE,		TRUE,		0, "key command weapon config 2" )
KEY_CMD(CMD_WEAPONCONFIG_3,			VK_F3,					0,			0,						0,			TRUE,		TRUE,		0, "key command weapon config 3" )
																																	
KEY_CMD(CMD_ANALYZE,				VK_F5,					0,			0,						0,			TRUE,		TRUE,		0, "key command analyze" )
KEY_CMD(CMD_GOSSIP,					VK_F6,					0,			0,						0,			TRUE,		TRUE,		0, "key command gossip" )
																																	 
KEY_CMD(CMD_RESTART_AFTER_DEATH,	VK_SPACE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_HIDE_ALL,				VK_SPACE,				0,			0,						0,			TRUE,		TRUE,		0, "key command hide menus" )
																																	 
//KEY_CMD(CMD_OPEN_CHAT_SCREEN,		'X',					0,			0,						0,			TRUE,		TRUE,		0, "key command open chat" )
																																	 
KEY_CMD(CMD_CHAT_ENTRY_TOGGLE_SLASH,'/',					0,			VK_OEM_2,				0,			FALSE,		TRUE,		0, "" )		
KEY_CMD(CMD_CHAT_ENTRY_TOGGLE,		VK_RETURN,				0,			0,						0,			FALSE,		TRUE,		0, "" )
KEY_CMD(CMD_CHAT_ENTRY_PREV_CMD,	VK_UP,					0,			VK_PRIOR,				0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CHAT_ENTRY_NEXT_CMD,	VK_DOWN,				0,			VK_NEXT,				0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_CHAT_ENTRY_EXIT,		VK_ESCAPE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
																															 
#if ISVERSION(DEVELOPMENT)																									 
//KEY_CMD(CMD_SCREENSHOT_BURST,		VK_SNAPSHOT,			VK_SHIFT,	VK_SNAPSHOT,			VK_MENU,	FALSE,		FALSE ,		0, "" )
#endif
KEY_CMD(CMD_SCREENSHOT,				HGVK_SCREENSHOT_UI,		0,			HGVK_SCREENSHOT_NO_UI,	VK_CONTROL,	FALSE,		FALSE ,		0, "" )

KEY_CMD(CMD_DEBUG_UP,				VK_UP,					0,			0,						0,			FALSE,		FALSE ,		0, "" )
KEY_CMD(CMD_DEBUG_DOWN,				VK_DOWN,				0,			0,						0,			FALSE,		FALSE ,		0, "" )
																																	 
KEY_CMD(CMD_UI_QUICK_EXIT,			VK_ESCAPE,				0,			VK_SPACE,				0,			FALSE,		FALSE,		0, "" )
																																	 
KEY_CMD(CMD_REMAP_KEY_CANCEL,		VK_ESCAPE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_REMAP_KEY_CLEAR,		VK_DELETE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
																															 
#if ISVERSION(DEVELOPMENT)
KEY_CMD(CMD_DRB_DEBUG,				'M',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_DRB_DEBUG2,				'N',					VK_CONTROL,	0,						0,			FALSE,		FALSE,		0, "" )
#endif
																																	 
KEY_CMD(CMD_MOVIE_PAUSE,			VK_SPACE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
KEY_CMD(CMD_MOVIE_CANCEL,			VK_ESCAPE,				0,			0,						0,			FALSE,		FALSE,		0, "" )
																																	 
#undef KEY_CMD																									 			 