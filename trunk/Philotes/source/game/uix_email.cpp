//----------------------------------------------------------------------------
// uix_email.cpp
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "gag.h"
#include "gamemail.h"
#include "uix.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "uix_email.h"
#include "uix_scheduler.h"
#include "language.h"
#include "stringreplacement.h"
#include "globalindex.h"
#include "units.h"
#include "player.h"
#include "c_font.h"
#include "inventory.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_trade.h"
#include "imports.h"
#include "gamemail.h"
#include "wordfilter.h"
#include "c_imm.h"
#include "s_playerEmail.h"
#include "c_playerEmail.h"

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// Static variables
//----------------------------------------------------------------------------

#define MAX_MAIL_MSGS 1000

static PLAYER_EMAIL_MESSAGE	s_PLAYER_EMAIL_MESSAGEs[MAX_MAIL_MSGS];
static int					s_iMsgCount = 0;

static ULONGLONG				s_nSelectedDeletionEmailId = INVALID_GUID;
static PLAYER_EMAIL_MESSAGE *	s_pSelectedEmail = NULL;
static int						s_iSelectedEmailIdx = -1;
static PLAYER_EMAIL_TYPES		s_nMailType = PLAYER_EMAIL_TYPE_PLAYER;

enum EMAIL_SORT_ENUM
{
	eEmailSortDate,
	eEmailSortFrom,
	eEmailSortSubject,
	eEmailSortExpries,

	EMAIL_SORT_COUNT
};

static const char * s_szSortText[EMAIL_SORT_COUNT] =
{
	"email_date",
	"email_from",
	"email_subject",
	"email_expires"
};

static const BOOL s_bSortAscDsc[EMAIL_SORT_COUNT] =
{
	TRUE, FALSE, FALSE, TRUE
};

static EMAIL_SORT_ENUM	s_eEmailSort = eEmailSortDate;
static BOOL				s_bSortDescending = TRUE;
static BOOL				s_bListNeedsSort = TRUE;
static BOOL				s_bAwaitingSendResult = FALSE;
static PGUID			s_nOutboxItem = INVALID_GUID;
static int				s_nAttachedMoney = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleEmail(
	void)
{
#if !ISVERSION(DEVELOPMENT)
	if (AppIsSinglePlayer())
	{
		return;
	}
#endif
	
	if( AppIsHellgate() )
	{
		UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_EMAIL_PANEL);

		if (pComponent)
		{
			if (UIComponentGetVisible(pComponent))
			{
				UIComponentActivate(pComponent, FALSE);
			}
			else
			{
				UIComponentActivate(pComponent, TRUE);
			}
		}
	}
	else
	{
		UITogglePaneMenu( KPaneMenuMail, KPaneMenuBackpack, KPaneMenuEquipment );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleEmail(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIToggleEmail();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEmailIsInReadMode(
	void)
{
	return UIComponentGetVisibleByEnum(UICOMP_EMAIL_VIEW_PANEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEmailIsInComposeMode(
	void)
{
	return UIComponentGetVisibleByEnum(UICOMP_EMAIL_COMPOSE_PANEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sEmailCompare(
	const void * item1,
	const void * item2)
{
	const PLAYER_EMAIL_MESSAGE * a = (const PLAYER_EMAIL_MESSAGE *)item1;
	const PLAYER_EMAIL_MESSAGE * b = (const PLAYER_EMAIL_MESSAGE *)item2;

	ASSERT_RETZERO(a && b);

	int iRetVal = 0;

	switch (s_eEmailSort)
	{
		case eEmailSortDate:
		{
			if (a->TimeSentUTC < b->TimeSentUTC)
			{
				iRetVal = -1;
			}
			else if (a->TimeSentUTC > b->TimeSentUTC)
			{
				iRetVal = 1;
			}
			break;
		}
		case eEmailSortFrom:
		{
			iRetVal = PStrCmp(a->wszSenderCharacterName, b->wszSenderCharacterName);
			break;
		}
		case eEmailSortSubject:
		{
			iRetVal = PStrCmp(a->wszEmailSubject, b->wszEmailSubject);
			break;
		}
		case eEmailSortExpries:
		{
			int iRemainingMsA = a->CalculateSecondsUntilDeletion();
			int iRemainingMsB = b->CalculateSecondsUntilDeletion();
			if (iRemainingMsA < iRemainingMsB)
			{
				iRetVal = -1;
			}
			else if (iRemainingMsA > iRemainingMsB)
			{
				iRetVal = 1;
			}
			break;
		}
	};

	if (s_bSortDescending)
	{
		iRetVal = -iRetVal;
	}

	return iRetVal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEmailSetBlinky(
	void)
{
	if( AppIsHellgate() )
	{
		if (!UIComponentGetVisibleByEnum(UICOMP_EMAIL_PANEL))
		{
			UI_COMPONENT *pButton = UIComponentGetByEnum(UICOMP_EMAIL_PANEL_BTN);
			if (pButton)
			{
				UI_COMPONENT *pBlinky = UIComponentFindChildByName(pButton, "incoming email notify");
				if (pBlinky)
				{
					UIComponentSetVisible(pBlinky, TRUE);
					UIComponentBlink(pBlinky, NULL, NULL, TRUE);
				}
			}
		}
	}
	else
	{
		if (!UIComponentGetVisibleByEnum(UICOMP_PANEMENU_MAIL ))
		{
			UI_COMPONENT *pBlinky = UIComponentGetByEnum(UICOMP_EMAIL_INCOMING_NOTIFY);
			if (pBlinky)
			{
				UIComponentSetVisible(pBlinky, TRUE);
				UIComponentBlink(pBlinky, NULL, NULL, TRUE);
			}
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIEmailClearOutboxItem(
	void)
{
	if (s_nOutboxItem != INVALID_GUID)
	{
		// return outbox item to inventory
		c_PlayerEmailRestoreOutboxAttachments();
		s_nOutboxItem = INVALID_GUID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sUIEmailGetMailColor(
	const PLAYER_EMAIL_TYPES eType,
	const EMAIL_STATUS eStatus )
{
	switch (eStatus)
	{
	case EMAIL_STATUS_ACCEPTED:
		{
			switch (eType)
			{
			case PLAYER_EMAIL_TYPE_PLAYER:
			default:
				return GetFontColor( FONTCOLOR_LIGHT_YELLOW );
			case PLAYER_EMAIL_TYPE_GUILD:
				return GetFontColor( FONTCOLOR_CHAT_GUILD );
			case PLAYER_EMAIL_TYPE_CSR:
			case PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE:
				return GetFontColor( FONTCOLOR_CHAT_CSR );
			case PLAYER_EMAIL_TYPE_SYSTEM:
				return GetFontColor( FONTCOLOR_CHAT_ANNOUNCEMENT );
			case PLAYER_EMAIL_TYPE_AUCTION:
				return GetFontColor( FONTCOLOR_VERY_LIGHT_PURPLE );
			};
		}
		break;
	case EMAIL_STATUS_BOUNCED:
		return GetFontColor( FONTCOLOR_LIGHT_RED );
	case EMAIL_STATUS_ABANDONED_COMPOSING:
	default:
		return GetFontColor( FONTCOLOR_LIGHT_RED );
	};
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIEmailSetEditColor(
	const PLAYER_EMAIL_TYPES eType,
	const EMAIL_STATUS eStatus )
{
	UI_COMPONENT *pComponent, *pChild;
	pComponent = UIComponentGetByEnum(UICOMP_EMAIL_BODY_PANEL);
	ASSERT_RETURN(pComponent);

	const DWORD dwColor = sUIEmailGetMailColor(eType, eStatus);

	if ((pChild = UIComponentFindChildByName(pComponent, "message subject editctrl")) != NULL)
	{
		pChild->m_dwColor = dwColor;
	}

	if ((pChild = UIComponentFindChildByName(pComponent, "message money editctrl")) != NULL)
	{
		pChild->m_dwColor = dwColor;
	}

	if ((pChild = UIComponentFindChildByName(pComponent, "message from label")) != NULL)
	{
		pChild->m_dwColor = dwColor;
	}

	if ((pChild = UIComponentFindChildByName(pComponent, "message from label bold")) != NULL)
	{
		pChild->m_dwColor = dwColor;
	}

	if ((pChild = UIComponentFindChildByName(pComponent, "message date label")) != NULL)
	{
		pChild->m_dwColor = dwColor;
	}

	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, NULL, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIEmailCapMoney(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);

	// clean up money string (leading zeros for instance)
	int nMoney = c_GetMoneyValueInComponent( pComponent );

	// cap the money value 
	cCurrency playerCurrency = UnitGetCurrency( UIGetControlUnit() );
	int nMoneyMax = playerCurrency.GetValue( KCURRENCY_VALUE_INGAME );
	nMoney = PIN( nMoney, 0, nMoneyMax );


	if (nMoney > 0)
	{
		c_SetMoneyValueInComponent( pComponent, nMoney );
	}
	else
	{
		UILabelSetText(pComponent, L"");
	}

	s_nAttachedMoney = nMoney;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIEmailSelectMessage(
	PLAYER_EMAIL_MESSAGE * pPLAYER_EMAIL_MESSAGE,
	int iEmailIdx)
{
	s_pSelectedEmail = pPLAYER_EMAIL_MESSAGE;
	s_iSelectedEmailIdx = iEmailIdx;

	UI_COMPONENT * pBodyPanel = UIComponentGetByEnum(UICOMP_EMAIL_BODY_PANEL);
	if (pBodyPanel)
	{
		UI_COMPONENT * pInvSlot = UIComponentFindChildByName(pBodyPanel, "email inbox slot");
		if (pInvSlot)
		{
			UNIT * pItem = s_pSelectedEmail ? UnitGetByGuid(AppGetCltGame(), s_pSelectedEmail->AttachedItemId) : NULL;
			UIInvSlotSetUnitId(UICastToInvSlot(pInvSlot), pItem ? pItem->unitid : INVALID_ID);
		}
	}

	UIEmailUpdateComposeButtonState();
	UIEmailRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailNewMailNotification(
	void)
{
	// already blinking?
	//
	if (AppIsHellgate())
	{
		UI_COMPONENT *pButton = UIComponentGetByEnum(UICOMP_EMAIL_PANEL_BTN);
		if (pButton)
		{
			UI_COMPONENT *pBlinky = UIComponentFindChildByName(pButton, "incoming email notify");
			if (pBlinky && UIComponentGetVisible(pBlinky))
			{
				return;
			}
		}
	}
	else
	{
		if (UIComponentGetVisibleByEnum(UICOMP_EMAIL_INCOMING_NOTIFY))
		{
			return;
		}
	}

	// play sound
	//
	int nReceiveSound = GlobalIndexGet( GI_SOUND_EMAIL_RECEIVE );
	if (nReceiveSound != INVALID_LINK)
	{
		c_SoundPlay(nReceiveSound, &c_SoundGetListenerPosition(), NULL);
	}

	// already in email panel?
	//
	if (AppIsHellgate() && UIComponentGetVisibleByEnum(UICOMP_EMAIL_PANEL))
	{
		return;
	}
	if (AppIsTugboat() && UIComponentGetVisibleByEnum(UICOMP_PANEMENU_MAIL))
	{
		return;
	}

	// flash notification
	//
	sEmailSetBlinky();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUIEmailRefreshMailList(
	void)
{
	if (c_PlayerEmailMessageCount() > 0)
	{
		PGUID PLAYER_EMAIL_MESSAGEIds[MAX_MAIL_MSGS];

		s_iMsgCount = c_PlayerEmailGetMessageIds(PLAYER_EMAIL_MESSAGEIds, MAX_MAIL_MSGS, FALSE);

		for (int i=0; i<s_iMsgCount; ++i)
		{
			c_PlayerEmailGetMessage(PLAYER_EMAIL_MESSAGEIds[i], s_PLAYER_EMAIL_MESSAGEs[i]);
		}

		// sort s_PLAYER_EMAIL_MESSAGEs...
		qsort(s_PLAYER_EMAIL_MESSAGEs, s_iMsgCount, sizeof(PLAYER_EMAIL_MESSAGE), sEmailCompare);

		// ...and update s_pSelectedEmail if necessary
		if (s_iSelectedEmailIdx != -1)
		{
			sUIEmailSelectMessage(&s_PLAYER_EMAIL_MESSAGEs[s_iSelectedEmailIdx], s_iSelectedEmailIdx);
		}
	}

	return s_iMsgCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailOnSendResult(
	PLAYER_EMAIL_RESULT eMailResult )
{
	sUIEmailRefreshMailList();

	BOOL bSuccess = (BOOL)-1;

	// TODO: remove these hard coded string keys and put in the global string table

	switch (eMailResult)
		{
		case PLAYER_EMAIL_SUCCESS:
		case PLAYER_EMAIL_UNKNOWN_ERROR_SILENT:	//	an error occurred that will be handled later by an auto bounce behind the scenes, show nothing to the user now
		{
			bSuccess = TRUE;
			break;
		}
		case PLAYER_EMAIL_TARGET_IGNORING_SENDER:	//	silently fail to send message
		{
			bSuccess = TRUE;
			c_PlayerEmailRestoreOutboxAttachments();
			break;
		}
		case PLAYER_EMAIL_GUILD_RANK_TOO_LOW:		//	SHOW TO USER: "You are not of a high enough guild rank to send guild-wide emails."
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), GlobalStringGet(GS_EMAIL_SEND_FAILURE_GUILD_RANK_TOO_LOW));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_TARGET_NOT_FOUND:			// SHOW TO USER: "That character doesn't exist!"
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), StringTableGetStringByKey("email_error_character_doesnt_exist"));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_NO_MULTSEND_ATTACHEMENTS:			// SHOW TO USER: "Cannot send attachments to multiple recipients!"
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), StringTableGetStringByKey("email_error_no_multsend_attachements"));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_INVALID_MONEY:
		case PLAYER_EMAIL_INVALID_ATTACHMENTS:				// SHOW TO USER: "Cannot attach that item or amount."
		{
			// KCK: Make sure the item gets cleared after failure cases
			sUIEmailClearOutboxItem();
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), StringTableGetStringByKey("email_error_invalid_attachment"));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_INVALID_ITEM:						// SHOW TO USER: "That item cannot be sent through e-mail.
		{
			// KCK: Make sure the item gets cleared after failure cases
			sUIEmailClearOutboxItem();
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), StringTableGetStringByKey("email_error_invalid_item"));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_NO_RECIPIENTS_SPECIFIED:			// SHOW TO USER: "No recipients specified!"
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), StringTableGetStringByKey("email_error_no_recipients_specified"));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_NOT_A_GUILD_MEMBER:				// SHOW TO USER: "You are not allowed to send e-mail to that guild."
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), StringTableGetStringByKey("email_error_not_a_guild_member"));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_TARGET_IS_TRIAL_USER:				// SHOW TO USER: "Cannot perform on Trial Players."
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), GlobalStringGet(GS_EMAIL_SEND_FAILURE_TARGET_IS_A_TRIAL_USER));
			bSuccess = FALSE;
			break;
		}
		case PLAYER_EMAIL_UNKNOWN_ERROR:			//	an error occurred that stopped the email from ever being created, show something to the user
		default:
		{
			WCHAR szStringBuf[256], szNum[32];
			LanguageFormatIntString(szNum, arrsize(szNum), (int)eMailResult);
			PStrCopy(szStringBuf, StringTableGetStringByKey("email_error_code"), arrsize(szStringBuf));
			PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_NUMBER), szNum );
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), szStringBuf);
			bSuccess = FALSE;
			break;
		}
	};

	if (s_bAwaitingSendResult && (bSuccess != (BOOL)-1))
	{
		s_bAwaitingSendResult = FALSE;

		if (bSuccess)
		{
			int nSendSound = GlobalIndexGet( GI_SOUND_EMAIL_SEND );
			if (nSendSound != INVALID_LINK)
			{
				c_SoundPlay(nSendSound, &c_SoundGetListenerPosition(), NULL);
			}

			UIEmailEnterReadMode();
		}
		else
		{
			UIEmailEnterComposeMode(NULL, NULL, FALSE);
		}

		UI_COMPONENT *pComposePanel = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL);
		if (pComposePanel)
		{
			UIEmailUpdateComposeButtonState();
		}
	}

	UIEmailRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailOnUpdate(
	void)
{
	sUIEmailRefreshMailList();
	UIEmailRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailInit(
	void)
{
	s_iMsgCount = 0;
	
	sUIEmailSelectMessage(NULL, -1);

	s_nMailType = PLAYER_EMAIL_TYPE_PLAYER;
	s_eEmailSort = eEmailSortDate;
	s_bListNeedsSort = TRUE;
	s_bAwaitingSendResult = FALSE;
	s_nOutboxItem = INVALID_ID;
	s_nAttachedMoney = 0;

	UI_COMPONENT *pDashboard = UIComponentGetByEnum(UICOMP_DASHBOARD);
	UI_COMPONENT *pEmailButtonPanel = pDashboard ? UIComponentFindChildByName(pDashboard, "email button panel") : NULL;

#if !ISVERSION(DEVELOPMENT)
	if (AppIsSinglePlayer())
	{
		if (pEmailButtonPanel)
		{
			UIComponentSetVisible(pEmailButtonPanel, FALSE);
			UIComponentSetActive(pEmailButtonPanel, FALSE);
		}
		return;
	}
#endif

	if (pEmailButtonPanel)
	{
		UIComponentSetVisible(pEmailButtonPanel, TRUE);
		UIComponentSetActive(pEmailButtonPanel, TRUE);
	}

	sUIEmailClearOutboxItem();
	UIEmailEnterReadMode();
	UIEmailUpdateSortLabel();
	UIEmailRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUIEmailGetCostToSend()
{
	if (!UIGetControlUnit())
		return 0;

	if (sEmailIsInComposeMode())
	{
		return x_PlayerEmailGetCostToSend(UIGetControlUnit(), s_nOutboxItem, s_nAttachedMoney);
	}
	else
	{
		return x_PlayerEmailGetCostToSend(UIGetControlUnit(), INVALID_GUID, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanReplyToEmailType(
	PLAYER_EMAIL_TYPES eEmailType)
{

	switch (eEmailType)
	{
		case PLAYER_EMAIL_TYPE_PLAYER:
		case PLAYER_EMAIL_TYPE_GUILD:
			return TRUE;
			
		case PLAYER_EMAIL_TYPE_CSR:
		case PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE:
		case PLAYER_EMAIL_TYPE_SYSTEM:
		case PLAYER_EMAIL_TYPE_AUCTION:
		case PLAYER_EMAIL_TYPE_CONSIGNMENT:
		default:
			return FALSE;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailUpdateComposeButtonState(
	void)
{
	UI_COMPONENT *pEmailPanel = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_EMAIL_PANEL) : UIComponentGetByEnum(UICOMP_PANEMENU_MAIL );
	if (pEmailPanel)
	{
		UI_COMPONENT *pComposeButton;
		if ((pComposeButton = UIComponentFindChildByName(pEmailPanel, "email compose button")) != NULL)
		{
			BOOL bEnableButton = TRUE;
			const WCHAR *szTooltipText = NULL;

			// check if the player has enough money
			//
			cCurrency playerCurrency = UnitGetCurrency( UIGetControlUnit() );
			const int nPlayerMoney = playerCurrency.GetValue( KCURRENCY_VALUE_INGAME );
			int nSendCost = sUIEmailGetCostToSend();
			if (sEmailIsInComposeMode())
			{
				nSendCost += s_nAttachedMoney;
			}

			if (UIGetControlUnit() && UnitHasBadge(UIGetControlUnit(), ACCT_TITLE_TRIAL_USER))
			{
				bEnableButton = FALSE;
				szTooltipText = GlobalStringGet(GS_TRIAL_PLAYERS_CANNOT_ACCESS);
			}
			else if (nPlayerMoney < nSendCost)
			{
				bEnableButton = FALSE;
				if( AppIsHellgate() )
				{
					szTooltipText = StringTableGetStringByKey("ui not enough palladium");
				}
				else
				{
					szTooltipText = StringTableGetStringByKey("ui insufficient postage");
				}
			}
			else // TODO: other checks here (?)
			{
			}

			if (sEmailIsInComposeMode())
			{
				// if we're in "compose" mode we need to update the send button as well
				//
				UI_COMPONENT *pComposePanel = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL);
				if (pComposePanel)
				{
					UI_COMPONENT *pSendButton;
					if ((pSendButton = UIComponentFindChildByName(pComposePanel, "email send button")) != NULL)
					{
						BOOL bEnableSendButton = bEnableButton;
						const WCHAR *szSendTooltipText = szTooltipText;
						if (s_bAwaitingSendResult)
						{
							bEnableSendButton = FALSE;
							szSendTooltipText = NULL;
						}
						if (!pSendButton->m_szTooltipText)
						{
							pSendButton->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
						}
						PStrCopy(pSendButton->m_szTooltipText, szSendTooltipText ? szSendTooltipText : L"", UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
						UIComponentSetActive(pSendButton, bEnableSendButton);
					}
				}

				bEnableButton = FALSE;
			}
			else if (sEmailIsInReadMode())
			{
				// if we're in "read" mode we need to update the reply button as well
				//
				UI_COMPONENT *pViewPanel = UIComponentGetByEnum(UICOMP_EMAIL_VIEW_PANEL);
				if (pViewPanel)
				{
					UI_COMPONENT *pReplyButton;
					if ((pReplyButton = UIComponentFindChildByName(pViewPanel, "email body reply button")) != NULL)
					{
						BOOL bEnableReplyButton = bEnableButton;
						const WCHAR *szReplyTooltipText = szTooltipText;
						if (!s_pSelectedEmail)
						{
							bEnableReplyButton = FALSE;
							szReplyTooltipText = NULL;
						}
						else
						{
							if (sCanReplyToEmailType( s_pSelectedEmail->eMessageType ) == FALSE)
							{
								bEnableReplyButton = FALSE;
							}
						}
						if (!pReplyButton->m_szTooltipText)
						{
							pReplyButton->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
						}
						PStrCopy(pReplyButton->m_szTooltipText, szReplyTooltipText ? szReplyTooltipText : L"", UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
						UIComponentSetActive(pReplyButton, bEnableReplyButton);
					}
				}
			}

			// update the button tooltip
			//
			if (!pComposeButton->m_szTooltipText)
			{
				pComposeButton->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
			}
			PStrCopy(pComposeButton->m_szTooltipText, szTooltipText ? szTooltipText : L"", UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);

			// (de)activate the compose button
			//
			UIComponentSetActive(pComposeButton, bEnableButton);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailOnActivate(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (ResultIsHandled(UIPanelOnActivate(component, msg, wParam, lParam)))
	{
		UIEmailUpdateComposeButtonState();

		sUIEmailRefreshMailList();
		UIEmailRepaint();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}


	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailOnPostActivate(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if( AppIsTugboat() )
	{
		UI_COMPONENT *pBlinky = UIComponentGetByEnum(UICOMP_EMAIL_INCOMING_NOTIFY);
		if (pBlinky)
		{
			UIComponentSetVisible(pBlinky, FALSE);
		}
	}
	else
	{
		UI_COMPONENT *pButton = UIComponentGetByEnum(UICOMP_EMAIL_PANEL_BTN);
		if (pButton)
		{
			UIButtonSetDown(UICastToButton(pButton), TRUE);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

			UI_COMPONENT *pBlinky = UIComponentFindChildByName(pButton, "incoming email notify");
			if (pBlinky)
			{
				UIComponentSetVisible(pBlinky, FALSE);
			}
		}		
	}
	

	if (s_iSelectedEmailIdx != -1)
	{
		sUIEmailSelectMessage(&s_PLAYER_EMAIL_MESSAGEs[s_iSelectedEmailIdx], s_iSelectedEmailIdx);
	}
	else
	{
		sUIEmailSelectMessage(NULL, -1);
	}

	sUIEmailRefreshMailList();
	UIEmailRepaint();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailOnPostInactivate(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	sUIEmailClearOutboxItem();

	IMM_Enable(FALSE);

	UI_COMPONENT *pButton = UIComponentGetByEnum(UICOMP_EMAIL_PANEL_BTN);
	if (pButton)
	{
		UIButtonSetDown(UICastToButton(pButton), FALSE);
		UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUIEmailMessageIsReturnedOrBounced(
	EMAIL_STATUS status )
{
	return (status == EMAIL_STATUS_BOUNCED || status == EMAIL_STATUS_ABANDONED_COMPOSING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailListOnPaint(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	UI_COMPONENT *pMessageListPanel = UIComponentGetByEnum(UICOMP_EMAIL_MESSAGE_LIST);
	ASSERT_RETVAL(pMessageListPanel, UIMSG_RET_NOT_HANDLED);
	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(UIComponentIterateChildren(pMessageListPanel, NULL, UITYPE_SCROLLBAR, FALSE));
	UI_COMPONENT *pHilightBar = UIComponentFindChildByName(pMessageListPanel, "email message hilight bar");
	UI_COMPONENT *pMessageListItem = NULL;
	UI_COMPONENT *pLabel = NULL;
	int iMsgIdx = 0, iComponentCount = 0;
	BOOL bHilightVisible = FALSE;

	while ((pMessageListItem = UIComponentIterateChildren(pMessageListPanel, pMessageListItem, UITYPE_PANEL, FALSE)) != NULL)
	{
		++iComponentCount;
	}

	if (pScrollBar)
	{
		pScrollBar->m_fMax = (float)(MAX(s_iMsgCount - iComponentCount, 0));
		iMsgIdx = FLOAT_TO_INT_ROUND(UIScrollBarGetValue(pScrollBar, FALSE));
	}

	pMessageListItem = NULL;
	while ((pMessageListItem = UIComponentIterateChildren(pMessageListPanel, pMessageListItem, UITYPE_PANEL, FALSE)) != NULL)
	{
		if (iMsgIdx < s_iMsgCount)
		{
			if ((pLabel = UIComponentFindChildByName(pMessageListItem, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].bIsMarkedRead ? "messagelist from label" : "messagelist from label bold")) != NULL)
			{
				UIComponentSetVisible(pLabel, TRUE);
				pLabel->m_dwColor = sUIEmailGetMailColor(s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].eMessageType, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].eMessageStatus);
				UILabelSetText(pLabel, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].wszSenderCharacterName);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].bIsMarkedRead ? "messagelist from label bold" : "messagelist from label")) != NULL)
			{
				UIComponentSetVisible(pLabel, FALSE);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, "messagelist date label")) != NULL)
			{
				WCHAR szTime[32] = L"";
				tm Time;
				localtime_s(&Time, &s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].TimeSentUTC);
				ASSERT(wcsftime(szTime, arrsize(szTime), L"%x", &Time) > 0);
				UILabelSetText(pLabel, szTime);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].bIsMarkedRead ? "messagelist subject label" : "messagelist subject label bold")) != NULL)
			{
				UIComponentSetVisible(pLabel, TRUE);

				WCHAR szSubject[MAX_EMAIL_SUBJECT];

				if (sUIEmailMessageIsReturnedOrBounced(s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].eMessageStatus))
				{
					PStrCopy(szSubject, StringTableGetStringByKey("email_subject_mail_returned"), arrsize(szSubject));
					PStrReplaceToken( szSubject, arrsize(szSubject), StringReplacementTokensGet(SR_MESSAGE), s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].wszEmailSubject );
				}
				else
				{
					PStrCopy(szSubject, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].wszEmailSubject, arrsize(szSubject));
				}

				ChatFilterStringInPlace(szSubject);
				UILabelSetText(pLabel, szSubject);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].bIsMarkedRead ? "messagelist subject label bold" : "messagelist subject label")) != NULL)
			{
				UIComponentSetVisible(pLabel, FALSE);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, "messagelist expires label")) != NULL)
			{
				WCHAR szStringBuf[100], szNum[32];
				int iRemainingDays = s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].CalculateDaysUntilDeletion();
				LanguageFormatIntString(szNum, arrsize(szNum), iRemainingDays);
				PStrCopy(szStringBuf, GlobalStringGet(GS_EMAIL_EXPIRES_SHORT), arrsize(szStringBuf));
				PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_NUMBER), szNum );
				UILabelSetText(pLabel, szStringBuf);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, "money icon")) != NULL)
			{
				const BOOL bVisible = (s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].AttachedMoney > 0);
				UIComponentSetActive(pLabel, bVisible);
				UIComponentSetVisible(pLabel, bVisible);
			}

			if ((pLabel = UIComponentFindChildByName(pMessageListItem, "attachment icon")) != NULL)
			{
				const BOOL bVisible = (s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].AttachedItemId != INVALID_GUID);
				UIComponentSetActive(pLabel, bVisible);
				UIComponentSetVisible(pLabel, bVisible);
			}

			if (pHilightBar && s_pSelectedEmail == &s_PLAYER_EMAIL_MESSAGEs[iMsgIdx])
			{
				UIComponentSetActive(pHilightBar, TRUE);
				pHilightBar->m_Position = pMessageListItem->m_Position;
				//trace("new y pos = %f\n", pHilightBar->m_Position.m_fY);
				bHilightVisible = TRUE;
			}

			pMessageListItem->m_dwData = (DWORD)iMsgIdx;
			pMessageListItem->m_qwData = (QWORD)s_PLAYER_EMAIL_MESSAGEs[iMsgIdx].EmailMessageId;

			UIComponentActivate(pMessageListItem, TRUE);
			UIComponentSetVisible(pMessageListItem, TRUE);
		}
		else
		{
			pMessageListItem->m_dwData = (DWORD)INVALID_LINK;
			pMessageListItem->m_qwData = (QWORD)INVALID_GUID;

			UIComponentSetVisible(pMessageListItem, FALSE);
		}

		++iMsgIdx;
	}

	if (pHilightBar)
	{
		UIComponentSetVisible(pHilightBar, bHilightVisible);
		UIComponentHandleUIMessage(pHilightBar, UIMSG_PAINT, 0, 0);
	}

	if (pScrollBar)
	{
		UIComponentActivate(pScrollBar, s_iMsgCount > iComponentCount);
		UIComponentSetVisible(pScrollBar, s_iMsgCount > iComponentCount);
		if (s_iMsgCount > iComponentCount)
		{
			UIComponentHandleUIMessage(pScrollBar, UIMSG_PAINT, 0, 0);
		}
	}

	return UIComponentOnPaint(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailMessageOnPaint(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentOnPaint(component, msg, wParam, lParam);

	UI_COMPONENT *pLabel, *pButton;
	WCHAR szStringBuf[100], szNum[32];
	UI_COMPONENT * pItemSlot = UIComponentFindChildByName(component, "email inbox slot");

	if (s_pSelectedEmail)
	{
		c_PlayerEmailGetMessage(s_pSelectedEmail->EmailMessageId, s_pSelectedEmail[0]);

		if (!s_pSelectedEmail->bIsMarkedRead)
		{
			c_PlayerEmailMarkMessageRead(s_pSelectedEmail->EmailMessageId);
		}

		UNIT * pPlayer = UIGetControlUnit();
		if (pItemSlot)
		{
			UIComponentSetActive(pItemSlot, pPlayer ? GameMailCanReceiveItem(pPlayer, NULL) : FALSE);
			UIComponentSetVisible(pItemSlot, s_pSelectedEmail->AttachedItemId != INVALID_GUID);
		}
	}
	else
	{
		if (pItemSlot)
		{
			UIComponentSetActive(pItemSlot, FALSE);
			UIComponentSetVisible(pItemSlot, FALSE);
		}
	}

	if (sEmailIsInReadMode())
	{
		static PGUID s_idPaintedEmail = INVALID_GUID;
		PGUID idSelectedEmail = s_pSelectedEmail ? s_pSelectedEmail->EmailMessageId : INVALID_GUID;

		if (s_idPaintedEmail != idSelectedEmail)
		{
			s_idPaintedEmail = idSelectedEmail;

			if ((pLabel = UIComponentFindChildByName(component, "message to label")) != NULL)
			{
				UILabelSetText(pLabel, s_pSelectedEmail ? gApp.m_wszPlayerName : L"");
			}

			if ((pLabel = UIComponentFindChildByName(component, "message from label")) != NULL)
			{
				if (s_pSelectedEmail) pLabel->m_dwColor = sUIEmailGetMailColor(s_pSelectedEmail->eMessageType, s_pSelectedEmail->eMessageStatus);
				UILabelSetText(pLabel, s_pSelectedEmail ? s_pSelectedEmail->wszSenderCharacterName : L"");
			}

			if ((pLabel = UIComponentFindChildByName(component, "message subject label")) != NULL)
			{
				if (s_pSelectedEmail) pLabel->m_dwColor = sUIEmailGetMailColor(s_pSelectedEmail->eMessageType, s_pSelectedEmail->eMessageStatus);

				WCHAR szSubject[MAX_EMAIL_SUBJECT];

				if (s_pSelectedEmail && sUIEmailMessageIsReturnedOrBounced(s_pSelectedEmail->eMessageStatus))
				{
					PStrCopy(szSubject, StringTableGetStringByKey("email_subject_mail_returned"), arrsize(szSubject));
					PStrReplaceToken( szSubject, arrsize(szSubject), StringReplacementTokensGet(SR_MESSAGE), s_pSelectedEmail->wszEmailSubject );
				}
				else
				{
					PStrCopy(szSubject, s_pSelectedEmail ? s_pSelectedEmail->wszEmailSubject : L"", arrsize(szSubject));
				}

				ChatFilterStringInPlace(szSubject);
				UILabelSetText(pLabel, szSubject);
			}

			if ((pLabel = UIComponentFindChildByName(component, "message body label")) != NULL)
			{
				WCHAR szBody[MAX_EMAIL_BODY];
				PStrCopy(szBody, s_pSelectedEmail ? s_pSelectedEmail->wszEmailBody : L"", arrsize(szBody));
				ChatFilterStringInPlace(szBody);
				UILabelSetText(pLabel, szBody);

				UI_COMPONENT *pScrollBar = UICastToLabel(pLabel)->m_pScrollbar;
				if (pScrollBar)
				{
					UIComponentHandleUIMessage(pScrollBar, UIMSG_PAINT, NULL, 0, 0);
				}
			}

			if ((pLabel = UIComponentFindChildByName(component, "message date label")) != NULL)
			{
				if (s_pSelectedEmail)
				{
					pLabel->m_dwColor = sUIEmailGetMailColor(s_pSelectedEmail->eMessageType, s_pSelectedEmail->eMessageStatus);
					tm Time;
					localtime_s(&Time, &s_pSelectedEmail->TimeSentUTC);
					ASSERT(wcsftime(szStringBuf, arrsize(szStringBuf), L"%x %X", &Time) > 0);
					UILabelSetText(pLabel, szStringBuf);
				}
				else
				{
					UILabelSetText(pLabel, L"");
				}
			}
		}

		if ((pLabel = UIComponentFindChildByName(component, "message money label")) != NULL)
		{
			if (s_pSelectedEmail && s_pSelectedEmail->AttachedMoney)
			{
				pLabel->m_dwColor = sUIEmailGetMailColor(s_pSelectedEmail->eMessageType, s_pSelectedEmail->eMessageStatus);
				LanguageFormatIntString(szNum, arrsize(szNum), s_pSelectedEmail->AttachedMoney);
				UILabelSetText(pLabel, szNum);
			}
			else
			{
				UILabelSetText(pLabel, L"");
			}
		}

		UI_COMPONENT *pMoneyIcon;
		if ((pMoneyIcon = UIComponentFindChildByName(component, "money icon")) != NULL)
		{
			const BOOL bVisible = s_pSelectedEmail && s_pSelectedEmail->AttachedMoney;
			UIComponentSetActive(pMoneyIcon, bVisible);
			UIComponentSetVisible(pMoneyIcon, bVisible);
		}

		if ((pLabel = UIComponentFindChildByName(component, "message expires label")) != NULL)
		{
			if (s_pSelectedEmail)
			{
				int iRemainingDays = s_pSelectedEmail->CalculateDaysUntilDeletion();
				LanguageFormatIntString(szNum, arrsize(szNum), iRemainingDays);
				PStrCopy(szStringBuf, GlobalStringGet(GS_EMAIL_EXPIRES_LONG), arrsize(szStringBuf));
				PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_NUMBER), szNum );
				UILabelSetText(pLabel, szStringBuf);
			}
			else
			{
				UILabelSetText(pLabel, L"");
			}
		}

		UIEmailUpdateComposeButtonState(); // this is to set the "reply" button state

		if ((pButton = UIComponentFindChildByName(component, "email delete button")) != NULL)
		{
		
			UIComponentSetActive(pButton, s_pSelectedEmail != NULL);
		}
	}
	else if (sEmailIsInComposeMode())
	{
		UNIT * pPlayer = UIGetControlUnit();
		if (pPlayer)
		{
			const BOOL bAllowAttachments = (GameMailCanSendItem(pPlayer, NULL) && s_nMailType == PLAYER_EMAIL_TYPE_PLAYER);
			const BOOL bAllowMoney = (s_nMailType == PLAYER_EMAIL_TYPE_PLAYER) && (!PlayerIsHardcoreDead(pPlayer));

			UI_COMPONENT * pEdit;
			if ((pEdit = UIComponentFindChildByName(component, "email outbox slot")) != NULL)
			{
				UIComponentSetActive(pEdit, bAllowAttachments);
				UIComponentSetVisible(pEdit, bAllowAttachments);
			}
			if (!bAllowAttachments && s_nOutboxItem != INVALID_GUID)
			{
				sUIEmailClearOutboxItem();
			}

			if ((pEdit = UIComponentFindChildByName(component, "message money editctrl")) != NULL)
			{
				UIComponentSetActive(pEdit, bAllowMoney);
				UIComponentSetVisible(pEdit, bAllowMoney);
				if (bAllowMoney)
				{
					sUIEmailCapMoney(pEdit);
				}
			}
			if ((pEdit = UIComponentFindChildByName(component, "text field money")) != NULL)
			{
				UIComponentSetActive(pEdit, bAllowMoney);
				UIComponentSetVisible(pEdit, bAllowMoney);
			}
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailRepaint(
	void)
{
	UI_COMPONENT * pBodyPanel = UIComponentGetByEnum(UICOMP_EMAIL_BODY_PANEL);
	if (pBodyPanel)
	{
		UIComponentHandleUIMessage(pBodyPanel, UIMSG_PAINT, 0, 0);
	}

	UI_COMPONENT * pEmailPanel = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_EMAIL_PANEL) : UIComponentGetByEnum(UICOMP_PANEMENU_MAIL );
	if (pEmailPanel)
	{
		UIComponentHandleUIMessage(pEmailPanel, UIMSG_PAINT, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailScrollBarOnScroll(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (component)
	{
		UIScrollBarOnScroll(component, msg, wParam, lParam);

		UI_COMPONENT *pEmailPanel = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_EMAIL_PANEL) : UIComponentGetByEnum(UICOMP_PANEMENU_MAIL );
		if (pEmailPanel)
		{
			UIComponentHandleUIMessage(pEmailPanel, UIMSG_PAINT, 0, 0);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailListItemOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!sEmailIsInReadMode())
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	const int iMsgIdx = (int)component->m_dwData;
	const PGUID nPLAYER_EMAIL_MESSAGEId = (PGUID)component->m_qwData;

	if (iMsgIdx != INVALID_LINK && nPLAYER_EMAIL_MESSAGEId != INVALID_GUID)
	{
		if (UnitHasBadge(UIGetControlUnit(), ACCT_TITLE_TRIAL_USER))
		{
			UIShowGenericDialog(GlobalStringGet(GS_EMAIL_EMAIL), GlobalStringGet(GS_TRIAL_PLAYERS_CANNOT_ACCESS));
		}
		else
		{
			sUIEmailSelectMessage(&s_PLAYER_EMAIL_MESSAGEs[iMsgIdx], iMsgIdx);
		}
	}
	else
	{
		sUIEmailSelectMessage(NULL, -1);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailMoneyOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (s_pSelectedEmail)
	{
		if (s_pSelectedEmail->AttachedMoney)
		{
			cCurrency playerCurrency = UnitGetCurrency(UIGetControlUnit());
			cCurrency messageCurrency(s_pSelectedEmail->AttachedMoney, 0);

			if (playerCurrency.CanCarry(AppGetCltGame(), messageCurrency) &&
				!PlayerIsHardcoreDead(UIGetControlUnit()))
			{
				c_PlayerEmailRemoveAttachedMoney(s_pSelectedEmail->EmailMessageId);
				UIEmailRepaint();

				int nMoneySound = GlobalIndexGet( GI_SOUND_UI_MONEY );
				if (nMoneySound != INVALID_LINK)
				{
					c_SoundPlay(nMoneySound, &c_SoundGetListenerPosition(), NULL);
				}
			}
			else
			{
				int nNoSound = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
				if (nNoSound != INVALID_LINK)
				{
					c_SoundPlay(nNoSound, &c_SoundGetListenerPosition(), NULL);
				}		
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackDeleteEmail( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	if (s_nSelectedDeletionEmailId != INVALID_GUID)
	{
		c_PlayerEmailDeleteMessage(s_nSelectedDeletionEmailId);

		sUIEmailSelectMessage(NULL, -1);
		sUIEmailRefreshMailList();
		s_nSelectedDeletionEmailId = INVALID_GUID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackDeleteEmailAttachments( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	if (s_pSelectedEmail)
	{
		DIALOG_CALLBACK tCallbackOK;
		DialogCallbackInit( tCallbackOK );
		tCallbackOK.pfnCallback = sCallbackDeleteEmail;

		UIShowGenericDialog(
			GlobalStringGet(GS_EMAIL_EMAIL),
			GlobalStringGet(GS_EMAIL_DELETE_CONFIRM_ATTACHMENTS),
			TRUE,
			&tCallbackOK);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateSendCostLabel(
	void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL);
	if (pComponent)
	{
		UI_COMPONENT *pLabel;
		if ((pLabel = UIComponentFindChildByName(pComponent, "message cost to send")) != NULL)
		{
			const int nSendCost = sUIEmailGetCostToSend();
			if (nSendCost > 0)
			{
				if( AppIsHellgate() )
				{
					WCHAR szNum[32];
					LanguageFormatIntString(szNum, arrsize(szNum), nSendCost);
					UILabelSetText(pLabel, szNum);
					UIComponentSetVisible(pLabel, TRUE);
				}
				else
				{
					cCurrency nCost( nSendCost, 0 );
					WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];		
					const WCHAR *uszFormat = GlobalStringGet(GS_PRICE_COPPER_SILVER_GOLD);
					if (uszFormat)
					{
						PStrCopy(uszTextBuffer, uszFormat, arrsize(uszTextBuffer));
					}
					const int MAX_MONEY_STRING = 128;
					WCHAR uszMoney[ MAX_MONEY_STRING ];
					PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
					// replace any money token with the money value
					const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
					PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

					PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
					// replace any money token with the money value
					puszGoldToken = StringReplacementTokensGet( SR_SILVER );
					PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

					PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
					// replace any money token with the money value
					puszGoldToken = StringReplacementTokensGet( SR_GOLD );
					PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

					UILabelSetText( pLabel, (nCost.IsZero() == FALSE)?uszTextBuffer:L"" );
				}
				
			}
			else
			{
				UIComponentSetVisible(pLabel, FALSE);
			}
		}
	}

	UIEmailUpdateComposeButtonState();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailGuildToggleOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIButtonGetDown(UICastToButton(component)))
	{
		s_nMailType = PLAYER_EMAIL_TYPE_GUILD;
	}
	else
	{
		s_nMailType = PLAYER_EMAIL_TYPE_PLAYER;
	}

	sUIEmailSetEditColor(s_nMailType, EMAIL_STATUS_ACCEPTED);

	sUIUpdateSendCostLabel();

	UI_COMPONENT *pComposePanel;
	if ((pComposePanel = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL)) != NULL)
	{
		UI_COMPONENT *pComponent;
		BOOL bVisible;

		if ((pComponent = UIComponentFindChildByName(pComposePanel, "message to editctrl")) != NULL)
		{
			bVisible = (s_nMailType != PLAYER_EMAIL_TYPE_GUILD);
			if (!bVisible && UICastToEditCtrl(pComponent)->m_bHasFocus)
			{
				UI_COMPONENT *pSubjectEditCtrl = UIComponentFindChildByName(pComposePanel, "message subject editctrl");
				if (pSubjectEditCtrl)
				{
					UIEditCtrlSetFocus(UICastToEditCtrl(pSubjectEditCtrl));
				}
			}
			UIComponentSetVisible(pComponent, bVisible);
			UIComponentSetActive(pComponent, bVisible);
			if (bVisible)
			{
				UIEditCtrlSetFocus(UICastToEditCtrl(pComponent));
			}
		}
		if ((pComponent = UIComponentFindChildByName(pComposePanel, "message to guild label")) != NULL)
		{
			bVisible = (s_nMailType == PLAYER_EMAIL_TYPE_GUILD);
			UIComponentSetVisible(pComponent, bVisible);
			UIComponentSetActive(pComponent, bVisible);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sComposeResticted(
	MAIL_CAN_COMPOSE_RESULT eResult)
{
	
	switch (eResult)
	{
		
		//----------------------------------------------------------------------------
		case MCCR_FAILED_GAGGED:
		{
			c_GagDisplayGaggedMessage();
			break;
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailEnterComposeMode(
	WCHAR *szRecipient /*= NULL*/,
	WCHAR *szSubject /*= NULL*/,
	BOOL bPopulateFields /*= TRUE*/)
{
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// check for not allowed
	MAIL_CAN_COMPOSE_RESULT eResult = GameMailCanComposeMessage( pPlayer );
	if (eResult != MCCR_OK)
	{
		sComposeResticted( eResult );
		return;
	}
		
	sUIEmailSelectMessage(NULL, -1);

	UI_COMPONENT *pComponent;

	if ((pComponent = UIComponentGetByEnum(UICOMP_EMAIL_VIEW_PANEL)) != NULL)
	{
		UIComponentSetVisible(pComponent, FALSE);
		UIComponentSetActive(pComponent, FALSE);
	}

	if ((pComponent = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL)) != NULL)
	{
		UIComponentSetActive(pComponent, TRUE);
		UIComponentSetVisible(pComponent, TRUE);
	}

	if (bPopulateFields)
	{
		s_nMailType = PLAYER_EMAIL_TYPE_PLAYER;

		sUIEmailSetEditColor(s_nMailType, EMAIL_STATUS_ACCEPTED);

		UI_COMPONENT *pGuildMailButton;
		if ((pGuildMailButton = UIComponentFindChildByName(pComponent, "send to guild toggle")) != NULL)
		{
			UIButtonSetDown(UICastToButton(pGuildMailButton), FALSE);
			UIEmailGuildToggleOnClick(pGuildMailButton, NULL, 0, 0);

			const WCHAR * wszGuildName = c_PlayerGetGuildName();
			if (wszGuildName && wszGuildName[0] && c_PlayerCanDoGuildAction(GUILD_ACTION_EMAIL_GUILD))
			{
				UI_COMPONENT * pLabel;
				if ((pLabel = UIComponentFindChildByName(pComponent, "message to guild label")) != NULL)
				{
					UILabelSetText(pLabel, wszGuildName);
				}
				UIComponentSetActive(pGuildMailButton, TRUE);
			}
			else
			{
				UIComponentSetActive(pGuildMailButton, FALSE);
			}
			UIComponentHandleUIMessage(pGuildMailButton, UIMSG_PAINT, 0, 0);
		}

		UI_COMPONENT * pEditCtrl;
		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message to editctrl")) != NULL)
		{
			UILabelSetText(UICastToLabel(pEditCtrl), szRecipient ? szRecipient : L"");
			if (!szRecipient || !szRecipient[0])
			{
				UIEditCtrlSetFocus(UICastToEditCtrl(pEditCtrl));
			}
		}

		UI_COMPONENT * pBodyPanel = UIComponentGetByEnum(UICOMP_EMAIL_BODY_PANEL);
		if (pBodyPanel)
		{
			UI_COMPONENT * pLabel;

			if ((pLabel = UIComponentFindChildByName(pBodyPanel, "message from label")) != NULL)
			{
				WCHAR uszPlayerName[ MAX_CHARACTER_NAME ] = L"";
				UnitGetName( UIGetControlUnit(), uszPlayerName, arrsize(uszPlayerName), 0 );
				UILabelSetText(pLabel, uszPlayerName);
			}

			if ((pLabel = UIComponentFindChildByName(pBodyPanel, "message date label")) != NULL)
			{
				WCHAR szTime[32] = L"";
				tm Time;
				time_t ltime;
				time( &ltime );
				localtime_s(&Time, &ltime);
				ASSERT(wcsftime(szTime, arrsize(szTime), L"%x %X", &Time) > 0);
				UILabelSetText(pLabel, szTime);
			}
		}

		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message subject editctrl")) != NULL)
		{
			UICastToEditCtrl(pEditCtrl)->m_nMaxLen = MAX_EMAIL_SUBJECT;
			UILabelSetText(UICastToLabel(pEditCtrl), szSubject ? szSubject : L"");
		}

		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message money editctrl")) != NULL)
		{
			UILabelSetText(UICastToLabel(pEditCtrl), L"");
			s_nAttachedMoney = 0;
		}

		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message body editctrl")) != NULL)
		{
			UI_EDITCTRL * pBodyEdit = UICastToEditCtrl(pEditCtrl);
			pBodyEdit->m_nMaxLen = MAX_EMAIL_BODY;
			UILabelSetText(UICastToLabel(pEditCtrl), L"");
			if (szRecipient && szRecipient[0])
			{
				UIEditCtrlSetFocus(pBodyEdit);
			}
		}

		sUIEmailClearOutboxItem();
	}

	sUIUpdateSendCostLabel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailEnterReadMode(
	void)
{
	UI_COMPONENT *pComponent;

	pComponent = UIComponentGetByEnum(UICOMP_EMAIL_BODY_PANEL);
	if (pComponent)
	{
		UI_COMPONENT *pLabel;
		if ((pLabel = UIComponentFindChildByName(pComponent, "message from label")) != NULL)
		{
			UILabelSetText(UICastToLabel(pLabel), L"");
		}
		if ((pLabel = UIComponentFindChildByName(pComponent, "message date label")) != NULL)
		{
			UILabelSetText(UICastToLabel(pLabel), L"");
		}
	}

	pComponent = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL);
	if (pComponent)
	{
		UIComponentSetVisible(pComponent, FALSE);
		UIComponentSetActive(pComponent, FALSE);
	}

	pComponent = UIComponentGetByEnum(UICOMP_EMAIL_VIEW_PANEL);
	if (pComponent)
	{
		UIComponentSetActive(pComponent, TRUE);
		UIComponentSetVisible(pComponent, TRUE);
	}

	UIEmailUpdateComposeButtonState();

	IMM_Enable(FALSE);

	UIEmailRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackCancelMessage( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	s_bAwaitingSendResult = FALSE;
	sUIEmailClearOutboxItem();
	UIEmailEnterReadMode();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailComposeBtnOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	UIEmailEnterComposeMode();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailReplyBtnOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	WCHAR szFromName[MAX_CHARACTER_NAME] = L"";
	WCHAR szSubject[MAX_EMAIL_SUBJECT] = L"";

	UI_COMPONENT * pBodyPanel = UIComponentGetByEnum(UICOMP_EMAIL_BODY_PANEL);
	if (pBodyPanel)
	{
		UI_COMPONENT * pLabel;

		if ((pLabel = UIComponentFindChildByName(pBodyPanel, "message from label")) != NULL)
		{
			PStrCopy(szFromName, UILabelGetText(pLabel), arrsize(szFromName));
		}

		if ((pLabel = UIComponentFindChildByName(pBodyPanel, "message subject label")) != NULL)
		{
			PStrCopy(szSubject, StringTableGetStringByKey("email_subject_reply"), arrsize(szSubject));
			PStrReplaceToken( szSubject, arrsize(szSubject), StringReplacementTokensGet(SR_MESSAGE), UILabelGetText(pLabel) );
			ChatFilterStringInPlace(szSubject);
		}
	}

	UIEmailEnterComposeMode(szFromName, szSubject);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailDeleteBtnOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!s_pSelectedEmail)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	s_nSelectedDeletionEmailId = s_pSelectedEmail->EmailMessageId;
	DIALOG_CALLBACK tCallbackOK;
	DialogCallbackInit( tCallbackOK );

	if (s_pSelectedEmail->AttachedMoney > 0 ||
		s_pSelectedEmail->AttachedItemId != INVALID_GUID)
	{
		tCallbackOK.pfnCallback = sCallbackDeleteEmailAttachments;
	}
	else
	{
		tCallbackOK.pfnCallback = sCallbackDeleteEmail;
	}

	UIShowGenericDialog(
		GlobalStringGet(GS_EMAIL_EMAIL),
		GlobalStringGet(GS_EMAIL_DELETE_CONFIRM),
		TRUE,
		&tCallbackOK);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailCancelBtnOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	DIALOG_CALLBACK tCallbackOK;
	DialogCallbackInit( tCallbackOK );
	tCallbackOK.pfnCallback = sCallbackCancelMessage;

	UIShowGenericDialog(
		GlobalStringGet(GS_EMAIL_EMAIL),
		GlobalStringGet(GS_EMAIL_CANCEL_CONFIRM),
		TRUE,
		&tCallbackOK);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailSendBtnOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	const WCHAR * szTarget = NULL;
	const WCHAR * szSubject = NULL;
	const WCHAR * szBody = NULL;
	int iAttachedMoney = 0;

	UI_COMPONENT *pComponent;
	if ((pComponent = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL)) != NULL)
	{
		UI_COMPONENT * pEditCtrl;
		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message to editctrl")) != NULL)
		{
			szTarget = UILabelGetText(UICastToLabel(pEditCtrl));
		}
		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message subject editctrl")) != NULL)
		{
			szSubject = UILabelGetText(UICastToLabel(pEditCtrl));
		}
		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message body editctrl")) != NULL)
		{
			szBody = UILabelGetText(UICastToLabel(pEditCtrl));
		}
		if ((pEditCtrl = UIComponentFindChildByName(pComponent, "message money editctrl")) != NULL)
		{
			const WCHAR *wszMoney = UILabelGetText(UICastToLabel(pEditCtrl));
			if (wszMoney)
			{
				PStrToInt(wszMoney, iAttachedMoney);
			}
		}
	}

	BOOL bRes = FALSE;

	switch (s_nMailType)
	{
		case PLAYER_EMAIL_TYPE_GUILD:
			bRes = c_PlayerEmailSendGuildMessage(szSubject, szBody);
			break;
		default:
			bRes = c_PlayerEmailSendCharacterMessage(szTarget, szSubject, szBody, s_nOutboxItem, (DWORD)iAttachedMoney);
			break;
	};

	if (bRes)
	{
		s_bAwaitingSendResult = TRUE;

		UI_COMPONENT *pComposePanel = UIComponentGetByEnum(UICOMP_EMAIL_COMPOSE_PANEL);
		if (pComposePanel)
		{
			UIEmailUpdateComposeButtonState();
		}
	}
	else
	{
		UIShowGenericDialog(
			GlobalStringGet(GS_EMAIL_EMAIL),
			GlobalStringGet(GS_EMAIL_MAIL_FAILED));
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailScrollUp(
	void)
{
	if (s_iSelectedEmailIdx > 0)
	{
		--s_iSelectedEmailIdx;

		sUIEmailSelectMessage(&s_PLAYER_EMAIL_MESSAGEs[s_iSelectedEmailIdx], s_iSelectedEmailIdx);

		UI_COMPONENT *pMessageListPanel = UIComponentGetByEnum(UICOMP_EMAIL_MESSAGE_LIST);
		if (pMessageListPanel)
		{
			UI_COMPONENT *pHilightBar = UIComponentFindChildByName(pMessageListPanel, "email message hilight bar");
			if (!UIComponentGetVisible(pHilightBar))
			{
				UI_SCROLLBAR *pScrollBar = UICastToScrollBar(UIComponentIterateChildren(pMessageListPanel, NULL, UITYPE_SCROLLBAR, FALSE));
				UIScrollBarSetValue(pScrollBar, UIScrollBarGetValue(pScrollBar) - 1.0f);
				UIEmailRepaint();
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailScrollDown(
	void)
{
	if (s_iSelectedEmailIdx+1 < s_iMsgCount)
	{
		++s_iSelectedEmailIdx;

		sUIEmailSelectMessage(&s_PLAYER_EMAIL_MESSAGEs[s_iSelectedEmailIdx], s_iSelectedEmailIdx);

		UI_COMPONENT *pMessageListPanel = UIComponentGetByEnum(UICOMP_EMAIL_MESSAGE_LIST);
		if (pMessageListPanel)
		{
			UI_COMPONENT *pHilightBar = UIComponentFindChildByName(pMessageListPanel, "email message hilight bar");
			if (!UIComponentGetVisible(pHilightBar))
			{
				UI_SCROLLBAR *pScrollBar = UICastToScrollBar(UIComponentIterateChildren(pMessageListPanel, NULL, UITYPE_SCROLLBAR, FALSE));
				if (s_iSelectedEmailIdx > 0)
				{
					UIScrollBarSetValue(pScrollBar, UIScrollBarGetValue(pScrollBar) + 1.0f);
				}
				else
				{
					UIScrollBarSetValue(pScrollBar, 0.0f);
				}
				UIEmailRepaint();
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailSortAscDscOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX *pButton = UICastToButton(component);
	s_bSortDescending = UIButtonGetDown(pButton);

	s_bListNeedsSort = TRUE;
	
	sUIEmailRefreshMailList();
	UIEmailRepaint();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEmailUpdateSortLabel(
	void)
{
	s_bSortDescending = s_bSortAscDsc[s_eEmailSort];

	UI_COMPONENT *pComponent = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_EMAIL_PANEL) : UIComponentGetByEnum(UICOMP_PANEMENU_MAIL );
	if (pComponent)
	{
		UI_COMPONENT *pLabelComponent = UIComponentFindChildByName(pComponent, "email sort label");
		if (pLabelComponent)
		{
			UI_LABELEX *pLabel = UICastToLabel(pLabelComponent);
			UILabelSetText(pLabel, StringTableGetStringByKey(s_szSortText[s_eEmailSort]));
		}

		UI_COMPONENT *pButton = UIComponentFindChildByName(pComponent, "email sort asc dsc");
		if (pButton)
		{
			UIButtonSetDown(UICastToButton(pButton), s_bSortDescending);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
		}
	}

	sUIEmailRefreshMailList();
	UIEmailRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailSortLabelOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	s_eEmailSort = (EMAIL_SORT_ENUM)((int)s_eEmailSort + 1);
	if (s_eEmailSort == EMAIL_SORT_COUNT)
	{
		s_eEmailSort = (EMAIL_SORT_ENUM)0;
	}

	s_bListNeedsSort = TRUE;

	UIEmailUpdateSortLabel();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailInboxSlotOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	int location = (int)lParam;

	if (location == GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR) &&
		s_pSelectedEmail && s_pSelectedEmail->AttachedItemId != INVALID_GUID && 		
		UIComponentGetVisibleByEnum(UICOMP_EMAIL_VIEW_PANEL))
	{
		UNITID unitid = (UNITID)wParam;
		UNIT * pContainer = UnitGetById(AppGetCltGame(), unitid);
		ASSERT_RETVAL(pContainer, UIMSG_RET_NOT_HANDLED);

		UNIT * pItem = UnitGetByGuid(AppGetCltGame(), s_pSelectedEmail->AttachedItemId);
		if (pItem && UnitInventoryGetByLocationAndUnitId(pContainer, location, pItem->unitid))
		{
			c_PlayerEmailClearItemAttachment(s_pSelectedEmail->EmailMessageId);
		}
	}

	return UIInvSlotOnInventoryChange(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailOutboxSlotOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_INVSLOT *pInvSlot = UICastToInvSlot(component);
	int location = (int)lParam;

	if (location == pInvSlot->m_nInvLocation)
	{
		UNITID unitid = (UNITID)wParam;
		UNIT * pContainer = UnitGetById(AppGetCltGame(), unitid);
		ASSERT_RETVAL(pContainer, UIMSG_RET_NOT_HANDLED);

		UNIT * pUnit = UnitInventoryGetByLocation(pContainer, location);
		if (pUnit)
		{
			s_nOutboxItem = UnitGetGuid(pUnit);
		}
		else
		{
			s_nOutboxItem = INVALID_GUID;
		}

		sUIUpdateSendCostLabel();
	}

	return UIInvSlotOnInventoryChange(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmailMoneyOnChar(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ))
	{
		WCHAR ucCharacter = (WCHAR)wParam;
		if (UIEditCtrlOnKeyChar( pComponent, nMessage, wParam, lParam ) != UIMSG_RET_NOT_HANDLED)
		{
			if (ucCharacter != VK_RETURN)
			{
				sUIEmailCapMoney(pComponent);
				sUIUpdateSendCostLabel();
			}

			return UIMSG_RET_HANDLED_END_PROCESS;  // input used
		}
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesEmail(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name							function pointer
		{ "UIToggleEmail",							UIToggleEmail },
		{ "UIEmailOnActivate",						UIEmailOnActivate },
		{ "UIEmailOnPostActivate",					UIEmailOnPostActivate },
		{ "UIEmailOnPostInactivate",				UIEmailOnPostInactivate },
		{ "UIEmailListOnPaint",						UIEmailListOnPaint },
		{ "UIEmailMessageOnPaint",					UIEmailMessageOnPaint },
		{ "UIEmailScrollBarOnScroll",				UIEmailScrollBarOnScroll },
		{ "UIEmailListItemOnClick",					UIEmailListItemOnClick },
		{ "UIEmailComposeBtnOnClick",				UIEmailComposeBtnOnClick },
		{ "UIEmailReplyBtnOnClick",					UIEmailReplyBtnOnClick },
		{ "UIEmailDeleteBtnOnClick",				UIEmailDeleteBtnOnClick },
		{ "UIEmailCancelBtnOnClick",				UIEmailCancelBtnOnClick },
		{ "UIEmailSendBtnOnClick",					UIEmailSendBtnOnClick },
		{ "UIEmailSortAscDscOnClick",				UIEmailSortAscDscOnClick },
		{ "UIEmailSortLabelOnClick",				UIEmailSortLabelOnClick },
		{ "UIEmailGuildToggleOnClick",				UIEmailGuildToggleOnClick },
		{ "UIEmailInboxSlotOnInventoryChange",		UIEmailInboxSlotOnInventoryChange },
		{ "UIEmailOutboxSlotOnInventoryChange",		UIEmailOutboxSlotOnInventoryChange },
		{ "UIEmailMoneyOnClick",					UIEmailMoneyOnClick },
		{ "UIEmailMoneyOnChar",						UIEmailMoneyOnChar }
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif
