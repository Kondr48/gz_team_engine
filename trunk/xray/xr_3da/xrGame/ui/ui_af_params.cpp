#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "../object_broker.h"
#include "../Artifact.h"
#include "../Actor.h"
#include "../ActorCondition.h"
#include "UIXmlInit.h"

CUIArtefactParams::CUIArtefactParams()
{
	Memory.mem_fill			(m_info_items, 0, sizeof(m_info_items));
}

CUIArtefactParams::~CUIArtefactParams()
{
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];
		xr_delete				(_s);
	}
}

LPCSTR af_item_sect_names[] = {
	"health_restore_speed",
	"radiation_restore_speed",
	"satiety_restore_speed",
	"power_restore_speed",
	"bleeding_restore_speed",
	"thirst_restore_speed",
	"power_life_restore_speed",
	"condition_loss_speed",
	"additional_inventory_weight",
	
	"burn_immunity",
	"strike_immunity",
	"shock_immunity",
	"wound_immunity",		
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
};

LPCSTR af_item_param_names[] = {
	"ui_inv_health",
	"ui_inv_radiation",
	"ui_inv_satiety",
	"ui_inv_power",
	"ui_inv_bleeding",
	"ui_inv_thirst",
	"ui_inv_power_life",
	"ui_inv_condition_loss",
	"ui_inv_additional_inventory_weight",

	"ui_inv_outfit_burn_protection",			// "(burn_imm)",
	"ui_inv_outfit_strike_protection",			// "(strike_imm)",
	"ui_inv_outfit_shock_protection",			// "(shock_imm)",
	"ui_inv_outfit_wound_protection",			// "(wound_imm)",
	"ui_inv_outfit_radiation_protection",		// "(radiation_imm)",
	"ui_inv_outfit_telepatic_protection",		// "(telepatic_imm)",
	"ui_inv_outfit_chemical_burn_protection",	// "(chemical_burn_imm)",
	"ui_inv_outfit_explosion_protection",		// "(explosion_imm)",
	"ui_inv_outfit_fire_wound_protection",		// "(fire_wound_imm)",
};

void CUIArtefactParams::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base				= "af_params";
	if (!xml_doc.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow		(xml_doc, _base, 0, this);

	for(u32 i =_item_start; i <_max_item_index; ++i)
	{
		m_info_items[i]			= xr_new<CUIStatic>();
		CUIStatic* _s			= m_info_items[i];
		_s->SetAutoDelete		(false);
		strconcat				(sizeof(_buff),_buff, _base, ":static_", af_item_sect_names[i]);
		CUIXmlInit::InitStatic	(xml_doc, _buff,	0, _s);
	}
}

float CArtefact::* af_prop_offsets[] = {
	&CArtefact::m_fHealthRestoreSpeed,
	&CArtefact::m_fRadiationRestoreSpeed,
	&CArtefact::m_fSatietyRestoreSpeed,
	&CArtefact::m_fPowerRestoreSpeed,
	&CArtefact::m_fBleedingRestoreSpeed,
	&CArtefact::m_fThirstRestoreSpeed,
	&CArtefact::m_fPowerLifeRestoreSpeed,
	&CArtefact::m_fConditionLoss,
	&CArtefact::m_additional_weight
};

bool CUIArtefactParams::Check(const shared_str& af_section)
{
	return !!pSettings->line_exist(af_section, "af_actor_properties");
}
#include "../string_table.h"
void CUIArtefactParams::SetInfo(CGameObject *obj)
{	
	CArtefact *art = smart_cast<CArtefact*> (obj);
	R_ASSERT2(art, "object is not CArtefact");
	const shared_str& af_section = art->cNameSect();
	CActor *pActor = Actor();
	if (!pActor) return;

	string128					_buff;
	float						_h = 0.0f;
	DetachAll					();
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];

		float					_val;
		if(i<_max_item_index1)
		{
			float CArtefact::* pRestoreSpeed = af_prop_offsets[i]; // за пример спасибо alpet'у
			_val = (art->*pRestoreSpeed)*art->GetCondition();
			if					(fis_zero(_val)) continue;
			_val				= _val*100.0f;
		}
		else
		{
			shared_str _sect	= pSettings->r_string(af_section, "hit_absorbation_sect");
			_val				= pSettings->r_float(_sect, af_item_sect_names[i]);
			if					(fsimilar(_val, 1.0f))				continue;
			_val				= (1.0f - _val);
			_val				*= 100.0f;

		}
		LPCSTR _sn = "%";
		if(i==_item_radiation_restore_speed || i==_item_power_restore_speed)
		{
			_val				/= 100.0f;
			_sn					= "";
		}
		else if (i==_item_additional_inventory_weight)
		{
			_val				/= 100.0f;
			_sn					= *CStringTable().translate("st_kg");
		}
		else if (i==_item_condition_loss_speed)
			_sn					= *CStringTable().translate("st_percent_in_hour");

		LPCSTR _color = (_val>0)?"%c[green]":"%c[red]";
		
		if(i==_item_bleeding_restore_speed)
			_val		*=	-1.0f;

		if(i==_item_bleeding_restore_speed || i==_item_radiation_restore_speed)
			_color = (_val>0)?"%c[red]":"%c[green]";


		sprintf_s					(	_buff, "%s %s %+.0f %s", 
									CStringTable().translate(af_item_param_names[i]).c_str(), 
									_color, 
									_val, 
									_sn);
		_s->SetText				(_buff);
		_s->SetWndPos			(_s->GetWndPos().x, _h);
		_h						+= _s->GetWndSize().y;
		AttachChild				(_s);
	}
	SetHeight					(_h);
}
