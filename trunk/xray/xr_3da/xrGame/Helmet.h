#pragma once

#include "inventory_item_object.h"
#include "Nightvision.h"

class CHelmet:	public CInventoryItemObject {
private:
	typedef			CInventoryItemObject	inherited;

public:								CHelmet						();
	virtual							~CHelmet					();
	virtual void					Load						(LPCSTR section);
	virtual BOOL					net_Spawn					(CSE_Abstract* DC);
	virtual CHelmet*				cast_helmet 				()		{ return this; }

protected:
	CNightVisionDevice*				m_NightVisionDevice;

public:
	bool					        bIsNightvisionAvaliable;
	CNightVisionDevice*				NightVisionDevice		    ()       { return m_NightVisionDevice; };
	virtual void				    OnMoveToSlot		        ();
	virtual void					OnMoveToRuck		        ();
	virtual void					UpdateCL			        ();
};
