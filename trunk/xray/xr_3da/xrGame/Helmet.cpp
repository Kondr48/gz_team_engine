#include "stdafx.h"
#include "helmet.h"
#include "Actor.h"
#include "inventory_space.h"
#include "Inventory.h"

CHelmet::CHelmet(void) 
{
	m_class_name				= get_class_name<CHelmet>(this);
	m_NightVisionDevice         = NULL;
	SetSlot (HELMET_SLOT);
}

CHelmet::~CHelmet(void) 
{
    xr_delete(m_NightVisionDevice);
}

void CHelmet::Load(LPCSTR section) 
{
	inherited::Load		(section);

	bIsNightvisionAvaliable	= !!READ_IF_EXISTS(pSettings, r_bool, section, "nightvision_avaliable", true);

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
		}
	}
};

void CHelmet::UpdateCL()
{
	inherited::UpdateCL();
	if (H_Parent() && H_Parent()->ID() == Actor()->ID() && m_NightVisionDevice)
		m_NightVisionDevice->UpdateSwitchNightVision();
}