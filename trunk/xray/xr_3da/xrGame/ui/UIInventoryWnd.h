#pragma once

class CInventory;
#include "UIScriptWnd.h"
#include "UIStatic.h"

#include "UIProgressBar.h"

#include "UIPropertiesBox.h"
#include "UIOutfitSlot.h"

#include "UIOutfitInfo.h"
#include "UIItemInfo.h"

#include "../inventory_space.h"

class CArtefact;
class CUI3tButton;
class CUIDragDropListEx;
class CUICellItem;

class CUIInventoryWnd: public CUIDialogWnd
{
private:
	typedef CUIDialogWnd	inherited;
	bool					m_b_need_reinit;
public:
							CUIInventoryWnd				();
	virtual					~CUIInventoryWnd			();

	virtual void			Init						();

	void					InitInventory				();
	void					InitInventory_delayed		();
	virtual bool			StopAnyMove					()					{return false;}

	virtual void			SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);
	virtual bool			OnMouse						(float x, float y, EUIMessages mouse_action);
	virtual bool			OnKeyboard					(int dik, EUIMessages keyboard_action);


	IC CInventory*			GetInventory				()					{return m_pInv;}

	virtual void			Update						();
	void					UpdateConditionProgressBars	();
	void                    UpdateOutfit();
    void                    MoveArtefactsToBag();
	virtual void			Draw						();

	virtual void			Show						();
	virtual void			Hide						();

	void					AddItemToBag				(PIItem pItem);
	
protected:
	enum eInventorySndAction{	eInvSndOpen	=0,
								eInvSndClose,
								eInvItemToSlot,
								eInvItemToBelt,
								eInvItemToRuck,
								eInvProperties,
								eInvDropItem,
								eInvAttachAddon,
								eInvDetachAddon,
								eInvItemUse,
								eInvSndMax};

	ref_sound					sounds					[eInvSndMax];
	void						PlaySnd					(eInventorySndAction a);

	CUIStatic					UIBack;
	CUIStatic*					UIRankFrame;
	CUIStatic*					UIRank;

	CUIStatic					UIBagWnd;
	CUIStatic					UIMoneyWnd;
	CUIStatic					UIDescrWnd;

	CUIFrameWindow				UIPersonalWnd;

	CUI3tButton*				UIExitButton;

	CUIStatic					UIStaticBottom;
	CUIStatic					UIStaticTime;
	CUIStatic					UIStaticTimeString;

	CUIStatic					UIStaticPersonal;
		
	CUIDragDropListEx*			m_pUIBagList;
	CUIDragDropListEx*			m_pUIBeltList;
	CUIDragDropListEx*			m_pUIPistolList;
	CUIDragDropListEx*			m_pUIAutomaticList;

	CUIProgressBar*				m_WeaponSlot1_progress;
	CUIProgressBar*				m_WeaponSlot2_progress;
	CUIProgressBar*				m_Helmet_progress;
	CUIProgressBar*				m_Outfit_progress;
	CUIDragDropListEx*			m_pUIKnifeList;
	CUIDragDropListEx*			m_pUIBinocularList;
	CUIDragDropListEx*			m_pUIDetectorOneList;
	CUIDragDropListEx*			m_pUIDetectorTwoList;
	CUIDragDropListEx*			m_pUITorchList;
	CUIDragDropListEx*			m_pUIPDAList;
	CUIDragDropListEx*			m_pUIHelmetList;
	CUIDragDropListEx*			m_pUISlotQuickAccessList_0;
	CUIDragDropListEx*			m_pUISlotQuickAccessList_1;
	CUIDragDropListEx*			m_pUISlotQuickAccessList_2;
	CUIDragDropListEx*			m_pUISlotQuickAccessList_3;
	CUIDragDropListEx*			m_pUINightvisionList;
	CUIProgressBar				UIProgressBarSatiety;
	CUIProgressBar				UIProgressBarThirst;
	CUIDragDropListEx*			m_slots_array [SLOTS_TOTAL];  // alpet: для индексированного доступа

	CUIStatic* m_InvSlot0Highlight;
    CUIStatic* m_InvSlot1Highlight;
    CUIStatic* m_InvSlot2Highlight;
    CUIStatic* m_InvSlot4Highlight;
	CUIStatic* m_InvSlot6Highlight;
	CUIStatic* m_InvSlot7Highlight;
	CUIStatic* m_InvSlot8Highlight;
	CUIStatic* m_InvSlot9Highlight;
	CUIStatic* m_InvSlot11Highlight;
	CUIStatic* m_InvSlot12Highlight;
	CUIStatic* m_InvSlot13Highlight;
	CUIStatic* m_InvSlot14Highlight;
	CUIStatic* m_InvSlot15Highlight;
	CUIStatic* m_InvSlot16Highlight;
	CUIStatic* m_InvSlot17Highlight;

	CUIStatic* m_HelmetOver;
	CUIStatic* m_NightvisionOver;

	enum
    {
        e_af_count = 10
    };

    CUIStatic* m_belt_list_over[e_af_count];
	//CUIStatic* m_belt_highlight[e_af_count];

	bool m_highlight_clear;

private:
	void clear_highlight_lists();
	void set_highlight_item(CUICellItem* cell_item);
	void highlight_item_slot(CUICellItem* cell_item);

public:

#if defined(INV_OUTFIT_FULL_ICON_HIDE)
	CUIDragDropListEx*		m_pUIOutfitList;
#else
	CUIOutfitDragDropList*		m_pUIOutfitList;
#endif
	
	void						ClearAllLists				();
	void						BindDragDropListEnents		(CUIDragDropListEx* lst);

	EListType					GetType						(CUIDragDropListEx* l);
	CUIDragDropListEx*			GetSlotList					(u32 slot_idx);

	bool		xr_stdcall		OnItemDrop					(CUICellItem* itm);
	bool		xr_stdcall		OnItemStartDrag				(CUICellItem* itm);
	bool		xr_stdcall		OnItemDbClick				(CUICellItem* itm);
	bool		xr_stdcall		OnItemSelected				(CUICellItem* itm);
	bool		xr_stdcall		OnItemRButtonClick			(CUICellItem* itm);
	bool        xr_stdcall      OnItemFocusReceive          (CUICellItem* itm);
    bool        xr_stdcall      OnItemFocusLost             (CUICellItem* itm);
    bool        xr_stdcall      OnItemFocusedUpdate         (CUICellItem* itm);

	CUIStatic					UIProgressBack;
	CUIProgressBar				UIProgressBarHealth;	
	CUIProgressBar				UIProgressBarPsyHealth;
	CUIProgressBar				UIProgressBarRadiation;
	CUIProgressBar				UIProgressBarBleeding;

	CUIPropertiesBox			UIPropertiesBox;
	
	//информация о персонаже
	CUIOutfitInfo				UIOutfitInfo;
	CUIItemInfo					UIItemInfo;

	CInventory*					m_pInv;

	CUICellItem*				m_pCurrentCellItem;

	bool						DropItem					(PIItem itm, CUIDragDropListEx* lst);
	bool						TryUseItem					(PIItem itm);
	//----------------------	-----------------------------------------------
	void						SendEvent_Item2Slot			(PIItem	pItem);
	void						SendEvent_Item2Belt			(PIItem	pItem);
	void						SendEvent_Item2Ruck			(PIItem	pItem);
	void						SendEvent_Item_Drop			(PIItem	pItem);
	void						SendEvent_Item_Eat			(PIItem	pItem);
	void						SendEvent_ActivateSlot		(PIItem	pItem);

	//---------------------------------------------------------------------

	void						ProcessPropertiesBoxClicked	();
	void						ActivatePropertiesBox		();

	void						DropCurrentItem				(bool b_all);
	void						EatItem						(PIItem itm);
	
	bool						ToSlot						(CUICellItem* itm, bool force_place);
	bool						ToBag						(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToBelt						(CUICellItem* itm, bool b_use_cursor_pos);


	void						AttachAddon					(PIItem item_to_upgrade);
	void						DetachAddon					(const char* addon_name);

	void						SetCurrentItem				(CUICellItem* itm);
	CUICellItem*				CurrentItem					();
	

	TIItemContainer				ruck_list;
	u32							m_iCurrentActiveSlot;
public:
	PIItem						CurrentIItem();
};

extern bool is_quick_slot(u32, CInventoryItem*, CInventory*);
