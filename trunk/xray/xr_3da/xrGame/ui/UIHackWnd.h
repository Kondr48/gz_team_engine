#pragma once

#include "UIWindow.h"
#include "UIWndCallback.h"
#include "../inventory_space.h"

class CUIFrameWindow;
class CUIFrameLineWnd;
class CUIStatic;
class CUIAnimatedStatic;
class CUIScrollView;
class CUIDragDropListEx;
class CUICellItem;
class CUI3tButton;

class CUIHackWnd: public CUIWindow, public CUIWndCallback
{
	typedef CUIWindow inherited;
	
protected:
 CUIFrameWindow*			UILeftFrame;
 CUIFrameWindow*			UIRightFrame;
 CUIFrameLineWnd*			UIDeviceListHeader;
 CUIAnimatedStatic*			UIDeviceListAnimation;
 CUIDragDropListEx*         UiActorDevivesList;
 CUIStatic*			        m_device_soft_version;
 CUIStatic*			        m_device_name;
 CUIStatic*			        m_device_icon;
 CUIStatic*			        m_device_encryption_level;
 CUIStatic*			        m_device_hack_time;
 CUIStatic*			        m_hack_console;
 CUI3tButton*               m_hacked_start_button;
 CUI3tButton*               m_hacked_stop_button;

 shared_str		            hacked_device_name;
 shared_str		            hacked_encryption_level;

public:					    CUIHackWnd			   ();
	virtual				    ~CUIHackWnd			   ();
    virtual void		    Draw				   ();
	void		            Init				   ();
	virtual void		    ClearRightWindow	   ();
    CInventory*			    m_pRuck;
	CUICellItem*            m_hacked_pda;
	void					BindDragDropListEnents		(CUIDragDropListEx* lst);
	void					ShowRightWindow     		(LPCSTR section);

	virtual void		    SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);
	 
	s32                        soft_ver;
	string128                  text_name;
	string128                  text_encryption_level;
	string128                  text_hack_time;
	string128                  text_soft_version;
    LPCSTR                     item_section;
	LPCSTR                     hacked_device;
	float                      hack_time;
	
	bool	xr_stdcall		OnItemDbClick			    (CUICellItem* itm);
	bool	xr_stdcall		OnItemSelected			    (CUICellItem* itm);
};

