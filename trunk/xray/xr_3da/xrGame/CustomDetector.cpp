#include "stdafx.h"
#include "customdetector.h"
#include "PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "game_cl_base.h"
#include "../skeletonanimated.h"
#include "inventory.h"
#include "level.h"
#include "ai_object_location.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "phworld.h"
#include "restriction_space.h"
#include "../IGame_Persistent.h"

#include "inventory_item_object.h"
#include "../xr_3da/feel_touch.h"
#include "hudsound.h"
#include "customzone.h"
#include "artifact.h"
#include "ai_sounds.h"
#include "Actor.h"
#include "Entity.h"

ITEM_INFO::ITEM_INFO()
{
	pParticle	= NULL;
	curr_ref	= NULL;
}

ITEM_INFO::~ITEM_INFO()
{
	if(pParticle)
		CParticlesObject::Destroy(pParticle);
}

CCustomDetector::CCustomDetector(void) 
{
	m_class_name				= get_class_name<CCustomDetector>(this);
	SetSlot (DETECTOR_ONE_SLOT);
}


CCustomDetector::~CCustomDetector(void) 
{
	 m_artefacts.destroy();
}

void CCustomDetector::Load(LPCSTR section) 
{
	inherited::Load		(section);

	m_sect_name = section;
	
	animGet				(m_anim_idle,					pSettings->r_string(*hud_sect,"anim_idle"));
	animGet				(m_anim_idle_sprint,			pSettings->r_string(*hud_sect,"anim_idle_sprint"));
	animGet				(m_anim_idle_moving,			pSettings->r_string(*hud_sect,"anim_idle_moving"));
	animGet				(m_anim_hide,					pSettings->r_string(*hud_sect,"anim_hide"));
	animGet				(m_anim_show,					pSettings->r_string(*hud_sect,"anim_show"));

	m_fAfDetectRadius	= pSettings->r_float(section,"af_radius");
	m_artefacts.load	(section, "af");
}

BOOL CCustomDetector::net_Spawn(CSE_Abstract* DC) 
{
	BOOL result = inherited::net_Spawn(DC);
	SetState					(eHidden);

	return result;	
}

void CCustomDetector::net_Destroy() 
{
	inherited::net_Destroy		();

}

void CCustomDetector::OnH_A_Chield() 
{
	inherited::OnH_A_Chield		();
}

void CCustomDetector::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);

	m_artefacts.clear			();
}

void CCustomDetector::UpdateCL		() 
{
	inherited::UpdateCL			();
	if( !IsWorking() )		return;
	UpfateWork				    ();
}

void CCustomDetector::UpfateWork()
{
	UpdateAf				();
	//m_ui->update			();
}

void CCustomDetector::shedule_Update		(u32 dt) 
{
	inherited::shedule_Update		(dt);
	if(!IsWorking()) return;
	Fvector						P; 
	P.set						(H_Parent()->Position());
	Position().set(P);
	m_artefacts.feel_touch_update(P,m_fAfDetectRadius);
}
void CCustomDetector::PhDataUpdate	(dReal step)
{

}

void CCustomDetector::Hide()
{
	SwitchState(eHiding);
}

void CCustomDetector::Show()
{
	SwitchState(eShowing);
}
#include "inventoryOwner.h"
#include "Entity_alive.h"
void CCustomDetector::UpdateXForm()
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

void CCustomDetector::OnStateSwitch		(u32 S)
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

void CCustomDetector::OnAnimationEnd		(u32 state)
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

bool CCustomDetector::TryPlayAnimIdle()
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
				if (GetHUD()->Visual())
					Msg("Есть визуал");
				
				CKinematics* K = smart_cast<CKinematics*>(GetHUD()->Visual());
				return true;
			}
		}
	return false;
}

void CCustomDetector::PlayAnimIdle()
{
	if (TryPlayAnimIdle()) return;
	VERIFY(GetState() == eIdle);
	m_pHUD->animPlay(random_anim(m_anim_idle), TRUE, NULL, GetState());
}

void CCustomDetector::onMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
	if (((cmd == ACTOR_DEFS::mcAnyMove) || (cmd == ACTOR_DEFS::mcSprint)) && (GetState() == eIdle))
		PlayAnimIdle();
}

BOOL CAfList::feel_touch_contact	(CObject* O)
{
	TypesMapIt it				= m_TypesMap.find(O->cNameSect());

	bool res					 = (it!=m_TypesMap.end());
	if(res)
	{
		CArtefact*	pAf				= smart_cast<CArtefact*>(O);
		
		if(pAf->GetAfRank()>m_af_rank)
			res = false;
	}
	
	return						res;
}

bool CCustomDetector::IsWorking()
{
	if(!H_Parent()) return false;
	CInventoryOwner* pA = smart_cast<CInventoryOwner*>(H_Parent());
	if(!pA) return false;
	return  H_Parent()==Level().CurrentViewEntity() && !!(pA->inventory().ActiveItem());
}