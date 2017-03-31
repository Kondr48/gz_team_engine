////////////////////////////////////////////////////////////////////////////
//	Module 		: HandTorch.cpp
//	Created 	: 15.03.2017
//  Modified 	: 28.03.2017
//	Author		: Kondr48
//	Description : Ручной фонарь
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "handtorch.h"
#include "PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "game_cl_base.h"
#include "Actor.h"
#include "Entity.h"
#include "../skeletoncustom.h"
#include "../camerabase.h"
#include "../LightAnimLibrary.h"
#include "xrserver_objects_alife_items.h"
#include "ai_sounds.h"

CHandTorch::CHandTorch(void) 
{
	m_class_name				= get_class_name<CHandTorch>(this);
	SetSlot                     (DETECTOR_ONE_SLOT);
	
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_virtual_size (0.1);
	light_render->set_shadow	(true);
		
	//glow_render					= ::Render->glow_create();
	
	fBrightness					= 1.f;
	lanim						= 0;
    battarey_life               = 1.f;
	battarey_power              = 1.f;
	m_switched_on               = false;
}

CHandTorch::~CHandTorch(void) 
{
    light_render.destroy ();
	//glow_render.destroy  ();
	HUD_SOUND::DestroySound(m_snd_switch);
}

void CHandTorch::Load(LPCSTR section) 
{
	inherited::Load		(section);

	animGet				(m_anim_switch,					pSettings->r_string(*hud_sect,"anim_switch"));
	
	HUD_SOUND::LoadSound(section, "snd_switch", m_snd_switch, ESoundTypes(SOUND_TYPE_ITEM_USING));
	battarey_life    = pSettings->r_float(section,  "battarey_life");
	light_trace_bone = pSettings->r_string(section, "light_trace_bone"); 
	VERIFY(light_trace_bone != BI_NONE);
}

BOOL CHandTorch::net_Spawn(CSE_Abstract* DC) 
{	
	if (!inherited::net_Spawn(DC))
		return				(FALSE);

	CKinematics* K			= smart_cast<CKinematics*>(Visual());
	CInifile* pUserData		= K->LL_UserData(); 

	lanim					= LALib.FindItem(pUserData->r_string("torch_definition","color_animator"));

	Fcolor clr				= pUserData->r_fcolor				("torch_definition", "color_r2");
	fBrightness				= clr.intensity();
	float range				= pUserData->r_float				("torch_definition", "range_r2");
	light_render->set_color	(clr);
	light_render->set_range	(range);

	light_render->set_cone	(deg2rad(pUserData->r_float			("torch_definition","spot_angle")));
	light_render->set_texture(pUserData->r_string				("torch_definition","spot_texture"));
	
	light_render->set_volumetric(pUserData->r_bool 				("torch_definition","volumetric_r2"));
	light_render->set_volumetric_quality(pUserData->r_float 	("torch_definition","volumetric_quality_r2"));
	light_render->set_volumetric_intensity(pUserData->r_float 	("torch_definition","volumetric_intensity_r2"));
	light_render->set_volumetric_distance(pUserData->r_float 	("torch_definition","volumetric_distance_r2"));

	/*glow_render->set_texture(pUserData->r_string				("torch_definition","glow_texture"));
	glow_render->set_color	(clr);
	glow_render->set_radius	(pUserData->r_float					("torch_definition","glow_radius"));*/

	return					(TRUE);
}

void CHandTorch::Hide()
{
	SwitchState(eTurnOff);
}

void CHandTorch::OnStateSwitch		(u32 S)
{
	inherited::OnStateSwitch	(S);
	switch(S){
	case eTurnOff:
		{
			    VERIFY(GetState() == eTurnOff);
	            PlaySwitch();
		}break;
	case eTurnOn:
		{
			    VERIFY(GetState() == eTurnOn);
	            PlaySwitch();
		}break;
	};
}

void CHandTorch::PlaySwitch()
{
	m_pHUD->animPlay(random_anim(m_anim_switch), TRUE, this, GetState());
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	PlaySound(m_snd_switch, pActor->Position());
}

void CHandTorch::Switch(bool turn)
{
	light_render->set_active(turn);
	//glow_render->set_active(turn);
	CKinematics* pVisual = smart_cast<CKinematics*>(m_pHUD->Visual());
	u16 bi				 = pVisual->LL_BoneID(light_trace_bone);
	if (turn==true)
	  pVisual->LL_SetBoneVisible(bi,	TRUE,	TRUE);
	else
      pVisual->LL_SetBoneVisible(bi,	FALSE,	TRUE);
	m_switched_on = turn;
}

void CHandTorch::OnAnimationEnd		(u32 state)
{
	switch (state)
	{
	case eHiding:
		{
			SwitchState(eHidden);
		}break;
	case eShowing:
		{
			SwitchState(eTurnOn);
		}break;
	case eTurnOn:
		{
			Switch(true);
			SwitchState(eIdle);
		}break;
	case eTurnOff:
		{
			Switch(false);
			SwitchState(eHiding);
		}break;
	};
}

void CHandTorch::UpdateCL()
{
	inherited::UpdateCL();
	
	CKinematics* pVisual				= smart_cast<CKinematics*>(m_pHUD->Visual());
	u16	bone_id			= pVisual->LL_BoneID(light_trace_bone);
	CBoneInstance& BI = smart_cast<CKinematics*>(m_pHUD->Visual())->LL_GetBoneInstance(bone_id);

	Fmatrix M;
	M.mul(m_pHUD->Transform(), BI.mTransform);
	light_render->set_rotation	(M.k,M.i);
	/*glow_render->set_direction(M.k);
	glow_render->set_position	(M.c);*/
	light_render->set_position	(M.c);

	UpdatePowerLoss();
}

void CHandTorch::UpdatePowerLoss()
{
	m_pActor = smart_cast<CActor *>(H_Parent());
	if (!((m_pActor) ? true : false)) {battarey_power = 1.0f; return;}
    
	battarey_power -= (1/(battarey_life * 3600)) * m_fDeltaTime;
	clamp(battarey_power, 0.0f, 1.0f);

	if (battarey_power <= 0 && m_switched_on == true)
        Switch(false);
}

void CHandTorch::SetBattareyPower(float x)
{
	battarey_power = x;
	clamp(battarey_power, 0.f, 1.f);
}

void CHandTorch::SetBattareyLife(float x)
{
    battarey_life = x;
	clamp(battarey_life, 0.f, 1.f);
}