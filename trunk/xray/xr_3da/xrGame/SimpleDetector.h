#pragma once
#include "customdetector.h"

class CUIArtefactDetectorSimple;

class CSimpleDetector :public CCustomDetector
{
	typedef CCustomDetector	inherited;
public:
					CSimpleDetector				();
	virtual			~CSimpleDetector			();

	virtual void    Load						(LPCSTR section);
	
protected:
//.	virtual void 	UpdateZones					();
	virtual void 	UpdateAf					();
	virtual void 	CreateUI					();
	CUIArtefactDetectorSimple&	ui				();
	HUD_SOUND		detect_snds;

public:
    shared_str		flash_bone;
	shared_str		on_off_bone;
	float           flash_light_range;
	float           onoff_light_range;
	Fvector3        flash_point;
};

