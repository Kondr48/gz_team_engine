////////////////////////////////////////////////////////////////////////////
//	Module 		: Helmet.h
//	Created 	: 15.03.2017
//  Modified 	: 27.06.2017
//	Author		: Kondr48
//	Description : �����
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "HudItemBase.h"
#include "Nightvision.h"
#include "hudsound.h"	
#include "..\..\..\build_config_defines.h"

#ifndef FIRE_WOUND_HIT_FIXED
struct SBoneProtections;
#endif

class CHelmet:	public CHudItemBase {
private:
	typedef			CHudItemBase	inherited;

public:								CHelmet						();
	virtual							~CHelmet					();
	virtual void					Load						(LPCSTR section);

	//����������� ������ ����, ��� ������, ����� ���� ����� �� ���������
	virtual void					Hit					(float P, ALife::EHitType hit_type);

	//������������ �� ������� ����������� ���
	//��� ��������������� ���� �����������
	//���� �� ��������� ����� ����
	float							GetHitTypeProtection(ALife::EHitType hit_type, s16 element);
	float							GetDefHitTypeProtection(ALife::EHitType hit_type);

#ifndef FIRE_WOUND_HIT_FIXED
	float							HitThruArmour		(float hit_power, s16 element, float AP);
#endif

	virtual BOOL					net_Spawn					(CSE_Abstract* DC);
	virtual CHelmet*				cast_helmet 				()		{ return this; }

protected:
	CNightVisionDevice*				m_NightVisionDevice;
	MotionSVec						m_anim_filtr;
	HUD_SOUND				        m_breath_sound;
	HUD_SOUND				        m_change_filter;
	HitImmunity::HitTypeSVec		m_HitTypeProtection;

#ifndef FIRE_WOUND_HIT_FIXED
	SBoneProtections*				m_boneProtection;	
#endif

public:
	enum EHudHelmetStates {
		eHelmetDressing		= 0,
		eHelmetUndressing,
		eHiding,
		eHidden,
		eChangeFilter,
	};
	
	bool					        bIsNightvisionAvaliable;
	CNightVisionDevice*				NightVisionDevice		    ()       { return m_NightVisionDevice; };
	virtual void					Show			        	();
	virtual void				    OnMoveToSlot		        ();
	virtual void					OnMoveToRuck		        ();
	virtual void					UpdateCL			        ();
	virtual void					OnStateSwitch		        (u32 S);
	virtual void					OnAnimationEnd		        (u32 state);
	virtual void			        save					    (NET_Packet &output_packet);
	virtual void			        load					    (IReader &input_packet);

	u32                             LastActiveSlot;
	virtual void                    RestoreLastActiveSlot       ();

	float					        m_fHealthRestoreSpeed;
	float 					        m_fRadiationRestoreSpeed;
	float 					        m_fSatietyRestoreSpeed;
    float 					        m_fThirstRestoreSpeed;
	float					        m_fPowerRestoreSpeed;
	float					        m_fBleedingRestoreSpeed;

	shared_str                      glass_texture;
	shared_str                      m_BonesProtectionSect;
	shared_str                      filter_section;

	bool                            helmet_is_dressed;
	bool                            filter_is_used;

	s32                             filter_life;
	float                           filter_condition;

	virtual void	                OnH_B_Independent	    (bool just_before_destroy);

	virtual void                    HelmetDressing          ();
	virtual void                    HelmetUndressing        ();

	virtual void		            onMovementChanged		(ACTOR_DEFS::EMoveCommand cmd);
};
