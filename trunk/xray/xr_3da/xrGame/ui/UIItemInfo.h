#pragma once
#include "uiwindow.h"


class CInventoryItem;
class CUIStatic;
class CUIScrollView;
class CUIProgressBar;
class CUIWpnParams;
class CUIArtefactParams;
class CUIFrameWindow;

extern const char * const 		fieldsCaptionColor;

class CUIItemInfo: public CUIWindow
{
private:
	typedef CUIWindow inherited;
	struct _desc_info
	{
		CGameFont*			pDescFont;
		u32					uDescClr;
		bool				bShowDescrText;
	};
	_desc_info				m_desc_info;
	CInventoryItem* m_pInvItem;
public:
						CUIItemInfo			 ();
	virtual				~CUIItemInfo		 ();
	void				Init				 (float x, float y, float width, float height, LPCSTR xml_name);
	void				Init				 (LPCSTR xml_name);
	void				InitItem			 (CInventoryItem* pInvItem);
	void				TryAddWpnInfo		 (const shared_str& wpn_section);
	void				TryAddArtefactInfo	 (const shared_str& af_section);
	void				TryAddPowerLevelInfo (float power_level);

	virtual void		Draw				 ();
	
    u32                 delay;
	u32                 GetPercentColor;

	bool				m_b_force_drawing;
	bool                m_complex_desc;
	bool                m_b_FitToHeight;

	CUIFrameWindow*     UIBackground;
	CUIStatic*			UIName;
	CUIStatic*			UIWeight;
	CUIStatic*			UICost;
	CUIScrollView*		UIDesc;
	CUIWpnParams*		UIWpnParams;
	CUIArtefactParams*	UIArtefactParams;
	
	Fvector2			UIItemImageSize; 
	CUIStatic*			UIItemImage;
};