#pragma once
#include "CustomDetector.h"
class CUIArtefactDetectorAdv;

class CAdvancedDetector : public CCustomDetector
{
    typedef CCustomDetector inherited;

public:
    CAdvancedDetector();
    virtual ~CAdvancedDetector();
    virtual void Show();
    virtual void Hide();

protected:
    virtual void UpdateAf();
    virtual void CreateUI();
    CUIArtefactDetectorAdv& ui();
};

//	static void 		BoneCallback					(CBoneInstance *B);
