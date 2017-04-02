#include "stdafx.h"
#include "simpledetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "hud_item_object.h"
#include "../skeletonanimated.h"
CSimpleDetector::CSimpleDetector(void)
{
	m_artefacts.m_af_rank = 1;
}

CSimpleDetector::~CSimpleDetector(void)
{}

void CSimpleDetector::Load(LPCSTR section) 
{
	inherited::Load		(section);

	flash_bone = pSettings->r_string(*hud_sect, "flash_bone"); 
	on_off_bone = pSettings->r_string(*hud_sect, "on_off_bone"); 

	flash_point = pSettings->r_fvector3(*hud_sect, "flash_point"); 

	flash_light_range  = pSettings->r_float(*hud_sect, "flash_light_range"); 
	onoff_light_range  = pSettings->r_float(*hud_sect, "onoff_light_range"); 
}

void CSimpleDetector::CreateUI()
{
    R_ASSERT(NULL == m_ui);
    m_ui = new CUIArtefactDetectorSimple();
	ui().construct		(this);
}

CUIArtefactDetectorSimple&  CSimpleDetector::ui()
{
	return *((CUIArtefactDetectorSimple*)m_ui);
}

void CSimpleDetector::UpdateAf()
{
	if(m_artefacts.m_ItemInfos.size()==0)	return;
	CAfList::ItemsMapIt it_b	= m_artefacts.m_ItemInfos.begin();
	CAfList::ItemsMapIt it_e	= m_artefacts.m_ItemInfos.end();
	CAfList::ItemsMapIt it		= it_b;
	float min_dist				= flt_max;

	Fvector						detector_pos = Position();

	for(;it_b!=it_e;++it_b)//only nearest
	{
		CArtefact *pAf			= it_b->first;
		if(pAf->H_Parent())		
			continue;

		float d = detector_pos.distance_to(pAf->Position());
		if( d < min_dist)
		{
			min_dist	= d;
			it			= it_b;
		}
	}
		
	ITEM_INFO& af_info		= it->second;

	ITEM_TYPE* item_type	= af_info.curr_ref;

	float dist				= min_dist;
		
	float fRelPow			= (dist/m_fAfDetectRadius);
	clamp					(fRelPow, 0.f, 1.f);

	//определить текущую частоту срабатывания сигнала
	af_info.cur_period = item_type->freq.x + 
		(item_type->freq.y - item_type->freq.x) * (fRelPow*fRelPow);

	float min_snd_freq	= 0.9f;
	float max_snd_freq	= 1.4f;

	float snd_freq		= min_snd_freq+(max_snd_freq-min_snd_freq)*(1.0f-fRelPow);

	if(af_info.snd_time > af_info.cur_period)
	{
		af_info.snd_time		= 0;
		HUD_SOUND::PlaySound (item_type->detect_snds, Fvector().set(0,0,0), 0, true );
		ui().Flash					(true, fRelPow);
		if(detect_snds.m_activeSnd)
			detect_snds.m_activeSnd->snd.set_frequency(snd_freq);
	} 
	else 
		af_info.snd_time += Device.fTimeDelta;
}

void CUIArtefactDetectorSimple::construct(CSimpleDetector* p)
{
	m_parent							= p;
	m_flash_bone						= BI_NONE;
	m_on_off_bone						= BI_NONE;
	Flash								(false, 0.0f);
}

CUIArtefactDetectorSimple::~CUIArtefactDetectorSimple()
{
	m_flash_light.destroy();
	m_on_off_light.destroy();
}

void CUIArtefactDetectorSimple::Flash(bool bOn, float fRelPower)
{
	if (!m_parent) return;

	CKinematics* pVisual = smart_cast<CKinematics*>(m_parent->GetHUD()->Visual());
	
	R_ASSERT			(pVisual);
	
	if (m_flash_bone == BI_NONE)
        setup_internals();

	if(bOn)
	{
		pVisual->LL_SetBoneVisible(m_flash_bone, TRUE, TRUE);
		m_flash_light->set_active(true);
		m_turn_off_flash_time = Device.dwTimeGlobal+iFloor(fRelPower*100.0f);
	}else
	{
		pVisual->LL_SetBoneVisible(m_flash_bone, FALSE, TRUE);
		m_flash_light->set_active(false);
		m_turn_off_flash_time = 0;
	}
}

void CUIArtefactDetectorSimple::setup_internals()
{
	if (!m_parent) return;

	R_ASSERT						(!m_flash_light);
	m_flash_light					= ::Render->light_create();
	m_flash_light->set_shadow		(false);
	m_flash_light->set_type			(IRender_Light::POINT);
	m_flash_light->set_range		(m_parent->flash_light_range);
	m_flash_light->set_hud_mode     (true);
		
	R_ASSERT						(!m_on_off_light);
	m_on_off_light					= ::Render->light_create();
	m_on_off_light->set_shadow		(false);
	m_on_off_light->set_type		(IRender_Light::POINT);
	m_on_off_light->set_range		(m_parent->onoff_light_range);
	m_on_off_light->set_hud_mode    (true);

	CKinematics* K                  = smart_cast<CKinematics*>(m_parent->GetHUD()->Visual());
	R_ASSERT						(K);

	R_ASSERT						(m_flash_bone==BI_NONE);
	R_ASSERT						(m_on_off_bone==BI_NONE);
	
	m_flash_bone					= K->LL_BoneID	(m_parent->flash_bone.c_str());
	m_on_off_bone					= K->LL_BoneID	(m_parent->on_off_bone.c_str());
	
	K->LL_SetBoneVisible			(m_flash_bone,	FALSE, TRUE);
	K->LL_SetBoneVisible			(m_on_off_bone, TRUE, TRUE);

    m_pOnOfLAnim					= LALib.FindItem("det_on_off");
	m_pFlashLAnim					= LALib.FindItem("det_flash");
}

void CUIArtefactDetectorSimple::update()
{
	inherited::update					();

	if(m_parent)
	{
		if(m_flash_bone==BI_NONE)
			setup_internals();

		if(m_turn_off_flash_time && m_turn_off_flash_time<Device.dwTimeGlobal)
			Flash (false, 0.0f);

		CKinematics* K      = smart_cast<CKinematics*>(m_parent->GetHUD()->Visual());
		Fmatrix M;

		if(m_flash_light->get_active())
		{
            CBoneInstance& BI = K->LL_GetBoneInstance(m_on_off_bone);
			M.mul(m_parent->GetHUD()->Transform(), BI.mTransform);
            Fvector3 bone_pos = M.c;
			Fvector3 point_pos = m_parent->flash_point;
			bone_pos.x += point_pos.x;
			bone_pos.y += point_pos.y;
			bone_pos.z += point_pos.z;
			m_flash_light->set_position	(bone_pos);
		}

		CBoneInstance& BI = K->LL_GetBoneInstance(m_flash_bone);
		M.mul(m_parent->GetHUD()->Transform(), BI.mTransform);
		m_on_off_light->set_position	(M.c);
		
		if(!m_on_off_light->get_active())
			m_on_off_light->set_active(true);

		int frame = 0;
		u32 clr					= m_pOnOfLAnim->CalculateRGB(Device.fTimeGlobal,frame);
		Fcolor					fclr;
		fclr.set				(clr);
		m_on_off_light->set_color(fclr);
	}
}
