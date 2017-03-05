#pragma once
#include "UIFrameLineWnd.h"

class CUIStatic;
class CUIFrameLineWnd;
class CUIDetectorWave;
class CSimpleDetector;
//class CAdvancedDetector;
//class CEliteDetector;
class CUIXml;
class CLAItem;
class CBoneInstance;

class CUIArtefactDetectorBase
{
public:
	virtual			~CUIArtefactDetectorBase	()	{};
	virtual void	update						()	{};
};

class CUIDetectorWave :public CUIFrameLineWnd
{
	typedef CUIFrameLineWnd inherited;
protected:
	float			m_curr_v;
	float			m_step;
public:
					CUIDetectorWave		():m_curr_v(0.0f),m_step(0.0f){};
			void	InitFromXML			(CUIXml& xml, LPCSTR path);
			void	SetVelocity			(float v);
	virtual void	Update				();
};

class CUIArtefactDetectorSimple :public CUIArtefactDetectorBase
{
	typedef CUIArtefactDetectorBase	inherited;

	CSimpleDetector*	m_parent;
	u16					m_flash_bone;
	u16					m_on_off_bone;
	u32					m_turn_off_flash_time;
	
	ref_light			m_flash_light;
	ref_light			m_on_off_light;
	CLAItem*			m_pOnOfLAnim;
	CLAItem*			m_pFlashLAnim;
	void				setup_internals			();
public:
	virtual				~CUIArtefactDetectorSimple	();
	void				update						();
	void				Flash						(bool bOn, float fRelPower);

	void				construct					(CSimpleDetector* p);
};