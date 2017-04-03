#pragma once
#include "UIFrameLineWnd.h"

class CUIStatic;
class CUIFrameLineWnd;
class CUIDetectorWave;
class CSimpleDetector;
class CAdvancedDetector;
class CUIXml;
class CLAItem;
class CBoneInstance;

class CUIArtefactDetectorBase
{
public:
    virtual ~CUIArtefactDetectorBase(){};
    virtual void update(){};
};

class CUIDetectorWave : public CUIFrameLineWnd
{
    typedef CUIFrameLineWnd inherited;

protected:
    float m_curr_v;
    float m_step;

public:
    CUIDetectorWave() : m_curr_v(0.0f), m_step(0.0f){};
    void InitFromXML(CUIXml& xml, LPCSTR path);
    void SetVelocity(float v);
    virtual void Update();
};

class CUIArtefactDetectorSimple : public CUIArtefactDetectorBase
{
    typedef CUIArtefactDetectorBase inherited;

    CSimpleDetector* m_parent;
    u16 m_flash_bone;
    u16 m_on_off_bone;
    u32 m_turn_off_flash_time;

    ref_light m_flash_light;
    ref_light m_on_off_light;
    CLAItem* m_pOnOfLAnim;
    CLAItem* m_pFlashLAnim;
    void setup_internals();

public:
    virtual ~CUIArtefactDetectorSimple();
    void update();
    void Flash(bool bOn, float fRelPower);

    void construct(CSimpleDetector* p);
};

class CUIArtefactDetectorAdv : public CUIArtefactDetectorBase
{
    typedef CUIArtefactDetectorBase inherited;

    CAdvancedDetector* m_parent;
    Fvector m_target_dir;
    float m_cur_y_rot;
    float m_curr_ang_speed;
    u16 m_bid;

public:
    virtual ~CUIArtefactDetectorAdv();
    virtual void update();
    void construct(CAdvancedDetector* p);
    void SetValue(const float v1, const Fvector& v2);
    float CurrentYRotation() const;
    static void BoneCallback(CBoneInstance* B);
    void ResetBoneCallbacks();
    void SetBoneCallbacks();
};
