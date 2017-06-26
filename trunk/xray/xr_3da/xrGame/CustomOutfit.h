#pragma once

#include "inventory_item_object.h"
#include "Nightvision.h"
#include "..\..\..\build_config_defines.h"

#ifndef FIRE_WOUND_HIT_FIXED
struct SBoneProtections;
#endif

class CCustomOutfit: public CInventoryItemObject {
	friend class COutfitScript;
private:
    typedef	CInventoryItemObject inherited;
public:
									CCustomOutfit		(void);
	virtual							~CCustomOutfit		(void);

	virtual void					Load				(LPCSTR section);
	
	//уменьшенная версия хита, для вызова, когда костюм надет на персонажа
	virtual void					Hit					(float P, ALife::EHitType hit_type);

	//коэффициенты на которые домножается хит
	//при соответствующем типе воздействия
	//если на персонаже надет костюм
	float							GetHitTypeProtection(ALife::EHitType hit_type, s16 element);
	float							GetDefHitTypeProtection(ALife::EHitType hit_type);

#ifndef FIRE_WOUND_HIT_FIXED
	float							HitThruArmour		(float hit_power, s16 element, float AP);
#endif

	//коэффициент на который домножается потеря силы
	//если на персонаже надет костюм
	float							GetPowerLoss		();


	virtual void					OnMoveToSlot		();
	virtual void					OnMoveToRuck		();
	virtual void					UpdateCL			();

protected:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
	float							m_fPowerLoss;

	shared_str						m_ActorVisual;
	shared_str						m_full_icon_name;

#ifndef FIRE_WOUND_HIT_FIXED
	SBoneProtections*				m_boneProtection;
#endif

protected:
	u32								m_ef_equipment_type;
	CNightVisionDevice*				m_NightVisionDevice;

public:
	float							m_additional_weight;
	float							m_additional_weight2;
	
	float					        m_fHealthRestoreSpeed;
	float 					        m_fRadiationRestoreSpeed;
	float 					        m_fSatietyRestoreSpeed;
    float 					        m_fThirstRestoreSpeed;
	float					        m_fPowerRestoreSpeed;
	float					        m_fBleedingRestoreSpeed;
	u32						        m_artefact_count;
	
	bool					        bIsHelmetAvaliable;
	bool					        bIsNightvisionAvaliable;

	virtual u32						ef_equipment_type		() const;
	CNightVisionDevice*				NightVisionDevice		() { return m_NightVisionDevice; };

#ifndef FIRE_WOUND_HIT_FIXED
	virtual	BOOL					BonePassBullet			(int boneID);
#endif

	const shared_str&				GetFullIconName			() const	{return m_full_icon_name;};
	u32						        get_artefact_count		() const	{ return m_artefact_count; }

	virtual void			net_Export			(NET_Packet& P);
	virtual void			net_Import			(NET_Packet& P);
};