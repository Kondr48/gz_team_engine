////////////////////////////////////////////////////////////////////////////
//	Module 		: Nightvision.cpp
//	Created 	: 03.01.2016
//  Modified 	: 03.01.2016
//	Author		: Alexander Petrov
//	Description : Night vision device extracted from CTorch 
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "pch_script.h"
#include "Nightvision.h"
#include "HUDManager.h"
#include "level.h"
#include "Actor.h"
#include "actorEffector.h"
#include "ai_sounds.h"
#include "inventory.h"
#include "../LightAnimLibrary.h"
#include "xrserver_objects_alife_items.h"

#pragma optimize("gyts", off)

CNightVisionDevice::CNightVisionDevice()
{
}

CNightVisionDevice::~CNightVisionDevice()
{
	HUD_SOUND::DestroySound	(m_OnSnd);
	HUD_SOUND::DestroySound	(m_OffSnd);
	HUD_SOUND::DestroySound	(m_IdleSnd);
	HUD_SOUND::DestroySound	(m_BrokenSnd);
}

void CNightVisionDevice::InitDevice(LPCSTR section)
{
	if (!pSettings->line_exist(section, "night_vision_device")) return;
	m_bEnabled = true;
	m_DeviceSect = pSettings->r_string(section, "night_vision_device");  // section of device config
	LoadConfig(*m_DeviceSect);
}

void CNightVisionDevice::LoadConfig(LPCSTR section)
{
	R_ASSERT2 (pSettings->section_exist(section), section);

	m_DeviceSect = section;
	m_EffectorSect = pSettings->r_string(section, "effector");

	if(pSettings->line_exist(section, "sound_on") )
	{		
		HUD_SOUND::LoadSound(section, "sound_on",     m_OnSnd,     SOUND_TYPE_ITEM_USING);
		HUD_SOUND::LoadSound(section, "sound_off",    m_OffSnd,    SOUND_TYPE_ITEM_USING);
		HUD_SOUND::LoadSound(section, "sound_idle",   m_IdleSnd,   SOUND_TYPE_ITEM_USING);
		HUD_SOUND::LoadSound(section, "sound_broken", m_BrokenSnd, SOUND_TYPE_ITEM_USING);
	}
}

void CNightVisionDevice::SwitchNightVision()
{
	if (OnClient()) return;
	SwitchNightVision(!m_bPowered);	
}

void CNightVisionDevice::SwitchNightVision(bool vision_on)
{
	if (m_bPowered == vision_on) return;

	
	if(vision_on)
	{
		m_bPowered = m_bEnabled;
	}
	else
	{
		m_bPowered = false;
	}

	CActor *pA = Actor();

	if(!pA)					return;	
	bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());

	// �������� �� ��������� �������
	LPCSTR disabled_names	= pSettings->r_string(*m_DeviceSect,"disabled_maps");
	LPCSTR curr_map			= *Level().name();
	u32 cnt					= _GetItemCount(disabled_names);
	bool b_allow			= true;
	string512				tmp;
	for(u32 i=0; i<cnt; ++i) {
		_GetItem(disabled_names, i, tmp);
		if(0==stricmp(tmp, curr_map)){
			b_allow = false;
			break;
		}
	}
		
	if (m_EffectorSect.size() && !b_allow) {
		HUD_SOUND::PlaySound(m_BrokenSnd, pA->Position(), pA, bPlaySoundFirstPerson);
		return;
	}

	if(m_bPowered) {
		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
		if(!pp) {
			if (m_EffectorSect.size())
			{
				AddEffector(pA, effNightvision, m_EffectorSect);
				HUD_SOUND::PlaySound(m_OnSnd, pA->Position(), pA, bPlaySoundFirstPerson);
				HUD_SOUND::PlaySound(m_IdleSnd, pA->Position(), pA, bPlaySoundFirstPerson, true);
			}
		}
	} else {
 		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
		if(pp){
			pp->Stop			(1.0f);
			HUD_SOUND::PlaySound(m_OffSnd, pA->Position(), pA, bPlaySoundFirstPerson);
			HUD_SOUND::StopSound(m_IdleSnd);
		}
	}
}


void CNightVisionDevice::UpdateSwitchNightVision   ()
{
	if (!m_bEnabled)
	{
		if (m_bPowered)	TurnOff();
		return;
	}
	if (OnClient()) return;
}

CPortableNVD::CPortableNVD()
{
	SetSlot(PNV_SLOT);
	need_slot = true;
}

CPortableNVD::~CPortableNVD()
{
}

BOOL CPortableNVD::net_Spawn(CSE_Abstract* DC)
{
	SetSlot					(TORCH_SLOT);
	CSE_ALifeItemNVD		*nvd	= smart_cast<CSE_ALifeItemNVD*>(DC);
	cNameVisual_set			(nvd->get_visual());

	R_ASSERT				(!CFORM());
	R_ASSERT				(smart_cast<CKinematics*>(Visual()));
	collidable.model		= xr_new<CCF_Skeleton>	(this);

	if (!inherited::net_Spawn(DC))
		return				(FALSE);
	
	m_bEnabled = nvd->m_enabled;
	SwitchNightVision (nvd->m_active);

	return TRUE;
}
void CPortableNVD::net_Destroy() 
{
	TurnOff();
}

void CPortableNVD::net_Export(NET_Packet& P) 
{
	inherited::net_Export		(P);
	u8 flags = 0;
	if (m_bEnabled) flags |= eEnabled;
	if (m_bPowered)	   flags |= eActive;
	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA)
	{
		if (pA->attached(this))
			flags |= eAttached;
	}
	P.w_u8(flags);
}			

void CPortableNVD::net_Import(NET_Packet& P)
{
	inherited::net_Import		(P);
	u8 flags = P.r_u8();
	m_bEnabled = !!(flags & eEnabled);
	SwitchNightVision ( !!(flags & eActive) );
}			

void CPortableNVD::OnH_A_Chield()
{


}
void CPortableNVD::OnH_B_Independent(bool just_before_destroy)
{
	TurnOff	();
	HUD_SOUND::StopSound		(m_OnSnd);
	HUD_SOUND::StopSound		(m_OffSnd);
	HUD_SOUND::StopSound		(m_IdleSnd);
}

void CPortableNVD::UpdateCL()
{
	UpdateSwitchNightVision		();
}

#define ATTACHABLE_ITEM CPortableNVD  // ������ ������� - ������
#include "can_be_attached.inc"
#undef ATTACHABLE_ITEM
