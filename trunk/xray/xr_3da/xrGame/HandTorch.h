////////////////////////////////////////////////////////////////////////////
//	Module 		: HandTorch.h
//	Created 	: 15.03.2017
//  Modified 	: 28.03.2017
//	Author		: Kondr48
//	Description : Ручной фонарь
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "HudItemBase.h"

class CLAItem;

class CHandTorch:	public CHudItemBase {
private:
	typedef			CHudItemBase	inherited;

public:
									CHandTorch			();
	virtual							~CHandTorch			();
	virtual void					Load				(LPCSTR section);
	virtual BOOL					net_Spawn			(CSE_Abstract* DC);
	virtual CHandTorch*				cast_hand_torch		()		{return this;}

protected:
	virtual void	                Switch              (bool turn);
	virtual void	                PlaySwitch          ();

public:
	virtual void					UpdateCL			();
	virtual void					OnStateSwitch		(u32 S);
	virtual void					OnAnimationEnd		(u32 state);
	virtual void					Hide				();

	enum EHandTorchStates {
		eIdle		= 0,
		eShowing,
		eHiding,
		eHidden,
		eTurnOn,
		eTurnOff
	};

protected:
	shared_str		                light_trace_bone;
	ref_light		                light_render;
	ref_glow		                glow_render;
	CLAItem*		                lanim;
	float			                fBrightness;

	HUD_SOUND                       m_snd_switch;

	MotionSVec						m_anim_switch;
};


