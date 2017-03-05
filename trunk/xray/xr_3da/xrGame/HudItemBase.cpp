////////////////////////////////////////////////////////////////////////////
//	Module 		: HudItemBase.cpp
//	Created 	: 04.03.2017
//  Modified 	: 04.03.2017
//	Author		: Kondr48
//	Description : Базовый класс, для худов вещей, ручного фонаря, детекторов и т. д. Старые классы наследуются как раньше.
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HudItemBase.h"
#include "PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "game_cl_base.h"
#include "Actor.h"
#include "Entity.h"

CHudItemBase::CHudItemBase(void) 
{
	m_class_name				= get_class_name<CHudItemBase>(this);
}

CHudItemBase::~CHudItemBase(void) 
{}

void CHudItemBase::Load(LPCSTR section) 
{
	inherited::Load			(section);
	animGet				(m_anim_idle,					pSettings->r_string(*hud_sect,"anim_idle"));
	animGet				(m_anim_idle_sprint,			pSettings->r_string(*hud_sect,"anim_idle_sprint"));
	animGet				(m_anim_idle_moving,			pSettings->r_string(*hud_sect,"anim_idle_moving"));
	animGet				(m_anim_hide,					pSettings->r_string(*hud_sect,"anim_hide"));
	animGet				(m_anim_show,					pSettings->r_string(*hud_sect,"anim_show"));
}

BOOL CHudItemBase::net_Spawn(CSE_Abstract* DC) 
{
	BOOL result = inherited::net_Spawn(DC);
	SetState					(eHidden);
	return result;	
}

void CHudItemBase::Hide()
{
	SwitchState(eHiding);
}

void CHudItemBase::Show()
{
	SwitchState(eShowing);
}
#include "inventoryOwner.h"
#include "Entity_alive.h"
void CHudItemBase::UpdateXForm()
{
		
	if (Device.dwFrame!=dwXF_Frame)
	{
		dwXF_Frame			= Device.dwFrame;

		if (0==H_Parent())	return;

		// Get access to entity and its visual
		CEntityAlive*		E		= smart_cast<CEntityAlive*>(H_Parent());
        
		if(!E)				return	;

		const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
		if (parent && parent->use_simplified_visual())
			return;

		VERIFY				(E);
		CKinematics*		V		= smart_cast<CKinematics*>	(E->Visual());
		VERIFY				(V);

		// Get matrices
		int					boneL,boneR,boneR2;
		E->g_WeaponBones	(boneL,boneR,boneR2);

		boneL = boneR2;

		V->CalculateBones	();
		Fmatrix& mL			= V->LL_GetTransform(u16(boneL));
		Fmatrix& mR			= V->LL_GetTransform(u16(boneR));

		// Calculate
		Fmatrix				mRes;
		Fvector				R,D,N;
		D.sub				(mL.c,mR.c);	D.normalize_safe();
		R.crossproduct		(mR.j,D);		R.normalize_safe();
		N.crossproduct		(D,R);			N.normalize_safe();
		mRes.set			(R,N,D,mR.c);
		mRes.mulA_43		(E->XFORM());
//		UpdatePosition		(mRes);
		XFORM().mul			(mRes,offset());
	}
}

void CHudItemBase::OnStateSwitch		(u32 S)
{
	inherited::OnStateSwitch	(S);
	switch(S){
	case eShowing:
		{
				VERIFY(GetState()==eShowing);
	            m_pHUD->animPlay		(random_anim(m_anim_show), FALSE, this, GetState());
		}break;
	case eHiding:
		{
				VERIFY(GetState()==eHiding);
	            m_pHUD->animPlay		(random_anim(m_anim_hide), TRUE, this, GetState());
		}break;
	case eIdle:
		{
				m_bPending = false;
	            PlayAnimIdle();
		}break;
	};
}

void CHudItemBase::OnAnimationEnd		(u32 state)
{
	switch (state)
	{
	case eHiding:
		{
			SwitchState(eHidden);
		}break;
	case eShowing:
		{
			SwitchState(eIdle);
		}break;
	};
}

bool CHudItemBase::TryPlayAnimIdle()
{ 
	    VERIFY(GetState() == eIdle);
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if (pActor)
		{
			CEntity::SEntityState st;
			pActor->g_State(st);
			if (st.bSprint && m_anim_idle_sprint.size())
			{
				m_pHUD->animPlay(random_anim(m_anim_idle_sprint), TRUE, NULL, GetState());
				return true;
			}
			else if (st.bMoving && m_anim_idle_moving.size())
			{
				m_pHUD->animPlay(random_anim(m_anim_idle_moving), TRUE, NULL, GetState());
				return true;
			}
		}
	return false;
}

void CHudItemBase::PlayAnimIdle()
{
	if (TryPlayAnimIdle()) return;
	VERIFY(GetState() == eIdle);
	m_pHUD->animPlay(random_anim(m_anim_idle), TRUE, NULL, GetState());
}

void CHudItemBase::onMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
	if (((cmd == ACTOR_DEFS::mcAnyMove) || (cmd == ACTOR_DEFS::mcSprint)) && (GetState() == eIdle))
		PlayAnimIdle();
}

void CHudItemBase::UpdateCL()
{
	inherited::UpdateCL();
}