////////////////////////////////////////////////////////////////////////////
//	Module 		: HandTorch.cpp
//	Created 	: 15.02.2017
//  Modified 	: 04.03.2017
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

CHandTorch::CHandTorch(void) 
{
	m_class_name				= get_class_name<CHandTorch>(this);
	SetSlot                     (DETECTOR_ONE_SLOT);
	
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_virtual_size (0.1);
	light_render->set_shadow	(true);
	
	light_omni					= ::Render->light_create();
	light_omni->set_type		(IRender_Light::POINT);
	light_omni->set_shadow		(false);
	
	glow_render					= ::Render->glow_create();
	
	fBrightness					= 1.f;
	m_prev_hp.set				(0,0);
	m_delta_h					= 0;
	lanim						= 0;
}

CHandTorch::~CHandTorch(void) 
{
    light_render.destroy ();
	light_omni.destroy   ();
	glow_render.destroy  ();
}

void CHandTorch::Load(LPCSTR section) 
{
	inherited::Load		(section);

	light_trace_bone	    = pSettings->r_string(section, "light_trace_bone");
	VERIFY(light_trace_bone!=BI_NONE);
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

	Fcolor clr_o			= pUserData->r_fcolor				("torch_definition", "omni_color_r2");
	float range_o			= pUserData->r_float				("torch_definition", "omni_range_r2");
	light_omni->set_color	(clr_o);
	light_omni->set_range	(range_o);

	light_render->set_cone	(deg2rad(pUserData->r_float			("torch_definition","spot_angle")));
	light_render->set_texture(pUserData->r_string				("torch_definition","spot_texture"));
	
	light_render->set_volumetric(pUserData->r_bool 				("torch_definition","volumetric_r2"));
	light_render->set_volumetric_quality(pUserData->r_float 	("torch_definition","volumetric_quality_r2"));
	light_render->set_volumetric_intensity(pUserData->r_float 	("torch_definition","volumetric_intensity_r2"));
	light_render->set_volumetric_distance(pUserData->r_float 	("torch_definition","volumetric_distance_r2"));

	glow_render->set_texture(pUserData->r_string				("torch_definition","glow_texture"));
	glow_render->set_color	(clr);
	glow_render->set_radius	(pUserData->r_float					("torch_definition","glow_radius"));

	return					(TRUE);
}

void CHandTorch::OnStateSwitch		(u32 S)
{
	inherited::OnStateSwitch	(S);
	switch(S){
	case eHiding:
		{
			    Switch(false);
			    VERIFY(GetState()==eHiding);
	            m_pHUD->animPlay(random_anim(m_anim_hide), TRUE, this, GetState());
		}break;
	};
}

void CHandTorch::Switch(bool turn)
{
	light_render->set_active(turn);
	light_omni->set_active(turn);
	glow_render->set_active(turn);
	CKinematics* pVisual = smart_cast<CKinematics*>(m_pHUD->Visual());
	u16 bi				 = pVisual->LL_BoneID(light_trace_bone);
	if (turn==true)
	  pVisual->LL_SetBoneVisible(bi,	TRUE,	TRUE);
	else
      pVisual->LL_SetBoneVisible(bi,	FALSE,	TRUE);
}

void CHandTorch::OnAnimationEnd		(u32 state)
{
	
	if(state == eShowing && !light_render->get_active()) 
	{
		Switch(true);
	} 

	inherited::OnAnimationEnd(state);
}

void CHandTorch::UpdateCL() // Kondr48, механизм абсолютно не катит для худового фонаря - тупо не красиво.
{
	inherited::UpdateCL();

	CKinematics* pVisual				= smart_cast<CKinematics*>(m_pHUD->Visual());
	u16	bone_id			= pVisual->LL_BoneID(light_trace_bone);
	CBoneInstance& BI = smart_cast<CKinematics*>(m_pHUD->Visual())->LL_GetBoneInstance(bone_id);

	Fmatrix M;
	M.mul(XFORM(),BI.mTransform);
	light_render->set_rotation	(M.k,M.i);
	light_render->set_position	(M.c);
}