#include "stdafx.h"
#include "AdvancedDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "../skeletonanimated.h"
#include "hud_item_object.h"
#include "game_object_space.h"

CAdvancedDetector::CAdvancedDetector() { m_artefacts.m_af_rank = 2; }
CAdvancedDetector::~CAdvancedDetector() {}
void CAdvancedDetector::CreateUI()
{
    R_ASSERT(NULL == m_ui);
    m_ui = new CUIArtefactDetectorAdv();
    ui().construct(this);
}

CUIArtefactDetectorAdv& CAdvancedDetector::ui() { return *((CUIArtefactDetectorAdv*)m_ui); }
void CAdvancedDetector::UpdateAf()
{
    ui().SetValue(0.0f, Fvector().set(0, 0, 0));
    if (m_artefacts.m_ItemInfos.size() == 0)
        return;

    CAfList::ItemsMapIt it_b = m_artefacts.m_ItemInfos.begin();
    CAfList::ItemsMapIt it_e = m_artefacts.m_ItemInfos.end();
    CAfList::ItemsMapIt it = it_b;
    float min_dist = flt_max;

    Fvector detector_pos = Position();
    for (; it_b != it_e; ++it_b) // only nearest
    {
        CArtefact* pAf = it_b->first;
        if (pAf->H_Parent())
            continue;

        float d = detector_pos.distance_to(pAf->Position());
        if (d < min_dist)
        {
            min_dist = d;
            it = it_b;
        }
    }

    ITEM_INFO& af_info = it->second;
    ITEM_TYPE* item_type = af_info.curr_ref;
    CArtefact* pCurrentAf = it->first;

    float dist = min_dist;
    float fRelPow = (dist / m_fAfDetectRadius);
    clamp(fRelPow, 0.f, 1.f);

    // direction
    Fvector dir_to_artefact;
    dir_to_artefact.sub(pCurrentAf->Position(), Device.vCameraPosition);
    dir_to_artefact.normalize();
    float _ang_af = dir_to_artefact.getH();
    float _ang_cam = Device.vCameraDirection.getH();

    float _diff = angle_difference_signed(_ang_af, _ang_cam);

    // sounds
    af_info.cur_period = item_type->freq.x + (item_type->freq.y - item_type->freq.x) * (fRelPow * fRelPow);

    float min_snd_freq = 0.9f;
    float max_snd_freq = 1.4f;

    float snd_freq = min_snd_freq + (max_snd_freq - min_snd_freq) * (1.0f - fRelPow);

    if (af_info.snd_time > af_info.cur_period)
    {
        af_info.snd_time = 0;
        HUD_SOUND::PlaySound(item_type->detect_snds, Fvector().set(0, 0, 0), this, true, false);
        if (item_type->detect_snds.m_activeSnd)
            item_type->detect_snds.m_activeSnd->snd.set_frequency(snd_freq);
    }
    else
        af_info.snd_time += Device.fTimeDelta;

    ui().SetValue(_diff, dir_to_artefact);
}

void CUIArtefactDetectorAdv::construct(CAdvancedDetector* p)
{
    m_parent = p;
    m_target_dir.set(0, 0, 0);
    m_curr_ang_speed = 0.0f;
    m_cur_y_rot = 0.0f;
    m_bid = u16(-1);
}

CUIArtefactDetectorAdv::~CUIArtefactDetectorAdv() {}

void CUIArtefactDetectorAdv::SetValue(const float val1, const Fvector& val2) { m_target_dir = val2; }

void CUIArtefactDetectorAdv::update()
{
    if (NULL == m_parent || m_bid == u16(-1))
        return;
    inherited::update();

	CKinematics* pVisual = smart_cast<CKinematics*>(m_parent->GetHUD()->Visual());
	R_ASSERT			(pVisual);

    BOOL b_visible = !fis_zero(m_target_dir.magnitude());
    if (b_visible != pVisual->LL_GetBoneVisible(m_bid))
        pVisual->LL_SetBoneVisible(m_bid, b_visible, TRUE);

    if (!b_visible)
        return;

    Fvector dest;
    Fmatrix Mi;
	Mi.invert(m_parent->GetHUD()->Transform());
    Mi.transform_dir(dest, m_target_dir);

    float dest_y_rot = -dest.getH();

    m_cur_y_rot = angle_inertion_var(m_cur_y_rot, dest_y_rot, PI_DIV_4, PI_MUL_4, PI_MUL_2, Device.fTimeDelta);
}

void CAdvancedDetector::Hide()
{
	inherited::Hide();
	ui().ResetBoneCallbacks();
	
}

void CAdvancedDetector::Show()
{
	inherited::Show();
	TurnDetectorInternal(true);
	ui().SetBoneCallbacks();
}

void CUIArtefactDetectorAdv::BoneCallback(CBoneInstance* B)
{
    CUIArtefactDetectorAdv* P = static_cast<CUIArtefactDetectorAdv*>(B->callback_param());
    Fmatrix rY;
    rY.rotateY(P->CurrentYRotation());
    B->mTransform.mulB_43(rY);
}

void CUIArtefactDetectorAdv::SetBoneCallbacks()
{	
	CKinematics* pVisual = smart_cast<CKinematics*>(m_parent->GetHUD()->Visual());
	R_ASSERT			(pVisual);

    m_bid = pVisual->LL_BoneID("screen_bone");

    CBoneInstance& bi = pVisual->LL_GetBoneInstance(m_bid);
    bi.set_callback(bctCustom, BoneCallback, this);

    float p, b;
    bi.mTransform.getHPB(m_cur_y_rot, p, b);
}

void CUIArtefactDetectorAdv::ResetBoneCallbacks()
{
	CKinematics* pVisual = smart_cast<CKinematics*>(m_parent->GetHUD()->Visual());
	R_ASSERT			(pVisual);

    u16 bid = pVisual->LL_BoneID("screen_bone");

    CBoneInstance& bi = pVisual->LL_GetBoneInstance(bid);
    bi.reset_callback();
}

float CUIArtefactDetectorAdv::CurrentYRotation() const
{
    float one = PI_MUL_2 / 24.0f;
    float ret = fmod(m_cur_y_rot, one);
    return (m_cur_y_rot - ret);
}
