#include "pch_script.h"
#include "actorcondition.h"
#include "actor.h"
#include "actorEffector.h"
#include "inventory.h"
#include "level.h"
#include "sleepeffector.h"
#include "game_base_space.h"
#include "autosave_manager.h"
#include "xrserver.h"
#include "ai_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "ui\UIVideoPlayerWnd.h"
#include "ui\UIMainIngameWnd.h"
#include "script_callback_ex.h"
#include "object_broker.h"
#include "weapon.h"
#include "torch.h"

#define MAX_SATIETY					1.0f
#define START_SATIETY				0.5f

bool CActor::IsLimping()
{
	return m_entity_condition->IsLimping();
}

BOOL	GodMode	()	
{ 
	if (GameID() == GAME_SINGLE) 
		return psActorFlags.test(AF_GODMODE); 
	return FALSE;	
}

CActorCondition::CActorCondition(CActor *object) :
	inherited	(object)
{
	m_fJumpPower				= 0.f;
	m_fStandPower				= 0.f;
	m_fWalkPower				= 0.f;
	m_fJumpWeightPower			= 0.f;
	m_fWalkWeightPower			= 0.f;
	m_fOverweightWalkK			= 0.f;
	m_fOverweightJumpK			= 0.f;
	m_fAccelK					= 0.f;
	m_fSprintK					= 0.f;
	m_fAlcohol					= 0.f;
	m_fSatiety					= 1.0f;
	m_fThirst					= 0.f;
	m_fBattareyLife				= 1.0f;

	VERIFY						(object);
	m_object					= object;
	m_condition_flags.zero		();

}

CActorCondition::~CActorCondition(void)
{
}

void CActorCondition::LoadCondition(LPCSTR entity_section)
{
	inherited::LoadCondition(entity_section);

	LPCSTR						section = READ_IF_EXISTS(pSettings,r_string,entity_section,"condition_sect",entity_section);

	m_fJumpPower				= pSettings->r_float(section,"jump_power");
	m_fStandPower				= pSettings->r_float(section,"stand_power");
	m_fWalkPower				= pSettings->r_float(section,"walk_power");
	m_fJumpWeightPower			= pSettings->r_float(section,"jump_weight_power");
	m_fWalkWeightPower			= pSettings->r_float(section,"walk_weight_power");
	m_fOverweightWalkK			= pSettings->r_float(section,"overweight_walk_k");
	m_fOverweightJumpK			= pSettings->r_float(section,"overweight_jump_k");
	m_fAccelK					= pSettings->r_float(section,"accel_k");
	m_fSprintK					= pSettings->r_float(section,"sprint_k");

	//порог силы и здоровь€ меньше которого актер начинает хромать
	m_fLimpingHealthBegin		= pSettings->r_float(section,	"limping_health_begin");
	m_fLimpingHealthEnd			= pSettings->r_float(section,	"limping_health_end");
	R_ASSERT					(m_fLimpingHealthBegin<=m_fLimpingHealthEnd);

	m_fLimpingPowerBegin		= pSettings->r_float(section,	"limping_power_begin");
	m_fLimpingPowerEnd			= pSettings->r_float(section,	"limping_power_end");
	R_ASSERT					(m_fLimpingPowerBegin<=m_fLimpingPowerEnd);

	m_fCantWalkPowerBegin		= pSettings->r_float(section,	"cant_walk_power_begin");
	m_fCantWalkPowerEnd			= pSettings->r_float(section,	"cant_walk_power_end");
	R_ASSERT					(m_fCantWalkPowerBegin<=m_fCantWalkPowerEnd);

	m_fCantSprintPowerBegin		= pSettings->r_float(section,	"cant_sprint_power_begin");
	m_fCantSprintPowerEnd		= pSettings->r_float(section,	"cant_sprint_power_end");
	R_ASSERT					(m_fCantSprintPowerBegin<=m_fCantSprintPowerEnd);

	m_fPowerLeakSpeed			= pSettings->r_float(section,"max_power_leak_speed");
	
	m_fV_Alcohol				= pSettings->r_float(section,"alcohol_v");

//. ???	m_fSatietyCritical			= pSettings->r_float(section,"satiety_critical");
	m_fV_Satiety				= pSettings->r_float(section,"satiety_v");		
	m_fV_SatietyPower			= pSettings->r_float(section,"satiety_power_v");
	m_fV_SatietyHealth			= pSettings->r_float(section,"satiety_health_v");

	m_fV_Thirst				    = pSettings->r_float(section,"thirst_v");	
	m_fS_Thirst                 = pSettings->r_float(section,"thirst_in_sprint_v");	
	m_MaxWalkWeight			    = pSettings->r_float(section,"max_walk_weight");
}


//вычисление параметров с ходом времени
#include "UI.h"
#include "HUDManager.h"

void CActorCondition::UpdateCondition()
{
	if (GodMode())				return;
	if (!object().g_Alive())	return;
	if (!object().Local() && m_object != Level().CurrentViewEntity())		return;	
	

	if ((object().mstate_real&mcAnyMove)) {
		ConditionWalk(object().inventory().TotalWeight()/object().inventory().GetMaxWeight(), isActorAccelerated(object().mstate_real,object().IsZoomAimingMode()), (object().mstate_real&mcSprint) != 0);
	}
	else {
		ConditionStand(object().inventory().TotalWeight()/object().inventory().GetMaxWeight());
	};
	
	if( IsGameTypeSingle() ){

		float k_max_power = 1.0f;

		if( true )
		{
			float weight = object().inventory().TotalWeight();

			float base_w = object().MaxCarryWeight();
/*
			CCustomOutfit* outfit	= m_object->GetOutfit();
			if(outfit)
				base_w += outfit->m_additional_weight2;
*/

			k_max_power = 1.0f + _min(weight,base_w)/base_w + _max(0.0f, (weight-base_w)/10.0f);
		}else
			k_max_power = 1.0f;
		
		SetMaxPower		(GetMaxPower() - m_fPowerLeakSpeed*m_fDeltaTime*k_max_power);
	}


	m_fAlcohol		+= m_fV_Alcohol*m_fDeltaTime;
	clamp			(m_fAlcohol,			0.0f,		1.0f);

	if ( IsGameTypeSingle() )
	{	
		CEffectorCam* ce = Actor()->Cameras().GetCamEffector((ECamEffectorType)effAlcohol);
		if	((m_fAlcohol>0.0001f) ){
			if(!ce){
				AddEffector(m_object,effAlcohol, "effector_alcohol", GET_KOEFF_FUNC(this, &CActorCondition::GetAlcohol));
			}
		}else{
			if(ce)
				RemoveEffector(m_object,effAlcohol);
		}

		
		CEffectorPP* ppe = object().Cameras().GetPPEffector((EEffectorPPType)effPsyHealth);
		
		string64			pp_sect_name;
		shared_str ln		= Level().name();
		strconcat			(sizeof(pp_sect_name),pp_sect_name, "effector_psy_health", "_", *ln);
		if(!pSettings->section_exist(pp_sect_name))
			strcpy_s			(pp_sect_name, "effector_psy_health");

		if	( !fsimilar(GetPsyHealth(), 1.0f, 0.05f) )
		{
			if(!ppe)
			{
				AddEffector(m_object,effPsyHealth, pp_sect_name, GET_KOEFF_FUNC(this, &CActorCondition::GetPsy));
			}
		}else
		{
			if(ppe)
				RemoveEffector(m_object,effPsyHealth);
		}
		if(fis_zero(GetPsyHealth()))
			health() =0.0f;
	};

	UpdateSatiety				();

	UpdateThirst				();

	UpdateBattareyLife          ();

	UpdateBoosters              (); 

	inherited::UpdateCondition	();

	if( IsGameTypeSingle() )
		UpdateTutorialThresholds();
}

void CActorCondition::UpdateSatiety()
{
	if (!IsGameTypeSingle()) return;

	float k = 1.0f;
	if(m_fSatiety>0)
	{
		m_fSatiety -=	m_fV_Satiety*
						k*
						m_fDeltaTime;
	
		clamp			(m_fSatiety,		0.0f,		1.0f);

	}
		
	//сытость увеличивает здоровье только если нет открытых ран
	if(!m_bIsBleeding)
	{
		m_fDeltaHealth += CanBeHarmed() ? 
					(m_fV_SatietyHealth*(m_fSatiety>0.0f?1.f:-1.f)*m_fDeltaTime)
					: 0;
	}

	//коэффициенты уменьшени€ восстановлени€ силы от сытоти и радиации
	float radiation_power_k		= 1.f;
	float satiety_power_k		= 1.f;
			
	m_fDeltaPower += m_fV_SatietyPower*
				radiation_power_k*
				satiety_power_k*
				m_fDeltaTime;
}

void CActorCondition::UpdateBoosters()
{
	for(u8 i=0;i<eBoostMaxCount;i++)
	{
		BOOSTER_MAP::iterator it = m_booster_influences.find((EBoostParams)i);
		if(it!=m_booster_influences.end())
		{
			it->second.fBoostTime -= m_fDeltaTime/(IsGameTypeSingle()?Level().GetGameTimeFactor():1.0f);
			if(it->second.fBoostTime<=0.0f)
			{
				DisableBoostParameters(it->second);
				m_booster_influences.erase(it);
			}
		}
	}

	if(m_object == Level().CurrentViewEntity())
		HUD().GetUI()->UIMainIngameWnd->UpdateBoosterIndicators(m_booster_influences);
}

void CActorCondition::UpdateThirst()
{
	if (!IsGameTypeSingle()) return;
	
	float k = 1.0f;
	if(m_fThirst<1)
	{
		CEntity::SEntityState st;
		object().g_State(st);
		
		if (st.bSprint)
		  m_fThirst		+= m_fS_Thirst*k*m_fDeltaTime;
		else
		  m_fThirst		+= m_fV_Thirst*k*m_fDeltaTime;
		  
	    clamp (m_fThirst,			0.0f,		1.0f);   
	}
}

void CActorCondition::UpdateBattareyLife()
{
	if (!IsGameTypeSingle()) return;
	
	if(m_fBattareyLife>0)
	{	
		CTorch* pTorch = smart_cast<CTorch*>(object().inventory().ItemFromSlot(TORCH_SLOT));
		float torch_pl;
		
		if (pTorch)
		 torch_pl = pTorch->GetPowerLoss();
		else
         torch_pl = 0.0f;
		
		m_fBattareyLife -= (torch_pl)*0.000277f*m_fDeltaTime;
		//ѕотер€ энергии от всех вкюченных потребителей

		clamp (m_fBattareyLife,			0.0f,		1.0f);
	}
}

CWound* CActorCondition::ConditionHit(SHit* pHDS)
{
	if (GodMode()) return NULL;
	return inherited::ConditionHit(pHDS);
}

//weight - "удельный" вес от 0..1
void CActorCondition::ConditionJump(float weight)
{
	float power			=	m_fJumpPower;
	power				+=	m_fJumpWeightPower*weight*(weight>1.f?m_fOverweightJumpK:1.f);
	m_fPower			-=	HitPowerEffect(power);
}
void CActorCondition::ConditionWalk(float weight, bool accel, bool sprint)
{	
	float power			=	m_fWalkPower;
	power				+=	m_fWalkWeightPower*weight*(weight>1.f?m_fOverweightWalkK:1.f);
	power				*=	m_fDeltaTime*(accel?(sprint?m_fSprintK:m_fAccelK):1.f);
	m_fPower			-=	HitPowerEffect(power);
}

void CActorCondition::ConditionStand(float weight)
{	
	float power			= m_fStandPower;
	power				*= m_fDeltaTime;
	m_fPower			-= power;
}


bool CActorCondition::IsCantWalk() const
{
	if(m_fPower< m_fCantWalkPowerBegin)
		m_bCantWalk		= true;
	else if(m_fPower > m_fCantWalkPowerEnd)
		m_bCantWalk		= false;
	return				m_bCantWalk;
}

#include "CustomOutfit.h"
#include "Artifact.h"

bool CActorCondition::IsCantWalkWeight()
{
	if(IsGameTypeSingle() && !GodMode())
	{
		float max_w				= m_MaxWalkWeight;

		CCustomOutfit* outfit	= m_object->GetOutfit();
		if(outfit)
			max_w += outfit->m_additional_weight;

		for(TIItemContainer::const_iterator it = object().inventory().m_belt.begin(); object().inventory().m_belt.end() != it; ++it) 
	     {
		   CArtefact*	cast_artefact = smart_cast<CArtefact*>(*it);
		     if(cast_artefact)
			   max_w	+= cast_artefact->GetAdditionalInventoryWeight();
	     }

		if( object().inventory().TotalWeight() > max_w )
		{
			m_condition_flags.set			(eCantWalkWeight, TRUE);
			return true;
		}
	}
	m_condition_flags.set					(eCantWalkWeight, FALSE);
	return false;
}

bool CActorCondition::IsCantSprint() const
{
	if(m_fPower< m_fCantSprintPowerBegin)
		m_bCantSprint	= true;
	else if(m_fPower > m_fCantSprintPowerEnd)
		m_bCantSprint	= false;
	return				m_bCantSprint;
}

bool CActorCondition::IsLimping() const
{
	if(m_fPower< m_fLimpingPowerBegin || GetHealth() < m_fLimpingHealthBegin)
		m_bLimping = true;
	else if(m_fPower > m_fLimpingPowerEnd && GetHealth() > m_fLimpingHealthEnd)
		m_bLimping = false;
	return m_bLimping;
}
extern bool g_bShowHudInfo;

void CActorCondition::save(NET_Packet &output_packet)
{
	inherited::save		(output_packet);
	save_data			(m_fAlcohol, output_packet);
	save_data			(m_condition_flags, output_packet);
	save_data			(m_fSatiety, output_packet);
	save_data			(m_fThirst, output_packet);
	save_data			(m_fBattareyLife, output_packet);
	output_packet.w_u8((u8)m_booster_influences.size());
	BOOSTER_MAP::iterator b = m_booster_influences.begin(), e = m_booster_influences.end();
	for(; b!=e; b++)
	{
		output_packet.w_u8((u8)b->second.m_type);
		output_packet.w_float(b->second.fBoostValue);
		output_packet.w_float(b->second.fBoostTime);
	}
}

void CActorCondition::load(IReader &input_packet)
{
	inherited::load		(input_packet);
	load_data			(m_fAlcohol, input_packet);
	load_data			(m_condition_flags, input_packet);
	load_data			(m_fSatiety, input_packet);
	load_data			(m_fThirst, input_packet);
	load_data			(m_fBattareyLife, input_packet);
	u8 cntr = input_packet.r_u8();
	for(; cntr>0; cntr--)
	{
		SBooster B;
		B.m_type = (EBoostParams)input_packet.r_u8();
		B.fBoostValue = input_packet.r_float();
		B.fBoostTime = input_packet.r_float();
		m_booster_influences[B.m_type] = B;
		BoostParameters(B);
	}
}

void CActorCondition::reinit	()
{
	inherited::reinit	();
	m_bLimping					= false;
	m_fSatiety					= 1.f;
	m_fBattareyLife				= 1.f;
	m_fThirst					= 0.f;
}

void CActorCondition::ChangeAlcohol	(float value)
{
	m_fAlcohol += value;
}

void CActorCondition::ChangeSatiety(float value)
{
	m_fSatiety += value;
	clamp		(m_fSatiety, 0.0f, 1.0f);
}

void CActorCondition::ChangeThirst(float value)
{
	m_fThirst += value;
	clamp		(m_fThirst, 0.0f, 1.0f);
}

void CActorCondition::ChangeBattareyLife(float value)
{
	m_fBattareyLife	+= value;
	clamp		(m_fBattareyLife, 0.0f, 1.0f);
}

void CActorCondition::BoostParameters(const SBooster& B)
{
	if(OnServer())
	{
		switch(B.m_type)
		{
			case eBoostHpRestore: BoostHpRestore(B.fBoostValue); break;
			case eBoostPowerRestore: BoostPowerRestore(B.fBoostValue); break;
			case eBoostRadiationRestore: BoostRadiationRestore(B.fBoostValue); break;
			case eBoostBleedingRestore: BoostBleedingRestore(B.fBoostValue); break;
			case eBoostMaxWeight: BoostMaxWeight(B.fBoostValue); break;
			case eBoostBurnImmunity: BoostBurnImmunity(B.fBoostValue); break;
			case eBoostShockImmunity: BoostShockImmunity(B.fBoostValue); break;
			case eBoostRadiationImmunity: BoostRadiationImmunity(B.fBoostValue); break;
			case eBoostTelepaticImmunity: BoostTelepaticImmunity(B.fBoostValue); break;
			case eBoostChemicalBurnImmunity: BoostChemicalBurnImmunity(B.fBoostValue); break;
			case eBoostExplImmunity: BoostExplImmunity(B.fBoostValue); break;
			case eBoostStrikeImmunity: BoostStrikeImmunity(B.fBoostValue); break;
			case eBoostFireWoundImmunity: BoostFireWoundImmunity(B.fBoostValue); break;
			case eBoostWoundImmunity: BoostWoundImmunity(B.fBoostValue); break;
			case eBoostRadiationProtection: BoostRadiationProtection(B.fBoostValue); break;
			case eBoostTelepaticProtection: BoostTelepaticProtection(B.fBoostValue); break;
			case eBoostChemicalBurnProtection: BoostChemicalBurnProtection(B.fBoostValue); break;
			default: NODEFAULT;	
		}
	}
}
void CActorCondition::DisableBoostParameters(const SBooster& B)
{
	if(!OnServer())
		return;

	switch(B.m_type)
	{
		case eBoostHpRestore: BoostHpRestore(-B.fBoostValue); break;
		case eBoostPowerRestore: BoostPowerRestore(-B.fBoostValue); break;
		case eBoostRadiationRestore: BoostRadiationRestore(-B.fBoostValue); break;
		case eBoostBleedingRestore: BoostBleedingRestore(-B.fBoostValue); break;
		case eBoostMaxWeight: BoostMaxWeight(-B.fBoostValue); break;
		case eBoostBurnImmunity: BoostBurnImmunity(-B.fBoostValue); break;
		case eBoostShockImmunity: BoostShockImmunity(-B.fBoostValue); break;
		case eBoostRadiationImmunity: BoostRadiationImmunity(-B.fBoostValue); break;
		case eBoostTelepaticImmunity: BoostTelepaticImmunity(-B.fBoostValue); break;
		case eBoostChemicalBurnImmunity: BoostChemicalBurnImmunity(-B.fBoostValue); break;
		case eBoostExplImmunity: BoostExplImmunity(-B.fBoostValue); break;
		case eBoostStrikeImmunity: BoostStrikeImmunity(-B.fBoostValue); break;
		case eBoostFireWoundImmunity: BoostFireWoundImmunity(-B.fBoostValue); break;
		case eBoostWoundImmunity: BoostWoundImmunity(-B.fBoostValue); break;
		case eBoostRadiationProtection: BoostRadiationProtection(-B.fBoostValue); break;
		case eBoostTelepaticProtection: BoostTelepaticProtection(-B.fBoostValue); break;
		case eBoostChemicalBurnProtection: BoostChemicalBurnProtection(-B.fBoostValue); break;
		default: NODEFAULT;	
	}
}
void CActorCondition::BoostHpRestore(const float value)
{
	m_change_v.m_fV_HealthRestore += value;
}
void CActorCondition::BoostPowerRestore(const float value)
{
	m_fV_SatietyPower += value;
}
void CActorCondition::BoostRadiationRestore(const float value)
{
	m_change_v.m_fV_Radiation += value;
}
void CActorCondition::BoostBleedingRestore(const float value)
{
	m_change_v.m_fV_WoundIncarnation += value;
}
void CActorCondition::BoostMaxWeight(const float value)
{
	m_object->inventory().SetMaxWeight(object().inventory().GetMaxWeight()+value);
	m_MaxWalkWeight += value;
}
void CActorCondition::BoostBurnImmunity(const float value)
{
	m_fBoostBurnImmunity += value;
}
void CActorCondition::BoostShockImmunity(const float value)
{
	m_fBoostShockImmunity += value;
}
void CActorCondition::BoostRadiationImmunity(const float value)
{
	m_fBoostRadiationImmunity += value;
}
void CActorCondition::BoostTelepaticImmunity(const float value)
{
	m_fBoostTelepaticImmunity += value;
}
void CActorCondition::BoostChemicalBurnImmunity(const float value)
{
	m_fBoostChemicalBurnImmunity += value;
}
void CActorCondition::BoostExplImmunity(const float value)
{
	m_fBoostExplImmunity += value;
}
void CActorCondition::BoostStrikeImmunity(const float value)
{
	m_fBoostStrikeImmunity += value;
}
void CActorCondition::BoostFireWoundImmunity(const float value)
{
	m_fBoostFireWoundImmunity += value;
}
void CActorCondition::BoostWoundImmunity(const float value)
{
	m_fBoostWoundImmunity += value;
}
void CActorCondition::BoostRadiationProtection(const float value)
{
	m_fBoostRadiationProtection += value;
}
void CActorCondition::BoostTelepaticProtection(const float value)
{
	m_fBoostTelepaticProtection += value;
}
void CActorCondition::BoostChemicalBurnProtection(const float value)
{
	m_fBoostChemicalBurnProtection += value;
}

void CActorCondition::UpdateTutorialThresholds()
{
}

bool CActorCondition::ApplyBooster(const SBooster& B, const shared_str& sect)
{
	if(B.fBoostValue>0.0f)
	{
		if (m_object->Local() && m_object == Level().CurrentViewEntity())
		{
			if(pSettings->line_exist(sect, "use_sound"))
			{
				if(m_use_sound._feedback())
					m_use_sound.stop		();

				shared_str snd_name			= pSettings->r_string(sect, "use_sound");
				m_use_sound.create			(snd_name.c_str(), st_Effect, sg_SourceType);
				m_use_sound.play			(NULL, sm_2D);
			}
		}

		BOOSTER_MAP::iterator it = m_booster_influences.find(B.m_type);
		if(it!=m_booster_influences.end())
			DisableBoostParameters((*it).second);

		m_booster_influences[B.m_type] = B;
		BoostParameters(B);
	}
	return true;
}