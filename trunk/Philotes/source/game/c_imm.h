#ifndef _INPUT_METHOD_MANAGER_H_
#define _INPUT_METHOD_MANAGER_H_

#define LANG_ABBRV_LEN		3
#define LANG_NAME_LEN		64

enum LANG_INDICATOR {
	INDICATOR_NON_IME,
	INDICATOR_CHS,
	INDICATOR_CHT,
	INDICATOR_KOREAN,
	INDICATOR_JAPANESE
};

enum IME_STATE {
	IMEUI_STATE_OFF,
	IMEUI_STATE_ON,
	IMEUI_STATE_ENGLISH
};

// Only applies to ChT & ChS
enum IME_TYPE {
	IME_INVALID,

	IME_CHT_NEW_PHONETIC,
	IME_CHT_NEW_CHANGJIE,
	IME_CHT_PHONETIC,
	IME_CHT_CHANGJIE,
	IME_CHT_UNICODE,
	IME_CHT_DAYI,
	IME_CHT_QUICK,
	IME_CHT_BIG5,
	IME_CHT_ARRAY,

	IME_CHT_BOSHIAMY,
	IME_CHT_CHEWING,
	IME_CHT_GOING80,
	IME_CHT_GOING75,

	IME_CHS_MSPY,
	IME_CHS_NEIMA,
	IME_CHS_QUANPIN,
	IME_CHS_SHUANGPIN,
	IME_CHS_ZHENGMA,
};

struct CInputLocale
{
	HKL   m_hKL;            // Keyboard layout
	WCHAR m_wszLangAbb[LANG_ABBRV_LEN];  // Language abbreviation
	WCHAR m_wszLang[LANG_NAME_LEN];    // Localized language name
};

BOOL IMM_Initialize(MEMORYPOOL* pPool);
BOOL IMM_Shutdown();
BOOL IMM_UpdateLanguageChange();
BOOL IMM_StartComposition();
BOOL IMM_EndComposition();
BOOL IMM_ProcessComposition(LPARAM lParam);
BOOL IMM_Enable(BOOL bEnable);
BOOL IMM_IsEnabled();
BOOL IMM_IgnoreOpenStatus();
BOOL IMM_UpdateOpenStatus();
DWORD IMM_GetVirtualKey();
BOOL IMM_HandleNotify(WPARAM wParam, LPARAM lParam, BOOL* bTrapped);

void IMM_GetLangIndicator(LPWSTR buf, DWORD len);
DWORD IMM_GetPrimaryLanguage();

void IMM_SetLanguageChangeHandler(void* (*handler)(void*));
void IMM_SetReadingWindowChangeHandler(void* (*handler)(void*));
void IMM_SetCandidateWindowChangeHandler(void* (*handler)(void*));

#endif
