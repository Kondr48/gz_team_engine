#include "stdafx.h"
#include "pch_script.h"
#include "torch.h"
#include "entity.h"
#include "actor.h"
#include "../LightAnimLibrary.h"
#include "PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "ai_sounds.h"

#include "HUDManager.h"
#include "level.h"
#include "../skeletoncustom.h"
#include "../camerabase.h"
#include "inventory.h"
#include "game_base_space.h"

#include "UIGameCustom.h"
#include "actorEffector.h"
#include "game_object_space.h"
#include "script_callback_ex.h"

#include "actorcondition.h"

static const float		TIME_2_HIDE					= 5.f;
static const float		TORCH_INERTION_CLAMP		= PI_DIV_6;
static const float		TORCH_INERTION_SPEED_MAX	= 7.5f;
static const float		TORCH_INERTION_SPEED_MIN	= 0.5f;
static const Fvector	TORCH_OFFSET				= {-0.2f,+0.1f,-0.3f};
static const Fvector	OMNI_OFFSET					= {-0.2f,+0.1f,-0.1f};
static const float		OPTIMIZATION_DISTANCE		= 100.f;

static bool stalker_use_dynamic_lights	= false;

CTorch::CTorch(void) 
{
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_virtual_size (0.1);
	light_render->set_shadow	(true);
	light_omni					= ::Render->light_create();
	light_omni->set_type		(IRender_Light::POINT);
	light_omni->set_shadow		(false);

	m_switched_on				= false;
	glow_render					= ::Render->glow_create();
	lanim						= 0;
	time2hide					= 0;
	fBrightness					= 1.f;
	b_lastState					= false;

	m_prev_hp.set				(0,0);
	m_delta_h					= 0;
	SetSlot						(TORCH_SLOT);
	need_slot					= true;
	battarey_life               = 1.f;
	battarey_power              = 1.f;
}

CTorch::~CTorch(void) 
{
	light_render.destroy	();
	light_omni.destroy	();
	glow_render.destroy		();
}

inline bool CTorch::can_use_dynamic_lights	()
{
	if (!H_Parent())
		return				(true);

	CInventoryOwner			*owner = smart_cast<CInventoryOwner*>(H_Parent());
	if (!owner)
		return				(true);

	return					(owner->can_use_dynamic_lights());
}

void CTorch::Load(LPCSTR section) 
{
	inherited::Load			(section);
	light_trace_bone		= pSettings->r_string(section, "light_trace_bone");
	battarey_life		    = pSettings->r_float(section,  "battarey_life");
}

void CTorch::Switch()
{
	if (OnClient()) return;
	bool bActive			= !m_switched_on;
	Switch					(bActive);
}

void CTorch::Switch	(bool light_on)
{
	m_switched_on			= light_on;
	if (can_use_dynamic_lights())
	{
		light_render->set_active(light_on);
		
		CActor *pA = smart_cast<CActor *>(H_Parent());
		if(!pA)light_omni->set_active(light_on);
	}
	glow_render->set_active					(light_on);

	if (*light_trace_bone) 
	{
		CKinematics* pVisual				= smart_cast<CKinematics*>(Visual()); VERIFY(pVisual);
		u16 bi								= pVisual->LL_BoneID(light_trace_bone);

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE);
		pVisual->CalculateBones				();

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE); //hack
	}

	/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
	// Колбек на переключение фонаря
	if(b_lastState == light_on) return;
	b_lastState = light_on;
	callback(GameObject::eSwitchTorch)(light_on);
	// вызываем событие также и для актора (при использовании его фонарика)
	CActor *pA = smart_cast<CActor *>(H_Parent());
	if ((pA) ? true : false)
	{
		Actor()->callback(GameObject::eSwitchTorch)(light_on);
	}
	/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/
}

BOOL CTorch::net_Spawn(CSE_Abstract* DC) 
{
	CSE_Abstract			*e	= (CSE_Abstract*)(DC);
	CSE_ALifeItemTorch		*torch	= smart_cast<CSE_ALifeItemTorch*>(e);
	R_ASSERT				(torch);
	cNameVisual_set			(torch->get_visual());

	R_ASSERT				(!CFORM());
	R_ASSERT				(smart_cast<CKinematics*>(Visual()));
	collidable.model		= xr_new<CCF_Skeleton>	(this);

	if (!inherited::net_Spawn(DC))
		return				(FALSE);

	bool b_r2				= !!psDeviceFlags.test(rsR2);

	CKinematics* K			= smart_cast<CKinematics*>(Visual());
	CInifile* pUserData		= K->LL_UserData(); 
	R_ASSERT3				(pUserData,"Empty Torch user data!",torch->get_visual());
	lanim					= LALib.FindItem(pUserData->r_string("torch_definition","color_animator"));
	guid_bone				= K->LL_BoneID	(pUserData->r_string("torch_definition","guide_bone"));	VERIFY(guid_bone!=BI_NONE);

	Fcolor clr				= pUserData->r_fcolor				("torch_definition",(b_r2)?"color_r2":"color");
	fBrightness				= clr.intensity();
	float range				= pUserData->r_float				("torch_definition",(b_r2)?"range_r2":"range");
	light_render->set_color	(clr);
	light_render->set_range	(range);

	Fcolor clr_o			= pUserData->r_fcolor				("torch_definition",(b_r2)?"omni_color_r2":"omni_color");
	float range_o			= pUserData->r_float				("torch_definition",(b_r2)?"omni_range_r2":"omni_range");
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

	//включить/выключить фонарик
	Switch					(torch->m_active);
	VERIFY					(!torch->m_active || (torch->ID_Parent != 0xffff));
	
	m_delta_h				= PI_DIV_2-atan((range*0.5f)/_abs(TORCH_OFFSET.x));

	return					(TRUE);
}

void CTorch::net_Destroy() 
{
	Switch					(false);
	inherited::net_Destroy	();
}

void CTorch::OnH_A_Chield() 
{
	inherited::OnH_A_Chield			();
	m_focus.set						(Position());
}

void CTorch::OnH_B_Independent	(bool just_before_destroy) 
{
	inherited::OnH_B_Independent	(just_before_destroy);
	time2hide						= TIME_2_HIDE;

	Switch						(false);
}

void CTorch::UpdatePowerLoss()
{
	m_pActor = smart_cast<CActor *>(H_Parent());
	if (!((m_pActor) ? true : false)) {battarey_power = 1.0f; return;}
    
	battarey_power -= (1/(battarey_life * 3600)) * m_fDeltaTime;
	clamp(battarey_power, 0.0f, 1.0f);

	if (battarey_power <= 0 && m_switched_on == true)
        Switch();
}

void CTorch::UpdateCL() 
{
	inherited::UpdateCL			();
	
	if (!m_switched_on)			return;

	UpdatePowerLoss             ();

	CBoneInstance			&BI = smart_cast<CKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
	Fmatrix					M;

	if (H_Parent()) 
	{
		CActor*			actor = smart_cast<CActor*>(H_Parent());
		if (actor)		smart_cast<CKinematics*>(H_Parent()->Visual())->CalculateBones_Invalidate	();

		if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition)<_sqr(OPTIMIZATION_DISTANCE) || GameID() != GAME_SINGLE) {
			// near camera
			smart_cast<CKinematics*>(H_Parent()->Visual())->CalculateBones	();
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
			if (can_use_dynamic_lights()) 
			{
				light_render->set_position	(M.c);
				light_render->set_rotation	(M.k,M.i);

				Fvector offset				= M.c; 
				offset.mad					(M.i,OMNI_OFFSET.x);
				offset.mad					(M.j,OMNI_OFFSET.y);
				offset.mad					(M.k,OMNI_OFFSET.z);
				light_omni->set_position	(M.c);
				light_omni->set_rotation	(M.k,M.i);
			}

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
				m_switched_on			= false;
				light_render->set_active(false);
				light_omni->set_active(false);
				glow_render->set_active	(false);
			}
		}
	}

	if (!m_switched_on)					return;

	// calc color animator
	if (!lanim)							return;

	int						frame;
	// возвращает в формате BGR
	u32 clr					= lanim->CalculateBGR(Device.fTimeGlobal,frame); 

	Fcolor					fclr;
	fclr.set				((float)color_get_B(clr),(float)color_get_G(clr),(float)color_get_R(clr),1.f);
	fclr.mul_rgb			(fBrightness/255.f);
	if (can_use_dynamic_lights())
	{
		light_render->set_color	(fclr);
		light_omni->set_color	(fclr);
	}
	glow_render->set_color		(fclr);
}


void CTorch::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CTorch::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CTorch::setup_physic_shell	()
{
	CPhysicsShellHolder::setup_physic_shell();
}

void CTorch::net_Export			(NET_Packet& P)
{
	inherited::net_Export		(P);
//	P.w_u8						(m_switched_on ? 1 : 0);

	BYTE F = 0;
	F |= (m_switched_on ? eTorchActive : 0);
	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA)
	{
		if (pA->attached(this))
			F |= eAttached;
	}
	P.w_u8(F);
}

void CTorch::net_Import			(NET_Packet& P)
{
	inherited::net_Import		(P);
		
	BYTE F = P.r_u8();
	bool new_m_switched_on				= !!(F & eTorchActive);

	if (new_m_switched_on != m_switched_on)			
		Switch(new_m_switched_on);
}

bool  CTorch::can_be_attached		() const
{
	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA) 
	{
		u32 slot = GetSlot();
		PIItem item = pA->inventory().m_slots[slot].m_pIItem;
		if( (const CTorch*)smart_cast<CTorch*>(item) == this )
			return true;
		else
			return false;
	}
	return true;
}
void CTorch::afterDetach			()
{
	inherited::afterDetach	();
	Switch					(false);
}
void CTorch::renderable_Render()
{
	inherited::renderable_Render();
}

void CTorch::SetBattareyPower(float x)
{
	battarey_power = x;
	clamp(battarey_power, 0.f, 1.f);
}

void CTorch::SetBattareyLife(float x)
{
    battarey_life = x;
	clamp(battarey_life, 0.f, 1.f);
}