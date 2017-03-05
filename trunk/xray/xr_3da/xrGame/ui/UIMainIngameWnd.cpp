#include "stdafx.h"

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "../UIZoneMap.h"


#include <dinput.h>
#include "../ai_space.h"
#include "../script_engine.h"
#include "../actor.h"
#include "../ActorCondition.h"
#include "../EntityCondition.h"
#include "../HUDManager.h"
#include "../PDA.h"
#include "../WeaponHUD.h"
#include "../character_info.h"
#include "../inventory.h"
#include "../UIGameSP.h"
#include "../weaponmagazined.h"
#include "../missile.h"
#include "../Grenade.h"
#include "../xrServer_objects_ALife.h"
#include "../alife_simulator.h"
#include "../alife_object_registry.h"
#include "../game_cl_base.h"
#include "../level.h"
#include "../seniority_hierarchy_holder.h"

#include "../date_time.h"
#include "../xrServer_Objects_ALife_Monsters.h"
#include "../../LightAnimLibrary.h"

#include "UIInventoryUtilities.h"
#include "UIHelper.h"

#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "../alife_registry_wrappers.h"
#include "../actorcondition.h"

#include "../string_table.h"
#include "../clsid_game.h"
#include "UIArtefactPanel.h"
#include "UIMap.h"
#include <functional>  // добавлено alpet для успешной сборки в VS 2013

#ifdef DEBUG
#	include "../attachable_item.h"
#endif

#include "../../xr_input.h"

#include "UIScrollView.h"
#include "map_hint.h"
#include "UIColorAnimatorWrapper.h"
#include "../game_news.h"
#include "../pch_script.h"


#ifdef DEBUG
#	include "../debug_renderer.h"

void test_draw	();
void test_key	(int dik);
void test_update();
#endif


using namespace InventoryUtilities;
using namespace luabind;

//	hud adjust mode
int			g_bHudAdjustMode			= 0;
float		g_fHudAdjustValue			= 0.0f;

DLL_API CUIMainIngameWnd* GetMainIngameWindow()
{
	if (g_hud)
	{
		CUI *pUI = g_hud->GetUI();
		if (pUI)
			return pUI->UIMainIngameWnd;
	}
	return NULL;
}


#ifdef SCRIPT_ICONS_CONTROL
	CUIStatic * warn_icon_list[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	
	bool __declspec(dllexport) external_icon_ctrl = false;			// alpet: для возможности внешнего контроля иконок (используется в NLC6 вместо типичных индикаторов). Никак не влияет на игру для остальных модов.	


	bool __declspec(dllexport) SetupGameIcon(u32 icon, u32 cl, float width, float height) // позволяет расцветить иконку или изменить её размер
	{
		CUIMainIngameWnd *window = GetMainIngameWindow();
		if (!window)
		{
			log_script_error("SetupGameIcon failed due GetMainIngameWindow() returned NULL");
			return false;
		}


		CUIStatic *sIcon = warn_icon_list[icon & 7];
		
		if (sIcon)
		{			
			if (width > 0 && height > 0)
			{
				sIcon->SetWidth (width);
				sIcon->SetHeight (height);
				sIcon->SetStretchTexture(cl > 0);
			}
			else 
				window->SetWarningIconColor((CUIMainIngameWnd::EWarningIcons)icon, cl);

			external_icon_ctrl = true;
			return true;
		}
		return false;
	}

#else
#define external_icon_ctrl				0
#endif

const u32	g_clWhite					= 0xffffffff;

#define		DEFAULT_MAP_SCALE			1.f

#define		C_SIZE						0.025f
#define		NEAR_LIM					0.5f

#define		SHOW_INFO_SPEED				0.5f
#define		HIDE_INFO_SPEED				10.f
#define		C_ON_ENEMY					D3DCOLOR_XRGB(0xff,0,0)
#define		C_DEFAULT					D3DCOLOR_XRGB(0xff,0xff,0xff)

#define				MAININGAME_XML				"maingame.xml"

CUIMainIngameWnd::CUIMainIngameWnd()
{
	m_pActor					= NULL;
	m_pWeapon					= NULL;
	m_pGrenade					= NULL;
	m_pItem						= NULL;
	UIZoneMap					= xr_new<CUIZoneMap>();
	m_pPickUpItem				= NULL;
	m_artefactPanel				= xr_new<CUIArtefactPanel>();
	m_pMPChatWnd				= NULL;
	m_pMPLogWnd					= NULL;	
#ifdef SCRIPT_ICONS_CONTROL
	warn_icon_list[ewiInvincible]	= &UIInvincibleIcon;	
	warn_icon_list[ewiArtefact]		= &UIArtefactIcon;
#endif
}

#include "UIProgressShape.h"
extern CUIProgressShape* g_MissileForceShape;

CUIMainIngameWnd::~CUIMainIngameWnd()
{
	DestroyFlashingIcons		();
	xr_delete					(UIZoneMap);
	xr_delete					(m_artefactPanel);
	HUD_SOUND::DestroySound		(m_contactSnd);
	xr_delete					(g_MissileForceShape);
}

void CUIMainIngameWnd::Init()
{
	CUIXml						uiXml;
	uiXml.Init					(CONFIG_PATH, UI_PATH, MAININGAME_XML);
	
	CUIXmlInit					xml_init;
	CUIWindow::Init				(0,0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

	Enable(false);


	AttachChild					(&UIStaticHealth);
	xml_init.InitStatic			(uiXml, "static_health", 0, &UIStaticHealth);

	AttachChild					(&UIStaticArmor);
	xml_init.InitStatic			(uiXml, "static_armor", 0, &UIStaticArmor);

	AttachChild					(&UIWeaponBack);
	xml_init.InitStatic			(uiXml, "static_weapon", 0, &UIWeaponBack);

	UIWeaponBack.AttachChild	(&UIWeaponSignAmmo);
	xml_init.InitStatic			(uiXml, "static_ammo", 0, &UIWeaponSignAmmo);
	UIWeaponSignAmmo.SetElipsis	(CUIStatic::eepEnd, 2);

	UIWeaponBack.AttachChild	(&UIWeaponIcon);
	xml_init.InitStatic			(uiXml, "static_wpn_icon", 0, &UIWeaponIcon);
	UIWeaponIcon.SetShader		(GetEquipmentIconsShader());
	UIWeaponIcon_rect			= UIWeaponIcon.GetWndRect();
	//---------------------------------------------------------
	AttachChild					(&UIPickUpItemIcon);
	xml_init.InitStatic			(uiXml, "pick_up_item", 0, &UIPickUpItemIcon);
	UIPickUpItemIcon.SetShader	(GetEquipmentIconsShader());
	UIPickUpItemIcon.Show		(false);

	m_iPickUpItemIconWidth		= UIPickUpItemIcon.GetWidth();
	m_iPickUpItemIconHeight		= UIPickUpItemIcon.GetHeight();
	m_iPickUpItemIconX			= UIPickUpItemIcon.GetWndRect().left;
	m_iPickUpItemIconY			= UIPickUpItemIcon.GetWndRect().top;
	//---------------------------------------------------------


	UIWeaponIcon.Enable			(false);

	//индикаторы 
	UIZoneMap->Init				();
	UIZoneMap->SetScale			(DEFAULT_MAP_SCALE);

	if(IsGameTypeSingle())
	{
		xml_init.InitStatic					(uiXml, "static_pda_online", 0, &UIPdaOnline);
		UIZoneMap->Background().AttachChild	(&UIPdaOnline);
	}


	//Полоса прогресса здоровья
	UIStaticHealth.AttachChild	(&UIHealthBar);
//.	xml_init.InitAutoStaticGroup(uiXml,"static_health", &UIStaticHealth);
	xml_init.InitProgressBar	(uiXml, "progress_bar_health", 0, &UIHealthBar);

	//Полоса прогресса армора
	UIStaticArmor.AttachChild	(&UIArmorBar);
//.	xml_init.InitAutoStaticGroup(uiXml,"static_armor", &UIStaticArmor);
	xml_init.InitProgressBar	(uiXml, "progress_bar_armor", 0, &UIArmorBar);

	

	// Подсказки, которые возникают при наведении прицела на объект
	AttachChild					(&UIStaticQuickHelp);
	xml_init.InitStatic			(uiXml, "quick_info", 0, &UIStaticQuickHelp);

	uiXml.SetLocalRoot			(uiXml.GetRoot());

	m_UIIcons					= xr_new<CUIScrollView>(); m_UIIcons->SetAutoDelete(true);
	xml_init.InitScrollView		(uiXml, "icons_scroll_view", 0, m_UIIcons);
	AttachChild					(m_UIIcons);

	m_ind_starvation		    = UIHelper::CreateStatic(uiXml, "indicator_starvation", this);
    m_ind_thirst		        = UIHelper::CreateStatic(uiXml, "indicator_thirst", this);
	m_ind_bleeding			    = UIHelper::CreateStatic(uiXml, "indicator_bleeding", this);
	m_ind_radiation			    = UIHelper::CreateStatic(uiXml, "indicator_radiation", this);
	m_ind_psy_health			= UIHelper::CreateStatic(uiXml, "indicator_psy_health", this);
	m_ind_weapon_broken		    = UIHelper::CreateStatic(uiXml, "indicator_weapon_broken", this);
	m_ind_helmet_broken		    = UIHelper::CreateStatic(uiXml, "indicator_helmet_broken", this);
	m_ind_outfit_broken		    = UIHelper::CreateStatic(uiXml, "indicator_outfit_broken", this);
	m_ind_overweight		    = UIHelper::CreateStatic(uiXml, "indicator_overweight", this);

	m_ind_boost_psy = UIHelper::CreateStatic(uiXml, "indicator_booster_psy", this);
    m_ind_boost_radia = UIHelper::CreateStatic(uiXml, "indicator_booster_radia", this);
    m_ind_boost_chem = UIHelper::CreateStatic(uiXml, "indicator_booster_chem", this);
    m_ind_boost_wound = UIHelper::CreateStatic(uiXml, "indicator_booster_wound", this);
    m_ind_boost_weight = UIHelper::CreateStatic(uiXml, "indicator_booster_weight", this);
    m_ind_boost_health = UIHelper::CreateStatic(uiXml, "indicator_booster_health", this);
    m_ind_boost_power = UIHelper::CreateStatic(uiXml, "indicator_booster_power", this);
    m_ind_boost_rad = UIHelper::CreateStatic(uiXml, "indicator_booster_rad", this);
    m_ind_boost_psy->Show(false);
    m_ind_boost_radia->Show(false);
    m_ind_boost_chem->Show(false);
    m_ind_boost_wound->Show(false);
    m_ind_boost_weight->Show(false);
    m_ind_boost_health->Show(false);
    m_ind_boost_power->Show(false);
    m_ind_boost_rad->Show(false);

	m_ind_battarey_frame		    = UIHelper::CreateStatic(uiXml, "battarey_life_frame", this);
	m_ind_battarey_progress_bar     = UIHelper::CreateProgressBar(uiXml, "battarey_life_progress", this);
	
	xml_init.InitStatic			(uiXml, "invincible_static", 0, &UIInvincibleIcon);
	UIInvincibleIcon.Show		(false);


	if(GameID()==GAME_ARTEFACTHUNT){
		xml_init.InitStatic		(uiXml, "artefact_static", 0, &UIArtefactIcon);
		UIArtefactIcon.Show		(false);
	}
	
	// Flashing icons initialize
	uiXml.SetLocalRoot						(uiXml.NavigateToNode("flashing_icons"));
	InitFlashingIcons						(&uiXml);

	uiXml.SetLocalRoot						(uiXml.GetRoot());
	
	AttachChild								(&UICarPanel);
	xml_init.InitWindow						(uiXml, "car_panel", 0, &UICarPanel);

	AttachChild								(&UIMotionIcon);
	UIMotionIcon.Init						();

	if(IsGameTypeSingle())
	{
		m_artefactPanel->InitFromXML		(uiXml, "artefact_panel", 0);
		this->AttachChild					(m_artefactPanel);	
	}

	AttachChild								(&UIStaticDiskIO);
	UIStaticDiskIO.SetWndRect				(1000,750,16,16);
	UIStaticDiskIO.GetUIStaticItem().SetRect(0,0,16,16);
	UIStaticDiskIO.InitTexture				("ui\\ui_disk_io");
	UIStaticDiskIO.SetOriginalRect			(0,0,32,32);
	UIStaticDiskIO.SetStretchTexture		(TRUE);
		

	HUD_SOUND::LoadSound					("maingame_ui", "snd_new_contact"		, m_contactSnd		, SOUND_TYPE_IDLE);
}

float UIStaticDiskIO_start_time = 0.0f;
void CUIMainIngameWnd::Draw()
{
#ifdef DEBUG
	test_draw				();
#endif
	// show IO icon
	bool IOActive	= (FS.dwOpenCounter>0);
	if	(IOActive)	UIStaticDiskIO_start_time = Device.fTimeGlobal;

	if ((UIStaticDiskIO_start_time+1.0f) < Device.fTimeGlobal)	UIStaticDiskIO.Show(false); 
	else {
		u32		alpha			= clampr(iFloor(255.f*(1.f-(Device.fTimeGlobal-UIStaticDiskIO_start_time)/1.f)),0,255);
		UIStaticDiskIO.Show		( true  ); 
		UIStaticDiskIO.SetColor	(color_rgba(255,255,255,alpha));
	}
	FS.dwOpenCounter = 0;

	if(!IsGameTypeSingle())
	{
		float		luminocity = smart_cast<CGameObject*>(Level().CurrentEntity())->ROS()->get_luminocity();
		float		power = log(luminocity > .001f ? luminocity : .001f)*(1.f/*luminocity_factor*/);
		luminocity	= exp(power);

		static float cur_lum = luminocity;
		cur_lum = luminocity*0.01f + cur_lum*0.99f;
		UIMotionIcon.SetLuminosity((s16)iFloor(cur_lum*100.0f));
	}
	if(!m_pActor) return;

	UIMotionIcon.SetNoise		((s16)(0xffff&iFloor(m_pActor->m_snd_noise*100.0f)));
	CUIWindow::Draw				();
	UIZoneMap->Render			();			

	RenderQuickInfos			();		

	draw_adjust_mode			();
}


void CUIMainIngameWnd::SetMPChatLog(CUIWindow* pChat, CUIWindow* pLog){
	m_pMPChatWnd = pChat;
	m_pMPLogWnd  = pLog;
}

void CUIMainIngameWnd::SetAmmoIcon (const shared_str& sect_name)
{
	if ( !sect_name.size() )
	{
		UIWeaponIcon.Show			(false);
		return;
	};

	UIWeaponIcon.Show			(true);
	//properties used by inventory menu
	float iGridWidth			= pSettings->r_float(sect_name, "inv_grid_width");
	float iGridHeight			= pSettings->r_float(sect_name, "inv_grid_height");

	float iXPos				= pSettings->r_float(sect_name, "inv_grid_x");
	float iYPos				= pSettings->r_float(sect_name, "inv_grid_y");

	UIWeaponIcon.GetUIStaticItem().SetOriginalRect(	(iXPos		 * INV_GRID_WIDTH),
													(iYPos		 * INV_GRID_HEIGHT),
													(iGridWidth	 * INV_GRID_WIDTH),
													(iGridHeight * INV_GRID_HEIGHT));
	UIWeaponIcon.SetStretchTexture(true);

	// now perform only width scale for ammo, which (W)size >2
	// all others ammo (1x1, 1x2) will be not scaled (original picture)
	float w = ((iGridWidth>2)?1.6f:iGridWidth)*INV_GRID_WIDTH*0.9f;
	float h = INV_GRID_HEIGHT*0.9f;//1 cell

	float x = UIWeaponIcon_rect.x1;
	if	(iGridWidth<2)
		x	+= ( UIWeaponIcon_rect.width() - w) / 2.0f;

	UIWeaponIcon.SetWndPos	(x, UIWeaponIcon_rect.y1);
	
	UIWeaponIcon.SetWidth	(w);
	UIWeaponIcon.SetHeight	(h);
};

void CUIMainIngameWnd::Update()
{
#ifdef DEBUG
	test_update();
#endif
	if (m_pMPChatWnd)
		m_pMPChatWnd->Update();
	if (m_pMPLogWnd)
		m_pMPLogWnd->Update();



	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!m_pActor) 
	{
		m_pItem					= NULL;
		m_pWeapon				= NULL;
		m_pGrenade				= NULL;
		CUIWindow::Update		();
		return;
	}

	if( !(Device.dwFrame%30) && IsGameTypeSingle() )
	{
			string256				text_str;
			CPda* _pda	= m_pActor->GetPDA();
			u32 _cn		= 0;
			if(_pda && 0!= (_cn=_pda->ActiveContactsNum()) )
			{
				sprintf_s(text_str, "%d", _cn);
				UIPdaOnline.SetText(text_str);
			}
			else
			{
				UIPdaOnline.SetText("");
			}
	};

	if( !(Device.dwFrame%5) )
	{
		UpdateActiveItemInfo				();
	}

	// Satiety icon
	float satiety = m_pActor->conditions().GetSatiety();
		
	if(satiety>0.75)
	{
		m_ind_starvation->Show(false);
	    m_ind_starvation->ResetClrAnimation();
	}
	else 
	{
		m_ind_starvation->Show(true);
		if(satiety > 0.5f)
		{
			m_ind_starvation->ResetClrAnimation();
			m_ind_starvation->InitTexture("ui_inGame2_circle_hunger_green");
		}
		else if(satiety > 0.25f)
		{
			m_ind_starvation->ResetClrAnimation();
			m_ind_starvation->InitTexture("ui_inGame2_circle_hunger_yellow");
		}
		else
		{
		    m_ind_starvation->InitTexture("ui_inGame2_circle_hunger_red");
			m_ind_starvation->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		}
	}

	// Thirst icon
	float thirst = m_pActor->conditions().GetThirst();
	if(thirst<0.1)
	{
		m_ind_thirst->Show(false);
        m_ind_thirst->ResetClrAnimation();
	}
	else
	{
		m_ind_thirst->Show(true);
		if(thirst <= 0.35f)
		{
			m_ind_thirst->InitTexture("ui_inGame2_circle_thirst_green");
			m_ind_thirst->ResetClrAnimation();
		}
		else if(thirst <= 0.7f)
		{
			m_ind_thirst->InitTexture("ui_inGame2_circle_thirst_yellow");
			m_ind_thirst->ResetClrAnimation();
		}
		else
		{
			m_ind_thirst->InitTexture("ui_inGame2_circle_thirst_red");
		    m_ind_thirst->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		}
	}

	// Psy Health icon
	float psy_health = m_pActor->conditions().GetPsyHealth();
	if(psy_health > 0.75)
	{
		m_ind_psy_health->Show(false);
	    m_ind_psy_health->ResetClrAnimation();
	}
	else
	{
		m_ind_psy_health->Show(true);
		if(psy_health > 0.5f)
		{
			m_ind_psy_health->InitTexture("ui_inGame2_triangle_Psy_green");
			m_ind_psy_health->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		}
		else if(psy_health > 0.25f)
		{
			m_ind_psy_health->InitTexture("ui_inGame2_triangle_Psy_yellow");
			m_ind_psy_health->SetClrLightAnim("ui_medium_blinking_alpha", true, true, false, true);
		}
		else
		{
			m_ind_psy_health->InitTexture("ui_inGame2_triangle_Psy_red");
			m_ind_psy_health->SetClrLightAnim("ui_fast_blinking_alpha", true, true, false, true);
		}
	}

	// Bleeding icon
	float bleeding = m_pActor->conditions().BleedingSpeed();
	if(fis_zero(bleeding, EPS))
	{
		m_ind_bleeding->Show(false);
		m_ind_bleeding->ResetClrAnimation();
	}
	else
	{
		m_ind_bleeding->Show(true);
		if(bleeding<0.35f)
		{
			m_ind_bleeding->InitTexture("ui_inGame2_circle_bloodloose_green");
			m_ind_bleeding->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		}
		else if(bleeding<0.7f)
		{
			m_ind_bleeding->InitTexture("ui_inGame2_circle_bloodloose_yellow");
			m_ind_bleeding->SetClrLightAnim("ui_medium_blinking_alpha", true, true, false, true);
		}
		else
		{
			m_ind_bleeding->InitTexture("ui_inGame2_circle_bloodloose_red");
			m_ind_bleeding->SetClrLightAnim("ui_fast_blinking_alpha", true, true, false, true);
		}
	}
// Radiation icon
	float radiation = m_pActor->conditions().GetRadiation();
	if(fis_zero(radiation, EPS))
	{
		m_ind_radiation->Show(false);
		m_ind_radiation->ResetClrAnimation();
	}
	else
	{
		m_ind_radiation->Show(true);
		if(radiation<0.35f)
		{
			m_ind_radiation->InitTexture("ui_inGame2_circle_radiation_green");
			m_ind_radiation->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		}
		else if(radiation<0.7f)
		{
			m_ind_radiation->InitTexture("ui_inGame2_circle_radiation_yellow");
			m_ind_radiation->SetClrLightAnim("ui_medium_blinking_alpha", true, true, false, true);
		}
		else
		{
			m_ind_radiation->InitTexture("ui_inGame2_circle_radiation_red");
			m_ind_radiation->SetClrLightAnim("ui_fast_blinking_alpha", true, true, false, true);
		}
	}

// Helmet broken icon
	PIItem	helmet = m_pActor->inventory().ItemFromSlot(HELMET_SLOT);
	m_ind_helmet_broken->Show(false);
	m_ind_helmet_broken->ResetClrAnimation();
	if(helmet)
	{
		float condition = helmet->GetCondition();
		if(condition<0.75f)
		{
			m_ind_helmet_broken->Show(true);
			if(condition>0.5f)
			{
				m_ind_helmet_broken->InitTexture("ui_inGame2_circle_Helmetbroken_green");
				m_ind_helmet_broken->ResetClrAnimation();
			}
			else if(condition>0.25f)
			{
				m_ind_helmet_broken->InitTexture("ui_inGame2_circle_Helmetbroken_yellow");
				m_ind_helmet_broken->ResetClrAnimation();
			}
			else
			{
				m_ind_helmet_broken->InitTexture("ui_inGame2_circle_Helmetbroken_red");
				m_ind_helmet_broken->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
			}
		}
	}
// Weapon broken icon
	u16 slot = m_pActor->inventory().GetActiveSlot();
	m_ind_weapon_broken->Show(false);
	m_ind_weapon_broken->ResetClrAnimation();
	if(slot == PISTOL_SLOT || slot == RIFLE_SLOT)
	{
		CWeapon* weapon = smart_cast<CWeapon*>(m_pActor->inventory().ItemFromSlot(slot));
		if(weapon)
		{
			float condition = weapon->GetCondition();
			if(condition<0.75)
			{
				m_ind_weapon_broken->Show(true);
				if(condition>0.5)
				{
					m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_green");
					m_ind_weapon_broken->ResetClrAnimation();
				}
				else if(condition>0.25)
				{
					m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_yellow");
					m_ind_weapon_broken->ResetClrAnimation();
				}
				else
				{
					m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_red");
					m_ind_weapon_broken->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
				}
			}
		}
	}
    // Overweight icon
	float cur_weight = m_pActor->inventory().TotalWeight();
	float max_weight = m_pActor->conditions().GetMaxWalkWeight();
	m_ind_overweight->Show(false);
	m_ind_overweight->ResetClrAnimation();
	if(cur_weight>=max_weight-10.0f && IsGameTypeSingle())
	{
		m_ind_overweight->Show(true);
		if(cur_weight>max_weight)
		{
			m_ind_overweight->InitTexture("ui_inGame2_circle_Overweight_red");
			m_ind_overweight->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		}
		else
		{
			m_ind_overweight->InitTexture("ui_inGame2_circle_Overweight_yellow");
		    m_ind_overweight->ResetClrAnimation();
		}
	}

	PIItem	pItem = m_pActor->inventory().ItemFromSlot(OUTFIT_SLOT);
	m_ind_outfit_broken->Show(false);
	m_ind_outfit_broken->ResetClrAnimation();
	UIArmorBar.Show					(false);
	UIStaticArmor.Show				(false);
	if (pItem)
	{
		float condition = pItem->GetCondition();
		UIArmorBar.Show					(true);
		UIStaticArmor.Show				(true);
		UIArmorBar.SetProgressPos		(condition*100);
	    if (condition<0.75f)
	    {
		  m_ind_outfit_broken->Show(true);
		  if(condition>0.5f)
		  {
			m_ind_outfit_broken->InitTexture("ui_inGame2_circle_Armorbroken_green");
		    m_ind_outfit_broken->ResetClrAnimation();
		  }
		  else if(condition>0.25f)
		  {
			m_ind_outfit_broken->InitTexture("ui_inGame2_circle_Armorbroken_yellow");
			m_ind_outfit_broken->ResetClrAnimation();
		  }
		  else
		  {
			m_ind_outfit_broken->InitTexture("ui_inGame2_circle_Armorbroken_red");
			m_ind_outfit_broken->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
		  }
	   }
	}

	// health&power
	UIHealthBar.SetProgressPos		                (m_pActor->GetfHealth()*100.0f);
	UIMotionIcon.SetPower			                (m_pActor->conditions().GetPower()*100.0f);
	// заряд батареи
	m_ind_battarey_progress_bar->SetProgressPos		(m_pActor->conditions().GetBattareyLife()*100.0f);
	
	UIZoneMap->UpdateRadar			(Device.vCameraPosition);
	float h,p;
	Device.vCameraDirection.getHP	(h,p);
	UIZoneMap->SetHeading			(-h);

	UpdatePickUpItem				();
	CUIWindow::Update				();
}

bool CUIMainIngameWnd::OnKeyboardPress(int dik)
{
		// поддержка режима adjust hud mode
	bool flag = false;
	if (g_bHudAdjustMode)
	{
		CWeaponHUD *pWpnHud = NULL;
		if (m_pWeapon)
		{
			pWpnHud = m_pWeapon->GetHUD();
//			if (!pWpnHud) return false;
		}
		else
			return false;

		Fvector tmpV;

		if (1 == g_bHudAdjustMode) //hud offset and zoom offset
		{
			CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());

			R_ASSERT(pActor);

			// output coordinate info to the console
			if (dik == DIK_P)
			{
				if (!pWpnHud) return false;

				Fmatrix m_offset = pWpnHud->HudOffsetMatrix();

				Msg("Print coordinates:");
				string256 tmpStr;
				sprintf_s(tmpStr, "[%s]",
					pSettings->r_string(*m_pWeapon->cNameSect(),"hud"));
				Log(tmpStr);
					sprintf_s(tmpStr, "position\t\t\t= %f,%f,%f",
						m_offset.c.x,
						m_offset.c.y,
						m_offset.c.z);
				Log(tmpStr);

				Fvector orient;
				m_offset.getHPB (orient);
				orient.mul (180.f/PI);

				sprintf_s(tmpStr, "orientation\t\t\t= %f,%f,%f",
						orient.x,
						orient.y,
						orient.z);
				Log(tmpStr);
						sprintf_s(tmpStr, "zoom_offset\t\t\t= %f,%f,%f",
						pWpnHud->ZoomOffset().x,
						pWpnHud->ZoomOffset().y,
						pWpnHud->ZoomOffset().z);
				Log(tmpStr);
				sprintf_s(tmpStr, "zoom_rotate_x\t\t= %f",
					pWpnHud->ZoomRotateX());
				Log(tmpStr);
				sprintf_s(tmpStr, "zoom_rotate_y\t\t= %f",
					pWpnHud->ZoomRotateY());
				Log(tmpStr);
				flag = true;

			} else if (pActor->IsZoomAimingMode())
			{
				if (!pWpnHud) return false;
				tmpV = pWpnHud->ZoomOffset();

				switch (dik)
				{
				// Rotate +x
				case DIK_K:
					pWpnHud->SetZoomRotateX(pWpnHud->ZoomRotateX() + g_fHudAdjustValue);
					flag = true;
					break;
				// Rotate -x
				case DIK_I:
					pWpnHud->SetZoomRotateX(pWpnHud->ZoomRotateX() - g_fHudAdjustValue);
					flag = true;
					break;
				// Rotate +y
				case DIK_L:
					pWpnHud->SetZoomRotateY(pWpnHud->ZoomRotateY() + g_fHudAdjustValue);
					flag = true;
					break;
				// Rotate -y
				case DIK_J:
					pWpnHud->SetZoomRotateY(pWpnHud->ZoomRotateY() - g_fHudAdjustValue);
					flag = true;
					break;
				// Shift +x
				case DIK_W:
					tmpV.y += g_fHudAdjustValue;
					flag = true;
					break;
				// Shift -y
				case DIK_S:
					tmpV.y -= g_fHudAdjustValue;
					flag = true;
					break;
				// Shift +x
				case DIK_D:
					tmpV.x += g_fHudAdjustValue;
					flag = true;
					break;
				// Shift -x
				case DIK_A:
					tmpV.x -= g_fHudAdjustValue;
					flag = true;
					break;
				// Shift +z
				case DIK_Q:
					tmpV.z += g_fHudAdjustValue;
					flag = true;
					break;
				// Shift -z
				case DIK_E:
					tmpV.z -= g_fHudAdjustValue;
					flag = true;
					break;
				}

				if (tmpV.x || tmpV.y || tmpV.z)
					pWpnHud->SetZoomOffset(tmpV);
			} else {
				if (!pWpnHud) return false;

				Fmatrix m_offset = pWpnHud->HudOffsetMatrix();
				tmpV = pWpnHud->ZoomOffset();

				switch (dik)
				{
				// Rotate +x
				case DIK_L:
					m_offset.k.x += g_fHudAdjustValue;
					flag = true;
					break;
				// Rotate -x
				case DIK_J:
					m_offset.k.x -= g_fHudAdjustValue;
					flag = true;
					break;
				// Rotate +y
				case DIK_I:
					m_offset.k.y += g_fHudAdjustValue;
					flag = true;
					break;
				// Rotate -y
				case DIK_K:
					m_offset.k.y -= g_fHudAdjustValue;
					flag = true;
					break;
				// Shift +x
				case DIK_W:
					m_offset.c.y += g_fHudAdjustValue;
					tmpV.y -= g_fHudAdjustValue;
					flag = true;
					break;
				// Shift -y
				case DIK_S:
					m_offset.c.y -= g_fHudAdjustValue;
					tmpV.y += g_fHudAdjustValue;
					flag = true;
					break;
				// Shift +x
				case DIK_D:
					m_offset.c.x += g_fHudAdjustValue;
					tmpV.x -= g_fHudAdjustValue;
					flag = true;
					break;
				// Shift -x
				case DIK_A:
					m_offset.c.x -= g_fHudAdjustValue;
					tmpV.x += g_fHudAdjustValue;
					flag = true;
					break;
				// Shift +z
				case DIK_Q:
					m_offset.c.z += g_fHudAdjustValue;
					tmpV.z -= g_fHudAdjustValue;
					flag = true;
					break;
				// Shift -z
				case DIK_E:
					m_offset.c.z -= g_fHudAdjustValue;
					tmpV.z += g_fHudAdjustValue;
					flag = true;
					break;
				}

				pWpnHud->SetHudOffsetMatrix(m_offset);
				if (tmpV.x || tmpV.y || tmpV.z)
					pWpnHud->SetZoomOffset(tmpV);
			}
		}
		else if (2 == g_bHudAdjustMode || 5 == g_bHudAdjustMode) //firePoints
		{
			if(TRUE==m_pWeapon->GetHUDmode())
				tmpV = (2 == g_bHudAdjustMode) ? pWpnHud->FirePoint() : pWpnHud->FirePoint2();
			else
				tmpV = (2 == g_bHudAdjustMode) ? m_pWeapon->vLoadedFirePoint : m_pWeapon->vLoadedFirePoint2;

		
			switch (dik)
			{
				// Shift +x
			case DIK_A:
				tmpV.y += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -x
			case DIK_D:
				tmpV.y -= g_fHudAdjustValue;
				flag = true;
				break;
				// Shift +z
			case DIK_Q:
				tmpV.x += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -z
			case DIK_E:
				tmpV.x -= g_fHudAdjustValue;
				flag = true;
				break;
				// Shift +y
			case DIK_S:
				tmpV.z += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -y
			case DIK_W:
				tmpV.z -= g_fHudAdjustValue;
				flag = true;
				break;
				// output coordinate info to the console
			case DIK_P:
				string256 tmpStr;
				if (m_pWeapon)
				{
					sprintf_s(tmpStr, "%s",
						*m_pWeapon->cNameSect());
					Log(tmpStr);
				}

			if(TRUE==m_pWeapon->GetHUDmode())
				Msg("weapon hud section:");
			else
				Msg("weapon section:");

				sprintf_s(tmpStr, "fire_point\t\t\t= %f,%f,%f",
					tmpV.x,
					tmpV.y,
					tmpV.z);
				Log(tmpStr);
				flag = true;
				break;
			}
//#ifdef	DEBUG
			if(TRUE==m_pWeapon->GetHUDmode())
				if (2 == g_bHudAdjustMode) pWpnHud->dbg_SetFirePoint(tmpV);
				else pWpnHud->dbg_SetFirePoint2(tmpV);
			else
			{
				if (2 == g_bHudAdjustMode)  m_pWeapon->vLoadedFirePoint = tmpV;
				else m_pWeapon->vLoadedFirePoint2 = tmpV;
			}
//#endif
		}
		else if (4 == g_bHudAdjustMode) //ShellPoint
		{
			if(TRUE==m_pWeapon->GetHUDmode())
				tmpV = pWpnHud->ShellPoint();
			else
				tmpV = m_pWeapon->vLoadedShellPoint;

			switch (dik)
			{
				// Shift +x
			case DIK_A:
				tmpV.y += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -x
			case DIK_D:
				tmpV.y -= g_fHudAdjustValue;
				flag = true;
				break;
				// Shift +z
			case DIK_Q:
				tmpV.x += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -z
			case DIK_E:
				tmpV.x -= g_fHudAdjustValue;
				flag = true;
				break;
				// Shift +y
			case DIK_S:
				tmpV.z += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -y
			case DIK_W:
				tmpV.z -= g_fHudAdjustValue;
				flag = true;
				break;
				// output coordinate info to the console
			case DIK_P:
				string256 tmpStr;
				if (m_pWeapon)
				{
					sprintf_s(tmpStr, "%s",
						*m_pWeapon->cNameSect());
					Log(tmpStr);
				}

			if(TRUE==m_pWeapon->GetHUDmode())
				Msg("weapon hud section:");
			else
				Msg("weapon section:");

				sprintf_s(tmpStr, "shell_point\t\t\t= %f,%f,%f",
					tmpV.x,
					tmpV.y,
					tmpV.z);
				Log(tmpStr);
				flag = true;
				break;
			}
//#ifdef DEBUG
			if(TRUE==m_pWeapon->GetHUDmode())
				pWpnHud->dbg_SetShellPoint(tmpV);
			else
				m_pWeapon->vLoadedShellPoint = tmpV;

//#endif
		}
		else if (3 == g_bHudAdjustMode) //MissileOffset
		{
			CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());

			R_ASSERT(pActor);

			tmpV = pActor->GetMissileOffset();

			if (!pActor) return false;
			switch (dik)
			{
				// Shift +x
			case DIK_E:
				tmpV.y += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -x
			case DIK_Q:
				tmpV.y -= g_fHudAdjustValue;
				flag = true;
				break;
				// Shift +z
			case DIK_D:
				tmpV.x += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -z
			case DIK_A:
				tmpV.x -= g_fHudAdjustValue;
				flag = true;
				break;
				// Shift +y
			case DIK_W:
				tmpV.z += g_fHudAdjustValue;
				flag = true;
				break;
				// Shift -y
			case DIK_S:
				tmpV.z -= g_fHudAdjustValue;
				flag = true;
				break;
				// output coordinate info to the console
			case DIK_P:
				string256 tmpStr;
				if (m_pWeapon)
				{
					sprintf_s(tmpStr, "%s",
						*m_pWeapon->cNameSect());
					Log(tmpStr);
				}

				sprintf_s(tmpStr, "missile_throw_offset\t\t\t= %f,%f,%f",
					pActor->GetMissileOffset().x,
					pActor->GetMissileOffset().y,
					pActor->GetMissileOffset().z);

				Log(tmpStr);
				flag = true;
				break;
			}

			pActor->SetMissileOffset(tmpV);
		}
		

		if (flag) return true;
	}
#ifdef DEBUG
		if(CAttachableItem::m_dbgItem){
			static float rot_d = deg2rad(0.5f);
			static float mov_d = 0.01f;
			bool shift = !!pInput->iGetAsyncKeyState(DIK_LSHIFT);
			flag = true;
			switch (dik)
			{
				// Shift +x
			case DIK_A:
				if(shift)	CAttachableItem::rot_dx(rot_d);
				else		CAttachableItem::mov_dx(rot_d);
				break;
				// Shift -x
			case DIK_D:
				if(shift)	CAttachableItem::rot_dx(-rot_d);
				else		CAttachableItem::mov_dx(-rot_d);
				break;
				// Shift +z
			case DIK_Q:
				if(shift)	CAttachableItem::rot_dy(rot_d);
				else		CAttachableItem::mov_dy(rot_d);
				break;
				// Shift -z
			case DIK_E:
				if(shift)	CAttachableItem::rot_dy(-rot_d);
				else		CAttachableItem::mov_dy(-rot_d);
				break;
				// Shift +y
			case DIK_S:
				if(shift)	CAttachableItem::rot_dz(rot_d);
				else		CAttachableItem::mov_dz(rot_d);
				break;
				// Shift -y
			case DIK_W:
				if(shift)	CAttachableItem::rot_dz(-rot_d);
				else		CAttachableItem::mov_dz(-rot_d);
				break;

			case DIK_SUBTRACT:
				if(shift)	rot_d-=deg2rad(0.01f);
				else		mov_d-=0.001f;
				Msg("rotation delta=[%f]; moving delta=[%f]",rot_d,mov_d);
				break;
			case DIK_ADD:
				if(shift)	rot_d+=deg2rad(0.01f);
				else		mov_d+=0.001f;
				Msg("rotation delta=[%f]; moving delta=[%f]",rot_d,mov_d);
				break;

			case DIK_P:
				Msg("LTX section [%s]",*CAttachableItem::m_dbgItem->item().object().cNameSect());
				Msg("attach_angle_offset [%f,%f,%f]",VPUSH(CAttachableItem::get_angle_offset()));
				Msg("attach_position_offset [%f,%f,%f]",VPUSH(CAttachableItem::get_pos_offset()));
				break;
			default:
				flag = false;
				break;
			}		
		if(flag)return true;;
		}
#endif		

	if(Level().IR_GetKeyState(DIK_LSHIFT) || Level().IR_GetKeyState(DIK_RSHIFT))
	{
		switch(dik)
		{
		case DIK_NUMPADMINUS:
			UIZoneMap->ZoomOut();
			return true;
			break;
		case DIK_NUMPADPLUS:
			UIZoneMap->ZoomIn();
			return true;
			break;
		}
	}
	else
	{
		switch(dik)
		{
		case DIK_NUMPADMINUS:
			//.HideAll();
			if (!external_icon_ctrl)
				HUD().GetUI()->HideGameIndicators();
			else
				HUD().GetUI()->ShowGameIndicators();

			return true;
			break;
		case DIK_NUMPADPLUS:
			//.ShowAll();
			HUD().GetUI()->ShowGameIndicators();
			return true;
			break;
		}
	}

	return false;
}


void CUIMainIngameWnd::RenderQuickInfos()
{
	if (!m_pActor)
		return;

	static CGameObject *pObject			= NULL;
	LPCSTR actor_action					= m_pActor->GetDefaultActionForObject();
	UIStaticQuickHelp.Show				(NULL!=actor_action);

	if(NULL!=actor_action){
		if(stricmp(actor_action,UIStaticQuickHelp.GetText()))
			UIStaticQuickHelp.SetTextST				(actor_action);
	}

	if (pObject!=m_pActor->ObjectWeLookingAt())
	{
		UIStaticQuickHelp.SetTextST				(actor_action);
		UIStaticQuickHelp.ResetClrAnimation		();
		pObject	= m_pActor->ObjectWeLookingAt	();
	}
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
	VERIFY(news->texture_name.size());

	HUD().GetUI()->m_pMessagesWnd->AddIconedPdaMessage(*(news->texture_name), news->tex_rect, news->SingleLineText(), news->show_time);
}

template <typename T>
bool test_push_window(lua_State *L, CUIWindow *wnd)
{
	T* derived = smart_cast<T*>(wnd);
	if (derived)
	{		
		luabind::detail::convert_to_lua<T*>(L, derived);
		return true;
	}
	return false;
}


void GetStaticRaw(CUIMainIngameWnd *wnd, lua_State *L)
{
	using namespace luabind::detail;			
	// wnd->GetChildWndList();
	shared_str name = lua_tostring(L, 2);
	CUIWindow *child = wnd->FindChild(name, 2); 	
	if (!child)
	{
		CUIStatic *src = &wnd->GetUIZoneMap()->Background();		
		child = src->FindChild(name, 5);
		
		if (!child)
		{
			src = &wnd->GetUIZoneMap()->ClipFrame();
			child = src->FindChild(name, 5);
		}
		if (!child)
		{
			src = &wnd->GetUIZoneMap()->Compass();
			child = src->FindChild(name, 5);
		}
	}

	if (child)
	{	
		// if (test_push_window<CUIMotionIcon>  (L, child)) return;		
		if (test_push_window<CUIProgressBar> (L, child)) return;		
		if (test_push_window<CUIStatic>		 (L, child)) return;
		if (test_push_window<CUIWindow>	     (L, child)) return;						
	}
	lua_pushnil(L);
}


#pragma optimize("s",on)
void CUIMainIngameWnd::script_register(lua_State *L)
{

	module(L)
		[

			class_<CUIMainIngameWnd, CUIWindow>("CUIMainIngameWnd")
			.def("GetStatic",		 &GetStaticRaw, raw(_2)),
			// .def("turn_off_icon", &TurnOffWarningIcon),
			def("get_main_window",   &GetMainIngameWindow) // get_mainingame_window better??
#ifdef SCRIPT_ICONS_CONTROL
			, def("setup_game_icon", &SetupGameIcon)
#endif			
		];

}
#pragma optimize("",on)

void CUIMainIngameWnd::SetFlashIconState_(EFlashingIcons type, bool enable)
{
	// Включаем анимацию требуемой иконки
	FlashingIcons_it icon = m_FlashingIcons.find(type);
	R_ASSERT2(icon != m_FlashingIcons.end(), "Flashing icon with this type not existed");
	icon->second->Show(enable);
}

void CUIMainIngameWnd::InitFlashingIcons(CUIXml* node)
{
	const char * const flashingIconNodeName = "flashing_icon";
	int staticsCount = node->GetNodesNum("", 0, flashingIconNodeName);

	CUIXmlInit xml_init;
	CUIStatic *pIcon = NULL;
	// Пробегаемся по всем нодам и инициализируем из них статики
	for (int i = 0; i < staticsCount; ++i)
	{
		pIcon = xr_new<CUIStatic>();
		xml_init.InitStatic(*node, flashingIconNodeName, i, pIcon);
		shared_str iconType = node->ReadAttrib(flashingIconNodeName, i, "type", "none");

		// Теперь запоминаем иконку и ее тип
		EFlashingIcons type = efiPdaTask;

		if		(iconType == "pda")		type = efiPdaTask;
		else if (iconType == "mail")	type = efiMail;
		else	R_ASSERT(!"Unknown type of mainingame flashing icon");

		R_ASSERT2(m_FlashingIcons.find(type) == m_FlashingIcons.end(), "Flashing icon with this type already exists");

		CUIStatic* &val	= m_FlashingIcons[type];
		val			= pIcon;

		AttachChild(pIcon);
		pIcon->Show(false);
	}
}

void CUIMainIngameWnd::DestroyFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		DetachChild(it->second);
		xr_delete(it->second);
	}

	m_FlashingIcons.clear();
}

void CUIMainIngameWnd::UpdateFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		it->second->Update();
	}
}

void CUIMainIngameWnd::AnimateContacts(bool b_snd)
{
	UIPdaOnline.ResetClrAnimation	();

	if(b_snd)
		HUD_SOUND::PlaySound	(m_contactSnd, Fvector().set(0,0,0), 0, true );

}

void CUIMainIngameWnd::SetPickUpItem	(CInventoryItem* PickUpItem)
{
	if (m_pPickUpItem != PickUpItem)
	{
		m_pPickUpItem = PickUpItem;
		UIPickUpItemIcon.Show(false);
		UIPickUpItemIcon.DetachAll();
	}
};

#include "UICellCustomItems.h"
#include "../pch_script.h"
#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../Actor.h"

typedef CUIWeaponCellItem::eAddonType eAddonType;

CUIStatic* init_addon(
	CUIWeaponCellItem *cell_item,
	LPCSTR sect,
	float scale,
	float scale_x,
	eAddonType idx)
{
	CUIStatic *addon = xr_new<CUIStatic>();
	addon->SetAutoDelete(true);

	auto pos		= cell_item->get_addon_offset(idx); pos.x *= scale*scale_x; pos.y *= scale;
	auto width		= (float)pSettings->r_u32(sect, "inv_grid_width")*INV_GRID_WIDTH;
	auto height		= (float)pSettings->r_u32(sect, "inv_grid_height")*INV_GRID_HEIGHT;
	auto tex_x		= (float)pSettings->r_u32(sect, "inv_grid_x")*INV_GRID_WIDTH;
	auto tex_y		= (float)pSettings->r_u32(sect, "inv_grid_y")*INV_GRID_HEIGHT;

	addon->SetStretchTexture	(true);
	addon->InitTexture			("ui\\ui_icon_equipment");
	addon->SetOriginalRect		(tex_x, tex_y, width, height);
	addon->SetWndRect			(pos.x, pos.y, width*scale*scale_x, height*scale);
	addon->SetColor				(color_rgba(255,255,255,192));

	return addon;
}

void CUIMainIngameWnd::UpdatePickUpItem	()
{
	if (!m_pPickUpItem || !Level().CurrentViewEntity() || Level().CurrentViewEntity()->CLS_ID != CLSID_OBJECT_ACTOR) 
	{
		return;
	};

	if (UIPickUpItemIcon.IsShown() ) return; // Real Wolf: Какой смысл постоянно обновлять? 10.08.2014.

	shared_str sect_name	= m_pPickUpItem->object().cNameSect();

	//properties used by inventory menu
	int m_iGridWidth	= pSettings->r_u32(sect_name, "inv_grid_width");
	int m_iGridHeight	= pSettings->r_u32(sect_name, "inv_grid_height");

	int m_iXPos			= pSettings->r_u32(sect_name, "inv_grid_x");
	int m_iYPos			= pSettings->r_u32(sect_name, "inv_grid_y");

	float scale_x = m_iPickUpItemIconWidth/
		float(m_iGridWidth*INV_GRID_WIDTH);

	float scale_y = m_iPickUpItemIconHeight/
		float(m_iGridHeight*INV_GRID_HEIGHT);

	scale_x = (scale_x>1) ? 1.0f : scale_x;
	scale_y = (scale_y>1) ? 1.0f : scale_y;

	float scale = scale_x<scale_y?scale_x:scale_y;

	UIPickUpItemIcon.GetUIStaticItem().SetOriginalRect(
		float(m_iXPos * INV_GRID_WIDTH),
		float(m_iYPos * INV_GRID_HEIGHT),
		float(m_iGridWidth * INV_GRID_WIDTH),
		float(m_iGridHeight * INV_GRID_HEIGHT));

	UIPickUpItemIcon.SetStretchTexture(true);

	// Real Wolf: Исправляем растягивание. 10.08.2014.
	scale_x = Device.fASPECT/0.75f;

	UIPickUpItemIcon.SetWidth(m_iGridWidth*INV_GRID_WIDTH*scale*scale_x);
	UIPickUpItemIcon.SetHeight(m_iGridHeight*INV_GRID_HEIGHT*scale);

	UIPickUpItemIcon.SetWndPos(m_iPickUpItemIconX + 
		(m_iPickUpItemIconWidth - UIPickUpItemIcon.GetWidth())/2,
		m_iPickUpItemIconY + 
		(m_iPickUpItemIconHeight - UIPickUpItemIcon.GetHeight())/2);

	UIPickUpItemIcon.SetColor(color_rgba(255,255,255,192));

	// Real Wolf: Добавляем к иконке аддоны оружия. 10.08.2014.
	if (auto wpn = m_pPickUpItem->cast_weapon() )
	{
		auto cell_item = xr_new<CUIWeaponCellItem>(wpn);

		if (wpn->SilencerAttachable() && wpn->IsSilencerAttached() )
		{
			auto sil = init_addon(cell_item, *wpn->GetSilencerName(), scale, scale_x, eAddonType::eSilencer);
			UIPickUpItemIcon.AttachChild(sil);
		}

		if (wpn->ScopeAttachable() && wpn->IsScopeAttached() )
		{
			auto scope = init_addon(cell_item, *wpn->GetScopeName(), scale, scale_x, eAddonType::eScope);
			UIPickUpItemIcon.AttachChild(scope);
		}

		if (wpn->GrenadeLauncherAttachable() && wpn->IsGrenadeLauncherAttached() )
		{
			auto launcher = init_addon(cell_item, *wpn->GetGrenadeLauncherName(), scale, scale_x, eAddonType::eLauncher);
			UIPickUpItemIcon.AttachChild(launcher);
		}
		delete_data(cell_item);
	}

	// Real Wolf: Колбек для скриптового добавления своих иконок. 10.08.2014.
	g_actor->callback(GameObject::eUIPickUpItemShowing)(m_pPickUpItem->object().lua_game_object(), &UIPickUpItemIcon);

	UIPickUpItemIcon.Show(true);
};

void CUIMainIngameWnd::UpdateActiveItemInfo()
{
	PIItem item		=  m_pActor->inventory().ActiveItem();
	if(item) 
	{
		xr_string					str_name;
		xr_string					icon_sect_name;
		xr_string					str_count;
		item->GetBriefInfo			(str_name, icon_sect_name, str_count);

		UIWeaponSignAmmo.Show		(true						);
		UIWeaponBack.SetText		(str_name.c_str			()	);
		UIWeaponSignAmmo.SetText	(str_count.c_str		()	);
		SetAmmoIcon					(icon_sect_name.c_str	()	);

		//-------------------
		m_pWeapon = smart_cast<CWeapon*> (item);		
	}else
	{
		UIWeaponIcon.Show			(false);
		UIWeaponSignAmmo.Show		(false);
		UIWeaponBack.SetText		("");
		m_pWeapon					= NULL;
	}
}

void CUIMainIngameWnd::OnConnected()
{
	UIZoneMap->SetupCurrentMap		();
}

void CUIMainIngameWnd::reset_ui()
{
	m_pActor						= NULL;
	m_pWeapon						= NULL;
	m_pGrenade						= NULL;
	m_pItem							= NULL;
	m_pPickUpItem					= NULL;
	UIMotionIcon.ResetVisibility	();
}

#ifdef DEBUG
/*
#include "d3dx9core.h"
#include "winuser.h"
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"d3d9.lib")
ID3DXFont*     g_pTestFont = NULL;
ID3DXSprite*        g_pTextSprite = NULL;   // Sprite for batching draw text calls
*/

/*
#include "UIGameTutorial.h"
#include "../actor_statistic_mgr.h"
CUIGameTutorial* g_tut = NULL;
*/
//#include "../postprocessanimator.h"
//CPostprocessAnimator* pp = NULL;
//extern void create_force_progress();

//#include "UIVotingCategory.h"

//CUIVotingCategory* v = NULL;
#include "UIFrameWindow.h"
CUIFrameWindow*		pUIFrame = NULL;

void test_update()
{
	if(pUIFrame)
		pUIFrame->Update();
}

void test_key	(int dik)
{

	if(dik==DIK_K)
	{
		if(!pUIFrame)
		{
			CUIXml uiXML;
			uiXML.Init(CONFIG_PATH, UI_PATH, "talk.xml");

			pUIFrame					= xr_new<CUIFrameWindow>();
			CUIXmlInit::InitFrameWindow	(uiXML, "frame_window", 0, pUIFrame);
		}else
			xr_delete(pUIFrame);
	}

/*
	if(dik==DIK_K){
		if(g_pTestFont){
			g_pTestFont->Release();
			g_pTestFont = NULL;
			
			g_pTextSprite->Release();
			return;
		}
	HRESULT hr;
	static int _height	= -12;
	static u32 _width	= 0;
	static u32 _weigth	= FW_BOLD;
	static BOOL _italic = FALSE;

    hr = D3DXCreateFont( HW.pDevice, _height, _width, _weigth, 1, _italic, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         "Times New Roman", &g_pTestFont );


	D3DXCreateSprite( HW.pDevice, &g_pTextSprite );

	g_pTestFont->PreloadText("This is a trivial call to ID3DXFont::DrawText", xr_strlen("This is a trivial call to ID3DXFont::DrawText"));

	}
*/
}
/*
D3DCOLOR _clr	= D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f );
LPCSTR _str		= "This is a trivial call to ID3DXFont::DrawText";
int _len		= 43;
*/
void test_draw	()
{
	if(pUIFrame)
		pUIFrame->Draw();
/*
	if(g_pTestFont){

//	g_pTestFont->PreloadText("This is a trivial call to ID3DXFont::DrawText", xr_strlen("This is a trivial call to ID3DXFont::DrawText"));
//	g_pTestFont2->PreloadText("This is a trivial call to ID3DXFont::DrawText", xr_strlen("This is a trivial call to ID3DXFont::DrawText"));

//	IDirect3DTexture9	*T;
//	RECT				R;
//	POINT				P;
//	g_pTestFont2->PreloadGlyphs(0,255);
//	g_pTestFont2->GetGlyphData(50, &T, &R, &P);
//	R_CHK		(D3DXSaveTextureToFile	("x:\\test_font.dds",D3DXIFF_DDS,T,0));

#define DT_TOP                      0x00000000
#define DT_LEFT                     0x00000000
#define DT_CENTER                   0x00000001
#define DT_RIGHT                    0x00000002
#define DT_VCENTER                  0x00000004
#define DT_BOTTOM                   0x00000008
#define DT_WORDBREAK                0x00000010
#define DT_SINGLELINE               0x00000020
#define DT_EXPANDTABS               0x00000040
#define DT_TABSTOP                  0x00000080
#define DT_NOCLIP                   0x00000100
#define DT_EXTERNALLEADING          0x00000200
#define DT_CALCRECT                 0x00000400
#define DT_NOPREFIX                 0x00000800
#define DT_INTERNAL                 0x00001000


		RECT rc;
        g_pTextSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );

		rc.left   = 50;
		rc.top    = 150;
		rc.right  = 250;
		rc.bottom = 180;

		for(int i=0; i<13; ++i){
			g_pTestFont->DrawText( g_pTextSprite, _str, _len, &rc, DT_SINGLELINE, _clr);
			rc.top			+= 30; rc.bottom		+= 30;
		}

		g_pTextSprite->End();

	}
*/
}

#endif

void CUIMainIngameWnd::draw_adjust_mode()
{
	if (g_bHudAdjustMode&&m_pWeapon) //draw firePoint,ShellPoint etc
	{
		CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
		if(!pActor)
			return;

		bool bCamFirstEye = !!m_pWeapon->GetHUDmode();
		string32 hud_view="HUD view";
		string32 _3rd_person_view="3-rd person view";
		CGameFont* F		= UI()->Font()->pFontDI;
		F->SetAligment		(CGameFont::alCenter);
//.		F->SetSizeI			(0.02f);
		F->OutSetI			(0.f,-0.8f);
		F->SetColor			(0xffffffff);
		F->OutNext			("Hud_adjust_mode=%d",g_bHudAdjustMode);
		if(g_bHudAdjustMode==1)
			F->OutNext			("adjusting zoom offset");
		else if(g_bHudAdjustMode==2)
			F->OutNext			("adjusting fire point for %s",bCamFirstEye?hud_view:_3rd_person_view);
		else if(g_bHudAdjustMode==3)
			F->OutNext			("adjusting missile offset");
		else if(g_bHudAdjustMode==4)
			F->OutNext			("adjusting shell point for %s",bCamFirstEye?hud_view:_3rd_person_view);
		else if(g_bHudAdjustMode == 5)
			F->OutNext			("adjusting fire point 2 for %s",bCamFirstEye?hud_view:_3rd_person_view);

		if(bCamFirstEye)
		{
			CWeaponHUD *pWpnHud = NULL;
			pWpnHud = m_pWeapon->GetHUD();

			Fvector FP,SP,FP2;

			CKinematics* V			= smart_cast<CKinematics*>(pWpnHud->Visual());
			VERIFY					(V);
			V->CalculateBones		();

			// fire point&direction
			Fmatrix& fire_mat		= V->LL_GetTransform(u16(pWpnHud->FireBone()));
			Fmatrix& parent			= pWpnHud->Transform	();

			const Fvector& fp		= pWpnHud->FirePoint();
			const Fvector& fp2		= pWpnHud->FirePoint2();
			const Fvector& sp		= pWpnHud->ShellPoint();

			fire_mat.transform_tiny	(FP,fp);
			parent.transform_tiny	(FP);

			fire_mat.transform_tiny	(FP2,fp2);
			parent.transform_tiny	(FP2);

			fire_mat.transform_tiny	(SP,sp);
			parent.transform_tiny	(SP);


			RCache.dbg_DrawAABB(FP,0.01f,0.01f,0.01f,D3DCOLOR_XRGB(255,0,0));
			RCache.dbg_DrawAABB(FP2,0.02f,0.02f,0.02f,D3DCOLOR_XRGB(0,0,255));
			RCache.dbg_DrawAABB(SP,0.01f,0.01f,0.01f,D3DCOLOR_XRGB(0,255,0));
		
		}else{
			Fvector FP = m_pWeapon->get_CurrentFirePoint();
			Fvector FP2 = m_pWeapon->get_CurrentFirePoint2();
			Fvector SP = m_pWeapon->get_LastSP();
			RCache.dbg_DrawAABB(FP,0.01f,0.01f,0.01f,D3DCOLOR_XRGB(255,0,0));
			RCache.dbg_DrawAABB(FP2,0.02f,0.02f,0.02f,D3DCOLOR_XRGB(0,0,255));
			RCache.dbg_DrawAABB(SP,0.02f,0.02f,0.02f,D3DCOLOR_XRGB(0,255,0));
		}
	}
}

void CUIMainIngameWnd::UpdateBoosterIndicators(const xr_map<EBoostParams, SBooster> influences)
{
    m_ind_boost_psy->Show(false);
    m_ind_boost_radia->Show(false);
    m_ind_boost_chem->Show(false);
    m_ind_boost_wound->Show(false);
    m_ind_boost_weight->Show(false);
    m_ind_boost_health->Show(false);
    m_ind_boost_power->Show(false);
    m_ind_boost_rad->Show(false);

    xr_map<EBoostParams, SBooster>::const_iterator b = influences.begin(), e = influences.end();
    for (; b != e; b++)
    {
        switch (b->second.m_type)
        {
        case eBoostHpRestore:
        {
            m_ind_boost_health->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_health->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_health->ResetClrAnimation();
        }
        break;
        case eBoostPowerRestore:
        {
            m_ind_boost_power->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_power->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_power->ResetClrAnimation();
        }
        break;
        case eBoostRadiationRestore:
        {
            m_ind_boost_rad->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_rad->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_rad->ResetClrAnimation();
        }
        break;
        case eBoostBleedingRestore:
        {
            m_ind_boost_wound->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_wound->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_wound->ResetClrAnimation();
        }
        break;
        case eBoostMaxWeight:
        {
            m_ind_boost_weight->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_weight->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_weight->ResetClrAnimation();
        }
        break;
        case eBoostRadiationImmunity:
        case eBoostRadiationProtection:
        {
            m_ind_boost_radia->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_radia->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_radia->ResetClrAnimation();
        }
        break;
        case eBoostTelepaticImmunity:
        case eBoostTelepaticProtection:
        {
            m_ind_boost_psy->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_psy->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_psy->ResetClrAnimation();
        }
        break;
        case eBoostChemicalBurnImmunity:
        case eBoostChemicalBurnProtection:
        {
            m_ind_boost_chem->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_chem->SetClrLightAnim("ui_slow_blinking_alpha", true, true, false, true);
            else
                m_ind_boost_chem->ResetClrAnimation();
        }
        break;
        }
    }
}