////////////////////////////////////////////////////////////////////////////
//	Module 		: HandTorch.h
//	Created 	: 15.02.2017
//  Modified 	: 04.03.2017
//	Author		: Kondr48
//	Description : Ручной фонарь
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "HudItemBase.h"

class CLAItem;

class CHandTorch:	public CHudItemBase {
private:
	typedef			CHudItemBase	inherited;

protected:
	shared_str		light_trace_bone;
	shared_str		light_texture;
	shared_str		glow_texture;
	shared_str		color_animator;
	Fcolor          clr;
	Fcolor          clr_o;
	float           virtual_size;
	float           range;
	float           glow_radius;
	float           range_o;
	float           angle;
	bool            shadow;
	bool            volumetric_light;
	float           volumetric_light_quality;
	float           volumetric_light_intensity;
	float           volumetric_light_distance;
public:
									CHandTorch						();
	virtual							~CHandTorch						();

	virtual void					Load							(LPCSTR section);
	
	virtual BOOL					net_Spawn						(CSE_Abstract* DC);
	
	virtual CHandTorch*				cast_hand_torch					()		{return this;}

protected:

	virtual void	                Switch(bool turn);

	ref_light		                light_render;
	ref_light		                light_omni;
	ref_glow		                glow_render;

	CLAItem*		                lanim;
	float			                fBrightness;

	float			                m_delta_h;
	Fvector2		                m_prev_hp;

public:

	virtual void					UpdateCL			();
	virtual void					OnStateSwitch		(u32 S);
	virtual void					OnAnimationEnd		(u32 state);
};
