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

static const float		TIME_2_HIDE					= 5.f;
static const float		TORCH_INERTION_CLAMP		= PI_DIV_6;
static const float		TORCH_INERTION_SPEED_MAX	= 7.5f;
static const float		TORCH_INERTION_SPEED_MIN	= 0.5f;
static const Fvector	TORCH_OFFSET				= {-0.2f,+0.1f,-0.3f};
static const Fvector	OMNI_OFFSET					= {-0.2f,+0.1f,-0.1f};
static const float		OPTIMIZATION_DISTANCE		= 100.f;

CHandTorch::CHandTorch(void) 
{
	m_class_name				= get_class_name<CHandTorch>(this);
	SetSlot                     (DETECTOR_SLOT);
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
	
	virtual_size		    = pSettings->r_float (section, "virtual_size");
	shadow		            = !!READ_IF_EXISTS(pSettings, r_bool, section, "shadow", false);
	range              		= pSettings->r_float (section, "range");
	angle		            = pSettings->r_float (section, "angle");
	light_texture    		= pSettings->r_string(section, "spot_texture");
	clr				        = pSettings->r_fcolor(section, "color");
	clr_o                   = pSettings->r_fcolor(section, "omni_color");
	range_o                 = pSettings->r_float (section, "omni_range");
	glow_texture	        = pSettings->r_string(section, "glow_texture");
	glow_radius	            = pSettings->r_float (section, "glow_radius");
	color_animator          = pSettings->r_string(section, "color_animator");

	volumetric_light		= !!READ_IF_EXISTS(pSettings, r_bool, section, "volumetric_light", false);
	if (volumetric_light)
	{
		volumetric_light_quality    = pSettings->r_float (section, "volumetric_light_quality");
		volumetric_light_intensity  = pSettings->r_float (section, "volumetric_light_intensity");
		volumetric_light_distance   = pSettings->r_float (section, "volumetric_light_distance");
	}
}

BOOL CHandTorch::net_Spawn(CSE_Abstract* DC) 
{
	BOOL result = inherited::net_Spawn(DC);

	light_render = ::Render->light_create();
	light_render->set_virtual_size(virtual_size);
	light_render->set_shadow(shadow);
	light_render->set_type(IRender_Light::SPOT);
	light_render->set_range(range);
	fBrightness = clr.intensity();
	light_render->set_color(clr);
	light_render->set_cone(deg2rad(angle));
	light_render->set_texture(light_texture.c_str());
	
	light_omni = ::Render->light_create();
	light_omni->set_type(IRender_Light::POINT);
	light_omni->set_shadow(false);
	light_omni->set_color(clr_o);
	light_omni->set_range(range_o);

	glow_render = ::Render->glow_create();
	glow_render->set_texture(glow_texture.c_str());
	glow_render->set_color(clr);
	glow_render->set_radius(glow_radius);

	lanim = LALib.FindItem(color_animator.c_str());

	if (lanim)
		Msg("Анимация света загружена");
	
	if (volumetric_light) // Включим объемный свет, если он есть.
	{
			light_render->set_volumetric(true);
	        light_render->set_volumetric_quality(volumetric_light_quality);
	        light_render->set_volumetric_intensity(volumetric_light_intensity);
	        light_render->set_volumetric_distance(volumetric_light_distance);
			Msg("Объемный свет типа установлен");
	}

	return result;	
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
	
	Fvector                 light_bone_pos;
	light_bone_pos = pVisual->LL_GetBonePosition(pVisual, light_trace_bone.c_str());
	Msg("позиция кости x = %f, y = %f, z = %f", light_bone_pos.x, light_bone_pos.y, light_bone_pos.z);

	u16 bi								= pVisual->LL_BoneID(light_trace_bone);
	CBoneInstance			&BI = smart_cast<CKinematics*>(m_pHUD->Visual())->LL_GetBoneInstance(bi);
	Fmatrix					M;

	if (H_Parent()) 
	{
		CActor*			actor = smart_cast<CActor*>(H_Parent());
		if (actor)		smart_cast<CKinematics*>(m_pHUD->Visual())->CalculateBones_Invalidate	();

		if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition)<_sqr(OPTIMIZATION_DISTANCE) || GameID() != GAME_SINGLE) {
			// near camera
			smart_cast<CKinematics*>(m_pHUD->Visual())->CalculateBones	();
			M.mul_43				(XFORM(),BI.mTransform);
		} else {
			// approximately the same
			M		= H_Parent()->XFORM		();
			H_Parent()->Center				(M.c);
			M.c.y	+= H_Parent()->Radius	()*2.f/3.f;
		}

		if (actor) 
		{
			m_prev_hp.x		= angle_inertion_var(m_prev_hp.x,-actor->cam_FirstEye()->yaw,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,Device.fTimeDelta);
			m_prev_hp.y		= angle_inertion_var(m_prev_hp.y,-actor->cam_FirstEye()->pitch,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,Device.fTimeDelta);

			Fvector			dir,right,up;	
			dir.setHP		(m_prev_hp.x+m_delta_h,m_prev_hp.y);
			Fvector::generate_orthonormal_basis_normalized(dir,up,right);


			if (true)
			{
				Fvector offset				= M.c; 
				offset.mad					(M.i,TORCH_OFFSET.x);
				offset.mad					(M.j,TORCH_OFFSET.y);
				offset.mad					(M.k,TORCH_OFFSET.z);
				light_render->set_position	(offset);

				if(false)
				{
					offset						= M.c; 
					offset.mad					(M.i,OMNI_OFFSET.x);
					offset.mad					(M.j,OMNI_OFFSET.y);
					offset.mad					(M.k,OMNI_OFFSET.z);
					light_omni->set_position	(offset);
				}
			}
			glow_render->set_position	(M.c);

			if (true)
			{
				light_render->set_rotation	(dir, right);
				
				if(false)
				{
					light_omni->set_rotation	(dir, right);
				}
			}
			glow_render->set_direction	(dir);

		}
		else 
		{
				light_render->set_position	(M.c);
				light_render->set_rotation	(M.k,M.i);
				Fvector offset				= M.c; 
				offset.mad					(M.i,OMNI_OFFSET.x);
				offset.mad					(M.j,OMNI_OFFSET.y);
				offset.mad					(M.k,OMNI_OFFSET.z);
				light_omni->set_position	(M.c);
				light_omni->set_rotation	(M.k,M.i);
			    glow_render->set_position	(M.c);
			    glow_render->set_direction	(M.k);
		}
	}
	else 
	{
		if (getVisible() && m_pPhysicsShell) 
		{
			M.mul						(XFORM(),BI.mTransform);
			{
				light_render->set_active(false);
				light_omni->set_active(false);
				glow_render->set_active	(false);
			}
		}
	}
	// calc color animator
	if (!lanim)							return;
	
	Msg("lanim загружен на апдейте");

	int						frame;
	// возвращает в формате BGR
	u32 clr					= lanim->CalculateBGR(Device.fTimeGlobal,frame); 

	Fcolor					fclr;
	fclr.set				((float)color_get_B(clr),(float)color_get_G(clr),(float)color_get_R(clr),1.f);
	fclr.mul_rgb			(fBrightness/255.f);
	light_render->set_color	(fclr);
	light_omni->set_color	(fclr);
	glow_render->set_color	(fclr);
}