////////////////////////////////////////////////////////////////////////////
//	Module 		: HudItemBase.h
//	Created 	: 04.03.2017
//  Modified 	: 04.03.2017
//	Author		: Kondr48
//	Description : Базовый класс, для худов вещей, ручного фонаря, детекторов и т. д. Старые классы наследуются как раньше.
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "hud_item_object.h"
#include "hudsound.h"
#include "PHObject.h"
#include "script_export_space.h"

class CHudItemBase:	public CHudItemObject, 
					public CPHUpdateObject {
private:
	typedef			CHudItemObject	inherited;

public:
									CHudItemBase						();
	virtual							~CHudItemBase						();

	virtual void					Load							(LPCSTR section);
	
	virtual BOOL					net_Spawn						(CSE_Abstract* DC);
	
	virtual CHudItemBase*			cast_hand_torch					()		{return this;}

	virtual void					PhDataUpdate					(dReal step)	{};
	virtual void					PhTune							(dReal step)	{};

protected:
	MotionSVec						m_anim_idle;
	MotionSVec						m_anim_idle_sprint;
	MotionSVec						m_anim_idle_moving;
	MotionSVec						m_anim_hide;
	MotionSVec						m_anim_show;

	virtual void	    PlayAnimIdle();
	bool			    TryPlayAnimIdle();
public:
	enum EHudItemBasestates {
		eIdle		= 0,
		eShowing,
		eHiding,
		eHidden,
	};

public:
	virtual void					Hide				();
	virtual void					Show				();
	virtual	void					UpdateXForm			();
	virtual void					UpdateCL			();
	virtual void					OnStateSwitch		(u32 S);
	virtual void					OnAnimationEnd		(u32 state);
	virtual bool					IsHidden			()	const	{return GetState()==eHidden;}
	virtual void		            onMovementChanged				(ACTOR_DEFS::EMoveCommand cmd);
};