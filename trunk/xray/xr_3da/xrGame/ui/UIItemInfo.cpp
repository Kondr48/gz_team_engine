#include "stdafx.h"

#include "uiiteminfo.h"
#include "uistatic.h"
#include "UIXmlInit.h"

#include "UIListWnd.h"
#include "UIProgressBar.h"
#include "UIScrollView.h"

#include "../string_table.h"
#include "../Inventory_Item.h"
#include "UIInventoryUtilities.h"
#include "../PhysicsShellHolder.h"
#include "UIWpnParams.h"
#include "ui_af_params.h"

#include "UIFrameWindow.h"

#include "../Torch.h"
#include "../HandTorch.h"

#define INV_GRID_WIDTH2 40.0f
#define INV_GRID_HEIGHT2 40.0f

CUIItemInfo::CUIItemInfo()
{
	UIItemImageSize.set			(0.0f,0.0f);
	UICost						= NULL;
	UIWeight					= NULL;
	UIItemImage					= NULL;
	UIDesc						= NULL;
	UIWpnParams					= NULL;
	UIArtefactParams			= NULL;
	UIName						= NULL;
	UIBackground                = NULL;
	m_pInvItem					= NULL;
	m_b_force_drawing			= false;
	m_b_FitToHeight             = false;
	m_complex_desc              = false;
}

CUIItemInfo::~CUIItemInfo()
{
	xr_delete					(UIWpnParams);
	xr_delete					(UIArtefactParams);
}

void CUIItemInfo::Init(LPCSTR xml_name){

	CUIXml						uiXml;
	bool xml_result				= uiXml.Init(CONFIG_PATH, UI_PATH, xml_name);
	R_ASSERT2					(xml_result, "xml file not found");

	CUIXmlInit					xml_init;

	if(uiXml.NavigateToNode("main_frame",0))
	{
		Frect wnd_rect;
		wnd_rect.x1		= uiXml.ReadAttribFlt("main_frame", 0, "x", 0);
		wnd_rect.y1		= uiXml.ReadAttribFlt("main_frame", 0, "y", 0);

		wnd_rect.x2		= uiXml.ReadAttribFlt("main_frame", 0, "width", 0);
		wnd_rect.y2		= uiXml.ReadAttribFlt("main_frame", 0, "height", 0);
		
		inherited::Init(wnd_rect.x1, wnd_rect.y1, wnd_rect.x2, wnd_rect.y2);
		delay = uiXml.ReadAttribInt("main_frame", 0, "delay", 500);
	}
	if (uiXml.NavigateToNode("background_frame", 0))
    {
        UIBackground = new CUIFrameWindow();
        UIBackground->SetAutoDelete(true);
        AttachChild(UIBackground);
        xml_init.InitFrameWindow(uiXml, "background_frame", 0, UIBackground);
    }

	m_complex_desc = false;

	if(uiXml.NavigateToNode("static_name",0))
	{
		UIName						= xr_new<CUIStatic>();	 
		AttachChild					(UIName);		
		UIName->SetAutoDelete		(true);
		xml_init.InitStatic			(uiXml, "static_name", 0,	UIName);
		m_complex_desc = (uiXml.ReadAttribInt("static_name", 0, "complex_desc", 0) == 1);
	}

	if(uiXml.NavigateToNode("static_weight",0))
	{
		UIWeight				= xr_new<CUIStatic>();	 
		AttachChild				(UIWeight);		
		UIWeight->SetAutoDelete(true);
		xml_init.InitStatic		(uiXml, "static_weight", 0,			UIWeight);
	}

	if(uiXml.NavigateToNode("static_cost",0))
	{
		UICost					= xr_new<CUIStatic>();	 
		AttachChild				(UICost);
		UICost->SetAutoDelete	(true);
		xml_init.InitStatic		(uiXml, "static_cost", 0,			UICost);
	}

	if(uiXml.NavigateToNode("descr_list",0))
	{
		UIWpnParams						= xr_new<CUIWpnParams>();
		UIArtefactParams				= xr_new<CUIArtefactParams>();
		UIWpnParams->InitFromXml		(uiXml);
		UIArtefactParams->InitFromXml	(uiXml);
		UIDesc							= xr_new<CUIScrollView>(); 
		AttachChild						(UIDesc);		
		UIDesc->SetAutoDelete			(true);
		m_desc_info.bShowDescrText		= !!uiXml.ReadAttribInt("descr_list", 0, "only_text_info", 1);
		m_b_FitToHeight                 = !!uiXml.ReadAttribInt("descr_list", 0, "fit_to_height", 0);
		xml_init.InitScrollView			(uiXml, "descr_list", 0, UIDesc);
		xml_init.InitFont				(uiXml, "descr_list:font", 0, m_desc_info.uDescClr, m_desc_info.pDescFont);
	}	

	if (uiXml.NavigateToNode("image_static", 0))
	{	
		UIItemImage					= xr_new<CUIStatic>();	 
		AttachChild					(UIItemImage);	
		UIItemImage->SetAutoDelete	(true);
		xml_init.InitStatic			(uiXml, "image_static", 0, UIItemImage);
		UIItemImage->TextureAvailable(true);

		UIItemImage->TextureOff			();
		UIItemImage->ClipperOn			();
		UIItemImageSize.set				(UIItemImage->GetWidth(),UIItemImage->GetHeight());
	}

	xml_init.InitAutoStaticGroup	(uiXml, "auto", 0, this);
}

void CUIItemInfo::Init(float x, float y, float width, float height, LPCSTR xml_name)
{
	inherited::Init	(x, y, width, height);
    Init			(xml_name);
}

bool				IsGameTypeSingle();

void CUIItemInfo::InitItem(CInventoryItem* pInvItem)
{
	m_pInvItem				= pInvItem;
	if(!m_pInvItem)			return;

	Fvector2 pos;
    pos.set(0.0f, 0.0f);
	string256				str;
	if(UIName)
	{
		UIName->SetText(pInvItem->Name());
        UIName->AdjustHeightToText();
        pos.y = UIName->GetWndPos().y + UIName->GetHeight() + 4.0f;
	}
	if(UIWeight)
	{
		LPCSTR kg_str = CStringTable().translate("st_kg").c_str();
        float weight = pInvItem->Weight();

		sprintf_s(str, "%3.2f %s", weight, kg_str);
        UIWeight->SetText(str);

        pos.x = UIWeight->GetWndPos().x;
        if (m_complex_desc)
        {
            UIWeight->SetWndPos(pos);
        }
	}
	if( UICost && IsGameTypeSingle() )
	{
		sprintf_s				(str, "%d %s", pInvItem->Cost(),*CStringTable().translate("ui_st_money_regional"));		// will be owerwritten in multiplayer
		UICost->SetText		(str);
		pos.x = UICost->GetWndPos().x;
        if (m_complex_desc)
        {
            UICost->SetWndPos(pos);
        }
	}
	if(UIDesc)
	{
		pos = UIDesc->GetWndPos();
		UIDesc->Clear						();
		VERIFY								(0==UIDesc->GetSize());
		if (m_desc_info.bShowDescrText)
        {
            CUIStatic* pItem = new CUIStatic();
            pItem->SetTextColor(m_desc_info.uDescClr);
            pItem->SetFont(m_desc_info.pDescFont);
            pItem->SetWidth(UIDesc->GetDesiredChildWidth());
            pItem->SetTextComplexMode(true);
            pItem->SetText(*pInvItem->ItemDescription());
            pItem->AdjustHeightToText();
            UIDesc->AddWindow(pItem, true);
        }
		    
		CTorch* torch = smart_cast<CTorch*>(pInvItem);
	    if (torch) 
		{
			float bp = torch->GetBattareyPower(); 
			TryAddPowerLevelInfo(bp);	
		}

		CHandTorch* hand_torch = smart_cast<CHandTorch*>(pInvItem);
	    if (hand_torch) 
		{
			float bp = hand_torch->GetBattareyPower(); 
			TryAddPowerLevelInfo(bp);	
		}

		TryAddArtefactInfo					(pInvItem->object().cNameSect());
		TryAddWpnInfo		    			(pInvItem->object().cNameSect());
		if (m_b_FitToHeight)
        {
            UIDesc->SetWndSize(Fvector2().set(UIDesc->GetWndSize().x, UIDesc->GetPadSize().y));
            Fvector2 new_size;
            new_size.y = UIDesc->GetWndPos().y + UIDesc->GetWndSize().y + 20.0f;
			new_size.x = GetWndSize().x;
            new_size.x = _max(105.0f, new_size.x);
            new_size.y = _max(105.0f, new_size.y);

            SetWndSize(new_size);
            
			if (UIBackground)
			{
				UIBackground->SetWidth			(GetWndSize().x);
				UIBackground->SetHeight			(GetWndSize().y);
			}
        }
		UIDesc->ScrollToBegin				();
	}
	if (UIItemImage)
    {
		// Загружаем картинку
		UIItemImage->SetShader				(InventoryUtilities::GetEquipmentIconsShader());

		int iGridWidth						= pInvItem->GetGridWidth();
		int iGridHeight						= pInvItem->GetGridHeight();
		int iXPos							= pInvItem->GetXPos();
		int iYPos							= pInvItem->GetYPos();

		UIItemImage->GetUIStaticItem().SetOriginalRect(	float(iXPos*INV_GRID_WIDTH), float(iYPos*INV_GRID_HEIGHT),
														float(iGridWidth*INV_GRID_WIDTH),	float(iGridHeight*INV_GRID_HEIGHT));
		UIItemImage->TextureOn				();
		UIItemImage->ClipperOn				();
		UIItemImage->SetStretchTexture		(false);
		Frect v_r							= {	0.0f, 
												0.0f, 
												float(iGridWidth*INV_GRID_WIDTH),	
												float(iGridHeight*INV_GRID_HEIGHT)};
		if(UI()->is_16_9_mode())
			v_r.x2 /= 1.328f;

		UIItemImage->GetUIStaticItem().SetRect	(v_r);
		UIItemImage->SetWidth					(_min(v_r.width(),	UIItemImageSize.x));
    }
}

void CUIItemInfo::TryAddWpnInfo (const shared_str& wpn_section){
	if (UIWpnParams->Check(wpn_section))
	{
		UIWpnParams->SetInfo(&m_pInvItem->object());
		UIDesc->AddWindow(UIWpnParams,false);
	}
}

void CUIItemInfo::TryAddArtefactInfo(const shared_str& af_section)
{
    if (UIArtefactParams->Check(af_section))
    {
        UIArtefactParams->SetInfo(&m_pInvItem->object());
        UIDesc->AddWindow(UIArtefactParams, false);
    }
}

void CUIItemInfo::Draw()
{
	if(m_pInvItem || m_b_force_drawing)
		inherited::Draw();
}

void CUIItemInfo::TryAddPowerLevelInfo (float power_level)
{
	string128 _buff;
	CUIStatic* BattareyPower = new CUIStatic();
	BattareyPower->SetTextColor(m_desc_info.uDescClr);
	BattareyPower->SetFont(m_desc_info.pDescFont);
	BattareyPower->SetWidth(UIDesc->GetDesiredChildWidth());
	BattareyPower->SetTextComplexMode(true);
	int pl = iFloor((power_level*100)+0.5);
	sprintf_s(_buff, "%s: %u %s", "Заряд батареи",	pl, "%.");
	BattareyPower->SetText(_buff);
	BattareyPower->AdjustHeightToText();
	UIDesc->AddWindow(BattareyPower, true);
}