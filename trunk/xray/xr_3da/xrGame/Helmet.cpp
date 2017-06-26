#include "stdafx.h"
#include "helmet.h"
#include "Actor.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "ai_sounds.h"

#ifndef FIRE_WOUND_HIT_FIXED
  #include "BoneProtections.h"
#endif

#include "../xr_ioconsole.h"

CHelmet::CHelmet(void) 
{
	m_class_name				= get_class_name<CHelmet>(this);

	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for(int i=0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 1.0f;

#ifndef FIRE_WOUND_HIT_FIXED
	m_boneProtection = xr_new<SBoneProtections>();
#endif

	m_flags.set(FUsingCondition, TRUE);
	SetSlot (HELMET_SLOT);
	m_NightVisionDevice         = NULL;
	helmet_is_dressed           = false;
}

CHelmet::~CHelmet(void) 
{
#ifndef FIRE_WOUND_HIT_FIXED
	xr_delete(m_boneProtection);
#endif
	xr_delete(m_NightVisionDevice);
	HUD_SOUND::DestroySound(m_breath_sound);
}

void CHelmet::Load(LPCSTR section) 
{
	CHudItemObject::Load		(section);

	bIsNightvisionAvaliable	= !!READ_IF_EXISTS(pSettings, r_bool, section, "nightvision_avaliable", true);

	HUD_SOUND::LoadSound(section, "breath_sound", m_breath_sound, SOUND_TYPE_ITEM_USING);

	glass_texture = pSettings->r_string(section, "glass_texture");	

	animGet				(m_anim_show,			pSettings->r_string(*hud_sect,"anim_show"));
	animGet				(m_anim_hide,		    pSettings->r_string(*hud_sect,"anim_hide"));
	animGet				(m_anim_filtr,			pSettings->r_string(*hud_sect,"anim_filtr"));

	m_HitTypeProtection[ALife::eHitTypeBurn]		= pSettings->r_float(section,"burn_protection");
	m_HitTypeProtection[ALife::eHitTypeStrike]		= pSettings->r_float(section,"strike_protection");
	m_HitTypeProtection[ALife::eHitTypeShock]		= pSettings->r_float(section,"shock_protection");
	m_HitTypeProtection[ALife::eHitTypeWound]		= pSettings->r_float(section,"wound_protection");
	m_HitTypeProtection[ALife::eHitTypeRadiation]	= pSettings->r_float(section,"radiation_protection");
	m_HitTypeProtection[ALife::eHitTypeTelepatic]	= pSettings->r_float(section,"telepatic_protection");
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]= pSettings->r_float(section,"chemical_burn_protection");
	m_HitTypeProtection[ALife::eHitTypeExplosion]	= pSettings->r_float(section,"explosion_protection");
	m_HitTypeProtection[ALife::eHitTypeFireWound]	= pSettings->r_float(section,"fire_wound_protection");
	m_HitTypeProtection[ALife::eHitTypePhysicStrike]= READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection", 0.0f);

	m_fHealthRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float,    section, "health_restore_speed",    0.0f );
	m_fRadiationRestoreSpeed	= READ_IF_EXISTS(pSettings, r_float,    section, "radiation_restore_speed", 0.0f );
	m_fSatietyRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float,    section, "satiety_restore_speed",   0.0f );
	m_fThirstRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float,    section, "thirst_restore_speed",    0.0f );
	m_fPowerRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float,    section, "power_restore_speed",     0.0f );
	m_fBleedingRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float,    section, "bleeding_restore_speed",  0.0f );

	LPCSTR nvd_sect = NULL;
	if (pSettings->line_exist(section, "night_vision_device") && bIsNightvisionAvaliable)
		nvd_sect = pSettings->r_string(section, "night_vision_device");

	if (nvd_sect)
		{
		   if (xr_strlen(nvd_sect) && pSettings->section_exist(nvd_sect))
		     {
		        m_NightVisionDevice = xr_new<CNightVisionDevice>();
		        m_NightVisionDevice->LoadConfig(nvd_sect);
		     }
		   else
		        Msg("!#ERROR: invalid night_vision_device '%s' for helmet '%s' ", nvd_sect, section);
		}	
}

BOOL CHelmet::net_Spawn(CSE_Abstract* DC) 
{
	BOOL result = inherited::net_Spawn(DC);
	return result;	
}

void CHelmet::OnMoveToSlot()
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			PIItem pNVD = pActor->inventory().ItemFromSlot(NIGHTVISION_SLOT);
			if(pNVD && !bIsNightvisionAvaliable)
				pActor->inventory().Ruck(pNVD);
		}
	}
};

void CHelmet::OnMoveToRuck()
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			if (m_NightVisionDevice)
				m_NightVisionDevice->SwitchNightVision(false);
			if (helmet_is_dressed)
				HelmetUndressing();
		}
	}
};

void CHelmet::UpdateCL()
{
	inherited::UpdateCL();
	if (H_Parent() && H_Parent()->ID() == Actor()->ID() && m_NightVisionDevice)
		m_NightVisionDevice->UpdateSwitchNightVision();
}

void CHelmet::Show()
{
    CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
    LastActiveSlot = pActor->inventory().GetActiveSlot();

	if (helmet_is_dressed)
	   SwitchState(eHelmetUndressing);
	else
	   SwitchState(eHelmetDressing);
}

void CHelmet::OnStateSwitch		(u32 S)
{
	CHudItemObject::OnStateSwitch	(S);
	switch(S){
	case eHelmetDressing:
		{
			    m_bPending = true;
				m_pHUD->animPlay		(random_anim(m_anim_show), FALSE, this, GetState());
		}break;
	case eHelmetUndressing:
		{
				m_bPending = true;
				HelmetUndressing();
				m_pHUD->animPlay		(random_anim(m_anim_hide), FALSE, this, GetState());
		}break;
	case eHiding:
		{
				SwitchState(eHidden);
		}break;
	};
}

void CHelmet::OnAnimationEnd		(u32 state)
{
	switch (state)
	{
	case eHelmetDressing:
		{
			RestoreLastActiveSlot();
			HelmetDressing();
			m_bPending = false;
		}break;
	case eHelmetUndressing:
		{
            RestoreLastActiveSlot();
			m_bPending = false;
		}break;
	};
}

void CHelmet::RestoreLastActiveSlot()
{
	CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
	pActor->inventory().Activate(LastActiveSlot);
	LastActiveSlot              = NO_ACTIVE_SLOT;
}

void CHelmet::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);
	HUD_SOUND::StopSound		(m_breath_sound);
}

void CHelmet::onMovementChanged(ACTOR_DEFS::EMoveCommand cmd) {}

void CHelmet::save(NET_Packet &output_packet)
{
	inherited::save		(output_packet);
	save_data			(helmet_is_dressed, output_packet);
}

void CHelmet::load(IReader &input_packet)
{
	inherited::load		(input_packet);
	load_data			(helmet_is_dressed, input_packet);
}

void CHelmet::HelmetDressing()
{
   helmet_is_dressed = true;
   CActor* pA = Actor();
   if (!pA) return;
   bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());
   HUD_SOUND::PlaySound(m_breath_sound, pA->Position(), pA, bPlaySoundFirstPerson, true);
   Console->Execute("r2_visor 1, 1, 1"); //Kondr48: дикий костыль

#ifndef FIRE_WOUND_HIT_FIXED
   if(pSettings->line_exist(cNameSect(),"bones_koeff_protection"))
		m_boneProtection->reload( pSettings->r_string(cNameSect(),"bones_koeff_protection"), smart_cast<CKinematics*>(pA->Visual()) );
#endif
}

void CHelmet::HelmetUndressing()
{
   helmet_is_dressed = false;	
   Console->Execute("r2_visor 0, 0, 0"); //Kondr48: дикий костыль
   Console->Execute("r2_rain_drops_intensity 0");
   HUD_SOUND::StopSound(m_breath_sound);
}

void CHelmet::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power *= m_HitTypeK[hit_type];
	ChangeCondition(-hit_power);
}

float CHelmet::GetDefHitTypeProtection(ALife::EHitType hit_type)
{
	return 1.0f - m_HitTypeProtection[hit_type]*GetCondition();
}

float CHelmet::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
	float fBase = m_HitTypeProtection[hit_type]*GetCondition();
#ifndef FIRE_WOUND_HIT_FIXED
	float bone = m_boneProtection->getBoneProtection(element);
	return 1.0f - fBase*bone;
#else
	return 1.0f - fBase;
#endif
}

#ifndef FIRE_WOUND_HIT_FIXED
float	CHelmet::HitThruArmour(float hit_power, s16 element, float AP)
{
	float BoneArmour = m_boneProtection->getBoneArmour(element)*GetCondition()*(1-AP);	
	float NewHitPower = hit_power - BoneArmour;
	if (NewHitPower < hit_power*m_boneProtection->m_fHitFrac) return hit_power*m_boneProtection->m_fHitFrac;
	return NewHitPower;
};
#endif