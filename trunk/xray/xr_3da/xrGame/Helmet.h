#pragma once

#include "HudItemBase.h"
#include "Nightvision.h"
#include "hudsound.h"	

struct SBoneProtections;

class CHelmet:	public CHudItemBase {
private:
	typedef			CHudItemBase	inherited;

public:								CHelmet						();
	virtual							~CHelmet					();
	virtual void					Load						(LPCSTR section);

	//уменьшенная версия хита, для вызова, когда шлем надет на персонажа
	virtual void					Hit					(float P, ALife::EHitType hit_type);

	//коэффициенты на которые домножается хит
	//при соответствующем типе воздействия
	//если на персонаже надет шлем
	float							GetHitTypeProtection(ALife::EHitType hit_type, s16 element);
	float							GetDefHitTypeProtection(ALife::EHitType hit_type);

	float							HitThruArmour		(float hit_power, s16 element, float AP);

	virtual BOOL					net_Spawn					(CSE_Abstract* DC);
	virtual CHelmet*				cast_helmet 				()		{ return this; }

protected:
	CNightVisionDevice*				m_NightVisionDevice;
	MotionSVec						m_anim_filtr;
	HUD_SOUND				        m_breath_sound;
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
	SBoneProtections*				m_boneProtection;	

public:
	enum EHudItemBasestates {
		eHelmetDressing		= 0,
		eHelmetUndressing,
		eHiding,
		eHidden,
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

	bool                            helmet_is_dressed;
	virtual void	                OnH_B_Independent	    (bool just_before_destroy);

	virtual void                    HelmetDressing          ();
	virtual void                    HelmetUndressing        ();

	virtual void		            onMovementChanged		(ACTOR_DEFS::EMoveCommand cmd);

	virtual	BOOL					BonePassBullet			(int boneID);
};
