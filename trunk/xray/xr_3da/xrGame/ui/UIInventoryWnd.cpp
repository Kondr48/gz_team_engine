#include "pch_script.h"
#include "UIInventoryWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../string_table.h"

#include "../actor.h"
#include "../uigamesp.h"
#include "../hudmanager.h"

#include "../CustomOutfit.h"

#include "../weapon.h"

#include "../script_process.h"

#include "../eatable_item.h"
#include "../inventory.h"

#include "UIInventoryUtilities.h"
using namespace InventoryUtilities;


#include "../InfoPortion.h"
#include "../level.h"
#include "../game_base_space.h"
#include "../entitycondition.h"

#include "../game_cl_base.h"
#include "../ActorCondition.h"
#include "UIDragDropListEx.h"
#include "UIOutfitSlot.h"
#include "UI3tButton.h"
#include "../../../build_config_defines.h"

#define				INVENTORY_ITEM_XML		"inventory_item.xml"
#define				INVENTORY_XML			"inventory_new.xml"

#include "UIHelper.h"
#include "UICellItem.h"

CUIInventoryWnd*	g_pInvWnd = NULL;

CUIInventoryWnd::CUIInventoryWnd()
{
	m_iCurrentActiveSlot				= NO_ACTIVE_SLOT;
	Init								();
	SetCurrentItem						(NULL);

	g_pInvWnd							= this;	
	m_b_need_reinit						= false;
	Hide								();	
}

void CUIInventoryWnd::Init()
{
	CUIXml								uiXml;
	bool xml_result						= uiXml.Init(CONFIG_PATH, UI_PATH, INVENTORY_XML);
	R_ASSERT3							(xml_result, "file parsing error ", uiXml.m_xml_file_name);

	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	AttachChild							(&UIBack);
	xml_init.InitStatic					(uiXml, "back", 0, &UIBack);

	AttachChild							(&UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_static", 0, &UIStaticBottom);

	AttachChild							(&UIBagWnd);
	xml_init.InitStatic					(uiXml, "bag_static", 0, &UIBagWnd);
	
	AttachChild							(&UIMoneyWnd);
	xml_init.InitStatic					(uiXml, "money_static", 0, &UIMoneyWnd);

	AttachChild							(&UIDescrWnd);
	xml_init.InitStatic					(uiXml, "descr_static", 0, &UIDescrWnd);


	UIDescrWnd.AttachChild				(&UIItemInfo);
	UIItemInfo.Init						(0, 0, UIDescrWnd.GetWidth(), UIDescrWnd.GetHeight(), INVENTORY_ITEM_XML);
	
	AttachChild							(&UIPersonalWnd);
	xml_init.InitFrameWindow			(uiXml, "character_frame_window", 0, &UIPersonalWnd);

	AttachChild							(&UIProgressBack);
	xml_init.InitStatic					(uiXml, "progress_background", 0, &UIProgressBack);
	
	UIProgressBack.AttachChild (&UIProgressBarHealth);
	xml_init.InitProgressBar (uiXml, "progress_bar_health", 0, &UIProgressBarHealth);
	
	UIProgressBack.AttachChild	(&UIProgressBarPsyHealth);
	xml_init.InitProgressBar (uiXml, "progress_bar_psy", 0, &UIProgressBarPsyHealth);

	UIProgressBack.AttachChild	(&UIProgressBarRadiation);
	xml_init.InitProgressBar (uiXml, "progress_bar_radiation", 0, &UIProgressBarRadiation);

	UIProgressBack.AttachChild	(&UIProgressBarBleeding);
	xml_init.InitProgressBar (uiXml, "progress_bar_bleeding", 0, &UIProgressBarBleeding);

	UIProgressBack.AttachChild	(&UIProgressBarSatiety);
	xml_init.InitProgressBar (uiXml, "progress_bar_satiety", 0, &UIProgressBarSatiety);

	UIProgressBack.AttachChild	(&UIProgressBarThirst);
	xml_init.InitProgressBar (uiXml, "progress_bar_thirst", 0, &UIProgressBarThirst);
	
	m_WeaponSlot1_progress	= UIHelper::CreateProgressBar(uiXml, "progess_bar_weapon1", this);
	m_WeaponSlot2_progress	= UIHelper::CreateProgressBar(uiXml, "progess_bar_weapon2", this);
	m_Helmet_progress		= UIHelper::CreateProgressBar(uiXml, "progess_bar_helmet", this);
	m_Outfit_progress		= UIHelper::CreateProgressBar(uiXml, "progess_bar_outfit", this);

	m_InvSlot0Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_weapon_0", this);
    m_InvSlot0Highlight->Show(false);
	m_InvSlot1Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_weapon_1", this);
    m_InvSlot1Highlight->Show(false);
	m_InvSlot2Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_weapon_2", this);
    m_InvSlot2Highlight->Show(false);
	m_InvSlot4Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_weapon_3", this);
    m_InvSlot4Highlight->Show(false);
	m_InvSlot6Highlight     = UIHelper::CreateStatic(uiXml, "highlight_outfit", this);
    m_InvSlot6Highlight->Show(false);
	m_InvSlot7Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_pda", this);
    m_InvSlot7Highlight->Show(false);
	m_InvSlot8Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_detector_1", this);
    m_InvSlot8Highlight->Show(false);
	m_InvSlot9Highlight     = UIHelper::CreateStatic(uiXml, "highlight_slot_torch", this);
    m_InvSlot9Highlight->Show(false);
	m_InvSlot11Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_helmet", this);
    m_InvSlot11Highlight->Show(false);
	m_InvSlot12Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_quick_access_0", this);
    m_InvSlot12Highlight->Show(false);
	m_InvSlot13Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_quick_access_1", this);
    m_InvSlot13Highlight->Show(false);
	m_InvSlot14Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_quick_access_2", this);
    m_InvSlot14Highlight->Show(false);
	m_InvSlot15Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_quick_access_3", this);
    m_InvSlot15Highlight->Show(false);
	m_InvSlot16Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_nightvision", this);
    m_InvSlot16Highlight->Show(false);
	m_InvSlot17Highlight    = UIHelper::CreateStatic(uiXml, "highlight_slot_detector_2", this);
    m_InvSlot17Highlight->Show(false);

	m_HelmetOver = UIHelper::CreateStatic(uiXml, "helmet_over", this);
    m_HelmetOver->Show(false);
	m_NightvisionOver = UIHelper::CreateStatic(uiXml, "nightvision_over", this);
    m_NightvisionOver->Show(false);

	m_belt_list_over[0] = UIHelper::CreateStatic(uiXml, "belt_list_over", this);
	m_belt_highlight[0] = UIHelper::CreateStatic(uiXml, "highlight_belt_list", this);
	m_belt_highlight[0]->SetVisible(false);
	Fvector2 pos;
	
	pos        = m_belt_list_over[0]->GetWndPos();
    float    w = uiXml.ReadAttribFlt("dragdrop_belt", 0, "cell_width",  10.0f);
    float    h = uiXml.ReadAttribFlt("dragdrop_belt", 0, "cell_height", 10.0f);
	float sp_x = uiXml.ReadAttribFlt("dragdrop_belt", 0, "cell_sp_x",   10.0f);
	float sp_y = uiXml.ReadAttribFlt("dragdrop_belt", 0, "cell_sp_y",   10.0f);

	for (u8 j = 1; j < e_af_count; ++j)
    {		 
        if (j == 5) //ѕеремещаемс€ на нижний р€д иконок
		{ 
		  pos.y += h + sp_y;
	      pos.x = m_belt_list_over[0]->GetWndPos().x; 
		  pos.x -= w + sp_x; 
		} 

		  pos.x += w + sp_x;
		 
		  m_belt_list_over[j] = UIHelper::CreateStatic(uiXml, "belt_list_over", this);      // Ѕлокировка €чеек
		  m_belt_list_over[j]->SetWndPos(pos);
		 
		  m_belt_highlight[j] = UIHelper::CreateStatic(uiXml, "highlight_belt_list", this); // ѕодсветка €чеек
		  m_belt_highlight[j]->SetWndPos(pos);
		  m_belt_highlight[j]->SetVisible(false);
    }

	UIPersonalWnd.AttachChild			(&UIStaticPersonal);
	xml_init.InitStatic					(uiXml, "static_personal",0, &UIStaticPersonal);
//	UIStaticPersonal.Init				(1, UIPersonalWnd.GetHeight() - 175, 260, 260);

	AttachChild							(&UIOutfitInfo);
	UIOutfitInfo.InitFromXml			(uiXml);
//.	xml_init.InitStatic					(uiXml, "outfit_info_window",0, &UIOutfitInfo);

	//Ёлементы автоматического добавлени€
	xml_init.InitAutoStatic				(uiXml, "auto_static", this);

	m_pUIBagList						= xr_new<CUIDragDropListEx>(); UIBagWnd.AttachChild(m_pUIBagList); m_pUIBagList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_bag", 0, m_pUIBagList);
	BindDragDropListEnents				(m_pUIBagList);

	m_pUIBeltList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBeltList); m_pUIBeltList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_belt", 0, m_pUIBeltList);
	BindDragDropListEnents				(m_pUIBeltList);

	m_pUIOutfitList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIOutfitList); m_pUIOutfitList->SetAutoDelete(true);

	xml_init.InitDragDropListEx			(uiXml, "dragdrop_outfit", 0, m_pUIOutfitList);
	BindDragDropListEnents				(m_pUIOutfitList);
	ZeroMemory(&m_slots_array, sizeof(m_slots_array));

	m_pUIPistolList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPistolList); m_pUIPistolList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_weapon_1", 0, m_pUIPistolList);
	BindDragDropListEnents(m_pUIPistolList);

	m_pUIAutomaticList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIAutomaticList); m_pUIAutomaticList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_weapon_2", 0, m_pUIAutomaticList);
	BindDragDropListEnents(m_pUIAutomaticList);

	m_pUIKnifeList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIKnifeList); m_pUIKnifeList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_weapon_0", 0, m_pUIKnifeList);
	BindDragDropListEnents				(m_pUIKnifeList);	
		
	m_pUIBinocularList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBinocularList); m_pUIBinocularList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_weapon_3", 0, m_pUIBinocularList);
	BindDragDropListEnents				(m_pUIBinocularList);
		
	m_pUIDetectorOneList				= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIDetectorOneList); m_pUIDetectorOneList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_detector_1", 0, m_pUIDetectorOneList);
	BindDragDropListEnents				(m_pUIDetectorOneList);

	m_pUITorchList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUITorchList); m_pUITorchList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_torch", 0, m_pUITorchList);
	BindDragDropListEnents				(m_pUITorchList);

	m_pUIPDAList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPDAList); m_pUIPDAList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_pda", 0, m_pUIPDAList);
	BindDragDropListEnents				(m_pUIPDAList);		
		
	m_pUIHelmetList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIHelmetList); m_pUIHelmetList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_helmet", 0, m_pUIHelmetList);
	BindDragDropListEnents				(m_pUIHelmetList);
		
	m_pUISlotQuickAccessList_0			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_0); m_pUISlotQuickAccessList_0->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_0", 0, m_pUISlotQuickAccessList_0);
	BindDragDropListEnents				(m_pUISlotQuickAccessList_0);
		
	m_pUISlotQuickAccessList_1			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_1); m_pUISlotQuickAccessList_1->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_1", 0, m_pUISlotQuickAccessList_1);
	BindDragDropListEnents				(m_pUISlotQuickAccessList_1);
		
	m_pUISlotQuickAccessList_2			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_2); m_pUISlotQuickAccessList_2->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_2", 0, m_pUISlotQuickAccessList_2);
	BindDragDropListEnents				(m_pUISlotQuickAccessList_2);
		
	m_pUISlotQuickAccessList_3			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_3); m_pUISlotQuickAccessList_3->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_3", 0, m_pUISlotQuickAccessList_3);
	BindDragDropListEnents				(m_pUISlotQuickAccessList_3);
		
	m_pUINightvisionList				= xr_new<CUIDragDropListEx>(); AttachChild(m_pUINightvisionList); m_pUINightvisionList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_nightvision", 0, m_pUINightvisionList);
	BindDragDropListEnents				(m_pUINightvisionList);

	m_pUIDetectorTwoList				= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIDetectorTwoList); m_pUIDetectorTwoList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_detector_2", 0, m_pUIDetectorTwoList);
	BindDragDropListEnents				(m_pUIDetectorTwoList);

	m_slots_array[KNIFE_SLOT]				= m_pUIKnifeList;
	m_slots_array[PISTOL_SLOT]				= m_pUIPistolList;
	m_slots_array[RIFLE_SLOT]				= m_pUIAutomaticList;
	m_slots_array[GRENADE_SLOT]				= NULL;	
	m_slots_array[BOLT_SLOT]				= NULL;		
	m_slots_array[ARTEFACT_SLOT]		    = NULL; // m_pUIBeltList;
	m_slots_array[OUTFIT_SLOT]				= m_pUIOutfitList;
	m_slots_array[APPARATUS_SLOT]			= m_pUIBinocularList;
	m_slots_array[PDA_SLOT]					= m_pUIPDAList;
	m_slots_array[DETECTOR_ONE_SLOT]		= m_pUIDetectorOneList;
	m_slots_array[TORCH_SLOT]				= m_pUITorchList;
	m_slots_array[HELMET_SLOT]				= m_pUIHelmetList;
	m_slots_array[SLOT_QUICK_ACCESS_0]		= m_pUISlotQuickAccessList_0;
	m_slots_array[SLOT_QUICK_ACCESS_1]		= m_pUISlotQuickAccessList_1;
	m_slots_array[SLOT_QUICK_ACCESS_2]		= m_pUISlotQuickAccessList_2;
	m_slots_array[SLOT_QUICK_ACCESS_3]		= m_pUISlotQuickAccessList_3;
	m_slots_array[NIGHTVISION_SLOT]			= m_pUINightvisionList;
	m_slots_array[DETECTOR_TWO_SLOT] 	    = m_pUIDetectorTwoList;
	m_slots_array[ITEMS_SLOT]				= NULL;
	
	//pop-up menu
	AttachChild							(&UIPropertiesBox);
	UIPropertiesBox.Init				(0,0,300,300);
	UIPropertiesBox.Hide				();

	AttachChild							(&UIStaticTime);
	xml_init.InitStatic					(uiXml, "time_static", 0, &UIStaticTime);

	UIStaticTime.AttachChild			(&UIStaticTimeString);
	xml_init.InitStatic					(uiXml, "time_static_str", 0, &UIStaticTimeString);

	UIExitButton						= xr_new<CUI3tButton>();UIExitButton->SetAutoDelete(true);
	AttachChild							(UIExitButton);
	xml_init.Init3tButton				(uiXml, "exit_button", 0, UIExitButton);

//Load sounds

	XML_NODE* stored_root				= uiXml.GetLocalRoot		();
	uiXml.SetLocalRoot					(uiXml.NavigateToNode		("action_sounds",0));
	::Sound->create						(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvSndClose],		uiXml.Read("snd_close",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToSlot],	uiXml.Read("snd_item_to_slot",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToBelt],	uiXml.Read("snd_item_to_belt",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToRuck],	uiXml.Read("snd_item_to_ruck",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvProperties],	uiXml.Read("snd_properties",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvAttachAddon],	uiXml.Read("snd_attach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0,	NULL),st_Effect,sg_SourceType);

	uiXml.SetLocalRoot					(stored_root);
}

EListType CUIInventoryWnd::GetType(CUIDragDropListEx* l)
{
	if(l==m_pUIBagList)			return iwBag;
	if(l==m_pUIBeltList)		return iwBelt;

	for (u32 i = 0; i < SLOTS_TOTAL; i++)
		if (m_slots_array[i] == l)
			return iwSlot;

	NODEFAULT;
#ifdef DEBUG
	return iwSlot;
#endif // DEBUG
}

void CUIInventoryWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
        sounds[a].play					(NULL, sm_2D);
}

CUIInventoryWnd::~CUIInventoryWnd()
{
//.	ClearDragDrop(m_vDragDropItems);
	ClearAllLists						();
}

bool CUIInventoryWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if(m_b_need_reinit)
		return true;

	//вызов дополнительного меню по правой кнопке
	if(mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if(UIPropertiesBox.IsShown())
		{
			UIPropertiesBox.Hide		();
			return						true;
		}
	}

	CUIWindow::OnMouse					(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

void CUIInventoryWnd::Draw()
{
	CUIWindow::Draw						();
}


void CUIInventoryWnd::Update()
{
	if(m_b_need_reinit)
		InitInventory					();


	CEntityAlive *pEntityAlive			= smart_cast<CEntityAlive*>(Level().CurrentEntity());

	if(pEntityAlive) 
	{
		float v = pEntityAlive->conditions().GetHealth()*100.0f;
		UIProgressBarHealth.SetProgressPos		(v);

		v = pEntityAlive->conditions().GetPsyHealth()*100.0f;
		UIProgressBarPsyHealth.SetProgressPos	(v);

		v = pEntityAlive->conditions().GetRadiation()*100.0f;
		UIProgressBarRadiation.SetProgressPos	(v);

		v = pEntityAlive->conditions().BleedingSpeed()*100.0f;
		UIProgressBarBleeding.SetProgressPos	(v);
		
		CActor*	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
			
		v =(m_pActor->conditions().GetSatiety())*100.0f;
		UIProgressBarSatiety.SetProgressPos	(v);

		v = (m_pActor->conditions().GetThirst())*100.0f;
		UIProgressBarThirst.SetProgressPos	(v);

		CInventoryOwner* pOurInvOwner	= smart_cast<CInventoryOwner*>(pEntityAlive);
		u32 _money						= 0;
		_money							= pOurInvOwner->get_money();
		// update money
		string64						sMoney;
		//red_virus
		sprintf_s						(sMoney,"%d %s", _money, *CStringTable().translate("ui_st_money_regional"));
		UIMoneyWnd.SetText				(sMoney);

		// update outfit parameters
		CCustomOutfit* outfit			= smart_cast<CCustomOutfit*>(pOurInvOwner->inventory().m_slots[OUTFIT_SLOT].m_pIItem);		
		UIOutfitInfo.Update				(outfit);		
	}

	UIStaticTimeString.SetText(*InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes));

	UpdateConditionProgressBars();

	UpdateOutfit();

	CUIWindow::Update					();
}

void CUIInventoryWnd::Show() 
{ 	

	InitInventory			();
	inherited::Show			();

	SendInfoToActor						("ui_inventory");

	Update								();
	PlaySnd								(eInvSndOpen);
}

void CUIInventoryWnd::Hide()
{
	PlaySnd								(eInvSndClose);
	inherited::Hide						();

	SendInfoToActor						("ui_inventory_hide");
	ClearAllLists						();

	//достать вещь в активный слот
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && m_iCurrentActiveSlot != NO_ACTIVE_SLOT && 
		pActor->inventory().m_slots[m_iCurrentActiveSlot].m_pIItem)
	{
		pActor->inventory().Activate(m_iCurrentActiveSlot);
		m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
	}

	if (!IsGameTypeSingle())
	{
		CActor *pActor		= smart_cast<CActor*>(Level().CurrentEntity());
		if(!pActor)			return;

		pActor->SetWeaponHideState(INV_STATE_INV_WND, false);
	}
}

void CUIInventoryWnd::AttachAddon(PIItem item_to_upgrade)
{
	PlaySnd										(eInvAttachAddon);
	R_ASSERT									(item_to_upgrade);
	if (OnClient())
	{
		NET_Packet								P;
		item_to_upgrade->object().u_EventGen	(P, GE_ADDON_ATTACH, item_to_upgrade->object().ID());
		P.w_u32									(CurrentIItem()->object().ID());
		item_to_upgrade->object().u_EventSend	(P);
	};

	item_to_upgrade->Attach						(CurrentIItem(), true);


	//спр€тать вещь из активного слота в инвентарь на врем€ вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && item_to_upgrade == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
	SetCurrentItem								(NULL);
}

void CUIInventoryWnd::DetachAddon(const char* addon_name)
{
	PlaySnd										(eInvDetachAddon);
	if (OnClient())
	{
		NET_Packet								P;
		CurrentIItem()->object().u_EventGen		(P, GE_ADDON_DETACH, CurrentIItem()->object().ID());
		P.w_stringZ								(addon_name);
		CurrentIItem()->object().u_EventSend	(P);
	};
	CurrentIItem()->Detach						(addon_name, true);

	//спр€тать вещь из активного слота в инвентарь на врем€ вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && CurrentIItem() == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
}


void	CUIInventoryWnd::SendEvent_ActivateSlot	(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ACTIVATE_SLOT, pItem->object().H_Parent()->ID());
	P.w_u32							(pItem->GetSlot());
	pItem->object().u_EventSend		(P);
}

void	CUIInventoryWnd::SendEvent_Item2Slot			(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2SLOT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
	g_pInvWnd->PlaySnd				(eInvItemToSlot);
};

void	CUIInventoryWnd::SendEvent_Item2Belt			(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2BELT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
	g_pInvWnd->PlaySnd				(eInvItemToBelt);
};

void	CUIInventoryWnd::SendEvent_Item2Ruck			(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2RUCK, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);

	g_pInvWnd->PlaySnd				(eInvItemToRuck);
};

void	CUIInventoryWnd::SendEvent_Item_Drop(PIItem	pItem)
{
	pItem->SetDropManual			(TRUE);

	if( OnClient() )
	{
		NET_Packet					P;
		pItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pItem->object().H_Parent()->ID());
		P.w_u16						(pItem->object().ID());
		pItem->object().u_EventSend(P);
	}
	g_pInvWnd->PlaySnd				(eInvDropItem);
};

void	CUIInventoryWnd::SendEvent_Item_Eat			(PIItem	pItem)
{
	R_ASSERT						(pItem->m_pCurrentInventory==m_pInv);
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM_EAT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
};


void CUIInventoryWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemRButtonClick);
	lst->m_f_item_focus_received    = CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemFocusReceive);
    lst->m_f_item_focus_lost        = CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemFocusLost);
    lst->m_f_item_focused_update    = CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemFocusedUpdate);
}


#include "../xr_level_controller.h"
#include <dinput.h>

bool CUIInventoryWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if(m_b_need_reinit)
		return true;

	if (UIPropertiesBox.GetVisible())
		UIPropertiesBox.OnKeyboard(dik, keyboard_action);

	if ( is_binded(kDROP, dik) )
	{
		if(WINDOW_KEY_PRESSED==keyboard_action)
			DropCurrentItem(false);
		return true;
	}

	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
#ifdef DEBUG
		if(DIK_NUMPAD7 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(-0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
		else if(DIK_NUMPAD8 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
#endif
	}
	if( inherited::OnKeyboard(dik,keyboard_action) )return true;

	return false;
}

void CUIInventoryWnd::UpdateConditionProgressBars()
{
	CActor*	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	
	PIItem itm = m_pActor->inventory().ItemFromSlot(PISTOL_SLOT);
	if(itm)
	{
		m_WeaponSlot1_progress->SetProgressPos(iCeil(itm->GetCondition()*15.0f)/15.0f);
	}
	else
		m_WeaponSlot1_progress->SetProgressPos(0);

	itm = m_pActor->inventory().ItemFromSlot(RIFLE_SLOT);
	if(itm)
		m_WeaponSlot2_progress->SetProgressPos(iCeil(itm->GetCondition()*15.0f)/15.0f);
	else
		m_WeaponSlot2_progress->SetProgressPos(0);

	itm = m_pActor->inventory().ItemFromSlot(OUTFIT_SLOT);
	if(itm)
		m_Outfit_progress->SetProgressPos(iCeil(itm->GetCondition()*15.0f)/15.0f);
	else
		m_Outfit_progress->SetProgressPos(0);

	itm = m_pActor->inventory().ItemFromSlot(HELMET_SLOT);
	if(itm)
		m_Helmet_progress->SetProgressPos(iCeil(itm->GetCondition()*15.0f)/15.0f);
	else
		m_Helmet_progress->SetProgressPos(0);
}

void CUIInventoryWnd::set_highlight_item(CUICellItem* cell_item)
{
    PIItem item = (PIItem)cell_item->m_pData;
    if (!item) {
        return;
    }
   
	highlight_item_slot(cell_item);

    m_highlight_clear = false;
}

void CUIInventoryWnd::highlight_item_slot(CUICellItem* cell_item)
{
    PIItem item = (PIItem)cell_item->m_pData;
    if (!item) return;

    if (CUIDragDropListEx::m_drag_item) return;

	if (item->Belt()) 
	{
	    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
		u32 af_count   = pActor->inventory().BeltWidth();
		
		for (u8 i = 0; i < af_count; ++i)
          m_belt_highlight[i]->SetVisible(true);
	}

	u32 slots_count = item->GetSlotsCount();
	if (slots_count < 1) return;
	for (u32 i = 1; i <= slots_count; i++)
	{
		u32 slot = item->GetSlots()[i-1];
		     if (slot == 0)  m_InvSlot0Highlight->Show(true);
		else if (slot == 1)	 m_InvSlot1Highlight->Show(true);
		else if (slot == 2)	 m_InvSlot2Highlight->Show(true);
		else if (slot == 4)	 m_InvSlot4Highlight->Show(true);
		else if (slot == 6)	 m_InvSlot6Highlight->Show(true);
		else if (slot == 7)	 m_InvSlot7Highlight->Show(true);
		else if (slot == 8)	 m_InvSlot8Highlight->Show(true);
		else if (slot == 9)	 m_InvSlot9Highlight->Show(true);
		else if (slot == 11) m_InvSlot11Highlight->Show(true);
		else if (slot == 12) m_InvSlot12Highlight->Show(true);
		else if (slot == 13) m_InvSlot13Highlight->Show(true);
		else if (slot == 14) m_InvSlot14Highlight->Show(true);
		else if (slot == 15) m_InvSlot15Highlight->Show(true);
		else if (slot == 16) m_InvSlot16Highlight->Show(true);
		else if (slot == 17) m_InvSlot17Highlight->Show(true);
	}
}

void CUIInventoryWnd::clear_highlight_lists()
{
    m_InvSlot0Highlight->Show(false);
	m_InvSlot1Highlight->Show(false);
	m_InvSlot2Highlight->Show(false);
	m_InvSlot4Highlight->Show(false);
	m_InvSlot6Highlight->Show(false);
	m_InvSlot7Highlight->Show(false);
	m_InvSlot8Highlight->Show(false);
	m_InvSlot9Highlight->Show(false);
	m_InvSlot11Highlight->Show(false);
	m_InvSlot12Highlight->Show(false);
	m_InvSlot13Highlight->Show(false);
	m_InvSlot14Highlight->Show(false);
	m_InvSlot15Highlight->Show(false);
	m_InvSlot16Highlight->Show(false);
	m_InvSlot17Highlight->Show(false);

	for (u8 i = 0; i < e_af_count; ++i)
    {
        m_belt_highlight[i]->SetVisible(false);
    }
    
    m_highlight_clear = true;
}

void CUIInventoryWnd::UpdateOutfit() 
{
    CActor* pActor			= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)				return;

    for (u8 i = 0; i < e_af_count; ++i)
    {
        m_belt_list_over[i]->SetVisible(true);
    }

    u32 af_count = pActor->inventory().BeltWidth();

    VERIFY(m_pUIBeltList);

	CCustomOutfit* outfit = pActor->GetOutfit();
	if (outfit && !outfit->bIsHelmetAvaliable)
        m_HelmetOver->Show(true);
    else
        m_HelmetOver->Show(false);
	
	if (outfit && !outfit->bIsNightvisionAvaliable)
        m_NightvisionOver->Show(true);
    else
        m_NightvisionOver->Show(false);
    
	if (!outfit) {
        MoveArtefactsToBag();
        return;
    }

    for (u8 i = 0; i < af_count; ++i)
    {
        m_belt_list_over[i]->SetVisible(false);
    }
}

void CUIInventoryWnd::MoveArtefactsToBag()
{
    while (m_pUIBeltList->ItemsCount())
    {
        CUICellItem* ci = m_pUIBeltList->GetItemIdx(0);
        VERIFY(ci && ci->m_pData);
        ToBag(ci, false);
    }  // for i
	m_pUIBeltList->ClearAll					(true);
}
