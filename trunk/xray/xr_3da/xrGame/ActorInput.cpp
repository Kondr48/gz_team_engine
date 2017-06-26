#include "stdafx.h"
#include <dinput.h>
#include "Actor.h"
#include "Torch.h"
#include "Nightvision.h"
#include "trade.h"
#include "Helmet.h"
#include "../CameraBase.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#include "hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "HudManager.h"
#include "UIGameSP.h"
#include "inventory.h"
#include "level.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "UsableScriptObject.h"
#include "clsid_game.h"
#include "actorcondition.h"
#include "actor_input_handler.h"
#include "string_table.h"
#include "UI/UIStatic.h"
#include "CharacterPhysicsSupport.h"
#include "InventoryBox.h"
#include "../../build_config_defines.h"
#include "pch_script.h"
#include "InventoryOwner.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "silencer.h"
#include "scope.h"
#include "grenadelauncher.h"
#include "Artifact.h"
#include "eatable_item.h"
#include "BottleItem.h"
#include "medkit.h"
#include "antirad.h"
#include "CustomOutfit.h"
#include "WeaponMagazined.h"

#include "WeaponKnife.h"

bool g_bAutoClearCrouch = true;

#include "script_engine.h"
void CActor::IR_OnKeyboardPress(int cmd)
{
	if (m_blocked_actions.find((EGameActions)cmd) != m_blocked_actions.end() ) return; // Real Wolf. 14.10.2014

	if (Remote())		return;

//	if (conditions().IsSleeping())	return;
	if (IsTalking())	return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;
	
	switch (cmd)
	{
	case kWPN_FIRE:
		{
			mstate_wishful &=~mcSprint;
			//-----------------------------
			if (OnServer())
			{
				NET_Packet P;
				P.w_begin(M_PLAYER_FIRE); 
				P.w_u16(ID());
				u_EventSend(P);
			}
		}break;
	default:
		{
		}break;
	}

	if (!g_Alive()) return;

	if(m_holder && kUSE != cmd)
	{
		m_holder->OnKeyboardPress			(cmd);
		if(m_holder->allowWeapon() && inventory().Action(cmd, CMD_START))		return;
		return;
	}else
		if(inventory().Action(cmd, CMD_START))					return;

	switch(cmd){
	case kJUMP:		
		{
			mstate_wishful |= mcJump;
			{
//				NET_Packet	P;
//				u_EventGen(P, GE_ACTOR_JUMPING, ID());
//				u_EventSend(P);
			}
		}break;
	case kCROUCH_TOGGLE:
		{
			g_bAutoClearCrouch = !g_bAutoClearCrouch;
			if (!g_bAutoClearCrouch)
				mstate_wishful |= mcCrouch;

		}break;
	case kSPRINT_TOGGLE:	
		{
			if (mstate_wishful & mcSprint)
				mstate_wishful &=~mcSprint;
			else
				mstate_wishful |= mcSprint;					
		}break;
	case kCAM_1:	cam_Set			(eacFirstEye);				break;
	case kCAM_2:	cam_Set			(eacLookAt);				break;
	case kCAM_3:	cam_Set			(eacFreeLook);				break;
	case kNIGHT_VISION:{                                                       // Kondr48: ПНВ вообще у нас может быть в двух случаях:
		PIItem nvd_in_outfit  = inventory().ItemFromSlot(OUTFIT_SLOT);         // ПНВ встроен в костюм, получаем предмет в слоте броник
		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(nvd_in_outfit);     // Убедимся, что это именно броник (ну а мало ли, хотя что там может быть еще?)
		if (nvd_in_outfit && outfit && outfit->NightVisionDevice())            // Если в слоте есть бронежилет, у него есть ПНВ
		{
			outfit->NightVisionDevice()->SwitchNightVision();                  // Вызываем переключение состояния ПНВ
			break;                                                             // Дальше делать ничего не нужно, прерываем функцию
		} 
		PIItem nvd_in_helmet = inventory().ItemFromSlot(HELMET_SLOT);          // ПНВ встроен в шлем, получаем предмет в слоте шлем
		CHelmet* cast_helmet = smart_cast<CHelmet*>(nvd_in_helmet);            // Убедимся, что это именно шлем (ну а мало ли, хотя что там может быть еще?)
		if (nvd_in_helmet && cast_helmet && cast_helmet->NightVisionDevice())  // Если в слоте есть шлем, у него есть ПНВ
		{
			cast_helmet->NightVisionDevice()->SwitchNightVision();             // Вызываем переключение состояния ПНВ
			break;                                                             // Дальше делать ничего не нужно, прерываем функцию
		} 
		PIItem nvd_portable = inventory().ItemFromSlot(NIGHTVISION_SLOT);      // ПНВ в слоте, то бишь портативное
		CNightVisionDevice* nvd = smart_cast<CNightVisionDevice*>(nvd_portable);
		if (nvd_portable && nvd)                                               // Аналогично, в слоте ПНВ предмет, именно класса ПНВ
			nvd->SwitchNightVision();                                          // Переключаем состоняие.
		}break;                                                                // А здесь уже прерывается, как и у других клавиш
    case kCLOCK:
		{
		luabind::functor<void>	clock_key;
		if (ai().script_engine().functor("gz_items_hud.clock_key",clock_key))
		clock_key();
	}break;
	case kGAZMASK:
		{
		PIItem helmet = inventory().ItemFromSlot(HELMET_SLOT);
		if (helmet && inventory().GetActiveSlot() != 11)
			inventory().Activate(11);
	}break;
	case kDET1:{
		luabind::functor<void>	det_one_activate;
		if (ai().script_engine().functor("gz_items_hud.det_one_activate",det_one_activate))
		det_one_activate();
	    }break;
    case kDET2:{
		luabind::functor<void>	det_two_activate;
		if (ai().script_engine().functor("gz_items_hud.det_two_activate",det_two_activate))
		det_two_activate();
	    }break;	
	case kTORCH:{ 
	    PIItem torch_in_slot  = inventory().ItemFromSlot(TORCH_SLOT);  // Несмотря на то, что фонарь сейчас работает на скриптах
		CTorch* torch = smart_cast<CTorch*>(torch_in_slot);            // Проверять заряд мы будем здесь
		if (torch && torch->GetBattareyPower() > 0)                    // Если батарейка села, функция тупо не запускается
		    {
		        luabind::functor<void>	flashlight_key;
		        if (ai().script_engine().functor("gz_items_hud.flashlight_key",flashlight_key))
		        flashlight_key();
		    }
	}break;
	case kWPN_1:	
	case kWPN_2:	
	case kWPN_3:	
	case kWPN_4:	
	case kWPN_5:	
	case kWPN_6:	
	case kWPN_RELOAD:
		//Weapons->ActivateWeaponID	(cmd-kWPN_1);			
		break;
	case kUSE:
		ActorUse();
		break;
	case kDROP:
		b_DropActivated			= TRUE;
		f_DropPower				= 0;
		break;
	case kNEXT_SLOT:
		{
			OnNextWeaponSlot();
		}break;
	case kPREV_SLOT:
		{
			OnPrevWeaponSlot();
		}break;

	case kUSE_BANDAGE:
	case kUSE_MEDKIT:
		{
			if(IsGameTypeSingle())
			{
				PIItem itm = inventory().item((cmd==kUSE_BANDAGE)?  CLSID_IITEM_BANDAGE:CLSID_IITEM_MEDKIT );	
				if(itm)
				{
					inventory().Eat				(itm);
					SDrawStaticStruct* _s		= HUD().GetUI()->UIGame()->AddCustomStatic("item_used", true);
					_s->m_endTime				= Device.fTimeGlobal+3.0f;// 3sec
					string1024					str;
					strconcat					(sizeof(str),str,*CStringTable().translate("st_item_used"),": ", itm->Name());
					_s->wnd()->SetText			(str);
				}
			}
		}break;
	case kUSE_SLOT_QUICK_ACCESS_0:
	case kUSE_SLOT_QUICK_ACCESS_1:
	case kUSE_SLOT_QUICK_ACCESS_2:
	case kUSE_SLOT_QUICK_ACCESS_3:
		{
			if(IsGameTypeSingle())
			{
				PIItem itm = 0;
				switch (cmd){
				case kUSE_SLOT_QUICK_ACCESS_0:
					
					itm = inventory().m_slots[SLOT_QUICK_ACCESS_0].m_pIItem;
					break;
				case kUSE_SLOT_QUICK_ACCESS_1:	
					itm = inventory().m_slots[SLOT_QUICK_ACCESS_1].m_pIItem;
					break;
				case kUSE_SLOT_QUICK_ACCESS_2:
					itm = inventory().m_slots[SLOT_QUICK_ACCESS_2].m_pIItem;
					break;
				case kUSE_SLOT_QUICK_ACCESS_3:
					itm = inventory().m_slots[SLOT_QUICK_ACCESS_3].m_pIItem;
					break;					
				}

				if (itm){
					CMedkit*			pMedkit				= smart_cast<CMedkit*>			(itm);
					CAntirad*			pAntirad			= smart_cast<CAntirad*>			(itm);
					CEatableItem*		pEatableItem		= smart_cast<CEatableItem*>		(itm);
					CBottleItem*		pBottleItem			= smart_cast<CBottleItem*>		(itm);				
					string1024					str;
					
					if(pMedkit || pAntirad || pEatableItem || pBottleItem){
						PIItem iitm = inventory().Same(itm,true);
						if(iitm){
							inventory().Eat(iitm);
							strconcat(sizeof(str),str,*CStringTable().translate("st_item_used"),": ", iitm->Name());
						}else{
							inventory().Eat(itm);
							strconcat(sizeof(str),str,*CStringTable().translate("st_item_used"),": ", itm->Name());
						}
						
						SDrawStaticStruct* _s		= HUD().GetUI()->UIGame()->AddCustomStatic("item_used", true);
						_s->m_endTime				= Device.fTimeGlobal+3.0f;// 3sec
						_s->wnd()->SetText			(str);
					}
				}
			}
		}break;			
	}
}
void CActor::IR_OnMouseWheel(int direction)
{
	if(inventory().Action( (direction>0)? kWPN_ZOOM_DEC:kWPN_ZOOM_INC , CMD_START)) return;

	if (direction>0)
		OnNextWeaponSlot				();
	else
		OnPrevWeaponSlot				();
}
void CActor::IR_OnKeyboardRelease(int cmd)
{
	if (m_blocked_actions.find((EGameActions)cmd) != m_blocked_actions.end() ) return; // Real Wolf. 14.10.2014

	if (Remote())		return;

//	if (conditions().IsSleeping())	return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;

	if (g_Alive())	
	{
		if (cmd == kUSE) 
			PickupModeOff();

		if(m_holder)
		{
			m_holder->OnKeyboardRelease(cmd);
			
			if(m_holder->allowWeapon() && inventory().Action(cmd, CMD_STOP))		return;
			return;
		}else
			if(inventory().Action(cmd, CMD_STOP))		return;



		switch(cmd)
		{
		case kJUMP:		mstate_wishful &=~mcJump;		break;
		case kDROP:		if(GAME_PHASE_INPROGRESS == Game().Phase()) g_PerformDrop();				break;
		case kCROUCH:	g_bAutoClearCrouch = true;
		}
	}
}

void CActor::IR_OnKeyboardHold(int cmd)
{
	if (m_blocked_actions.find((EGameActions)cmd) != m_blocked_actions.end() ) return; // Real Wolf. 14.10.2014

	if (Remote() || !g_Alive())					return;
//	if (conditions().IsSleeping())				return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;
	if (IsTalking())							return;

	if(m_holder)
	{
		m_holder->OnKeyboardHold(cmd);
		return;
	}

	float LookFactor = GetLookFactor();
	switch(cmd)
	{
	case kUP:
	case kDOWN: 
		cam_Active()->Move( (cmd==kUP) ? kDOWN : kUP, 0, LookFactor);									break;
	case kCAM_ZOOM_IN: 
	case kCAM_ZOOM_OUT: 
		cam_Active()->Move(cmd);												break;
	case kLEFT:
	case kRIGHT:
		if (eacFreeLook!=cam_active) cam_Active()->Move(cmd, 0, LookFactor);	break;

	case kACCEL:	mstate_wishful |= mcAccel;									break;
	case kL_STRAFE:	mstate_wishful |= mcLStrafe;								break;
	case kR_STRAFE:	mstate_wishful |= mcRStrafe;								break;
	case kL_LOOKOUT:mstate_wishful |= mcLLookout;								break;
	case kR_LOOKOUT:mstate_wishful |= mcRLookout;								break;
	case kFWD:		mstate_wishful |= mcFwd;									break;
	case kBACK:		mstate_wishful |= mcBack;									break;
	case kCROUCH:	mstate_wishful |= mcCrouch;									break;


	}
}

void CActor::IR_OnMouseMove(int dx, int dy)
{
	if (Remote())		return;
//	if (conditions().IsSleeping())	return;

	if(m_holder) 
	{
		m_holder->OnMouseMove(dx,dy);
		return;
	}

	float LookFactor = GetLookFactor();

	CCameraBase* C	= cameras	[cam_active];
	float scale		= (C->f_fov/g_fov)*psMouseSens * psMouseSensScale/50.f  / LookFactor;
	if (dx){
		float d = float(dx)*scale;
		cam_Active()->Move((d<0)?kLEFT:kRIGHT, _abs(d));
	}
	if (dy){
		float d = ((psMouseInvert.test(1))?-1:1)*float(dy)*scale*3.f/4.f;
		cam_Active()->Move((d>0)?kUP:kDOWN, _abs(d));
	}
}
#include "HudItem.h"
bool CActor::use_Holder				(CHolderCustom* holder)
{

	if(m_holder){
		bool b = false;
		CGameObject* holderGO			= smart_cast<CGameObject*>(m_holder);
		
		if(smart_cast<CCar*>(holderGO))
			b = use_Vehicle(0);
		else
			if (holderGO->CLS_ID==CLSID_OBJECT_W_MOUNTED ||
				holderGO->CLS_ID==CLSID_OBJECT_W_STATMGUN)
				b = use_MountedWeapon(0);

		if(inventory().ActiveItem()){
			CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
			if(hi) hi->OnAnimationEnd(hi->GetState());
		}

		return b;
	}else{
		bool b = false;
		CGameObject* holderGO			= smart_cast<CGameObject*>(holder);
		if(smart_cast<CCar*>(holder))
			b = use_Vehicle(holder);

		if (holderGO->CLS_ID==CLSID_OBJECT_W_MOUNTED ||
			holderGO->CLS_ID==CLSID_OBJECT_W_STATMGUN)
			b = use_MountedWeapon(holder);
		
		if(b){//used succesfully
			// switch off torch...
			CAttachableItem *I = CAttachmentOwner::attachedItem(CLSID_DEVICE_TORCH);
			if (I){
				CTorch* torch = smart_cast<CTorch*>(I);
				if (torch) torch->Switch(false);
			}
		}

		if(inventory().ActiveItem()){
			CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
			if(hi) hi->OnAnimationEnd(hi->GetState());
		}

		return b;
	}
}

void CActor::ActorUse()
{
	//mstate_real = 0;
	PickupModeOn();

		
	if (m_holder)
	{
		CGameObject*	GO			= smart_cast<CGameObject*>(m_holder);
		NET_Packet		P;
		CGameObject::u_EventGen		(P, GEG_PLAYER_DETACH_HOLDER, ID());
		P.w_u32						(GO->ID());
		CGameObject::u_EventSend	(P);
		return;
	}
				
	if(character_physics_support()->movement()->PHCapture())
		character_physics_support()->movement()->PHReleaseObject();

	

	if(m_pUsableObject)m_pUsableObject->use(this);
	
	if(m_pInvBoxWeLookingAt && m_pInvBoxWeLookingAt->nonscript_usable())
	{
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
		if(pGameSP) pGameSP->StartCarBody(this, m_pInvBoxWeLookingAt );
		return;
	}

	if(!m_pUsableObject||m_pUsableObject->nonscript_usable())
	{
		if(m_pPersonWeLookingAt)
		{
			CEntityAlive* pEntityAliveWeLookingAt = 
				smart_cast<CEntityAlive*>(m_pPersonWeLookingAt);

			VERIFY(pEntityAliveWeLookingAt);

			if (GameID()==GAME_SINGLE)
			{			
				if(pEntityAliveWeLookingAt->g_Alive())
				{
					TryToTalk();
				}
				//обыск трупа
				else  if(!Level().IR_GetKeyState(DIK_LSHIFT))
				{
					//только если находимся в режиме single
					CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
					if(pGameSP)pGameSP->StartCarBody(this, m_pPersonWeLookingAt );
				}
			}
		}

		collide::rq_result& RQ = HUD().GetCurrentRayQuery();
		CPhysicsShellHolder* object = smart_cast<CPhysicsShellHolder*>(RQ.O);
		u16 element = BI_NONE;
		if(object) 
			element = (u16)RQ.element;

		if(object && Level().IR_GetKeyState(DIK_LSHIFT))
		{
			bool b_allow = !!pSettings->line_exist("ph_capture_visuals",object->cNameVisual());
			if(b_allow && !character_physics_support()->movement()->PHCapture())
			{
				character_physics_support()->movement()->PHCaptureObject(object,element);

			}

		}
		else
		{
			if (object && smart_cast<CHolderCustom*>(object))
			{
					NET_Packet		P;
					CGameObject::u_EventGen		(P, GEG_PLAYER_ATTACH_HOLDER, ID());
					P.w_u32						(object->ID());
					CGameObject::u_EventSend	(P);
					return;
			}

		}
	}


}
BOOL CActor::HUDview				( )const 
{ 
	return IsFocused()
		&&(cam_active==eacFirstEye)		
		&&((!m_holder) || (m_holder && m_holder->allowWeapon() && m_holder->HUDView() ) ); 
}

//Список слотов, которые активируются на колёсико мышки
static	u32 SlotsToCheck [] = {
		KNIFE_SLOT		  ,	
		PISTOL_SLOT		  ,
		RIFLE_SLOT		  ,
		GRENADE_SLOT	  ,
		APPARATUS_SLOT	  ,
		BOLT_SLOT		  ,
		DETECTOR_ONE_SLOT , //Kondr48: добавил слоты детекторов в список, убрал не нужный слот артефактов
		DETECTOR_TWO_SLOT ,
};

void	CActor::OnNextWeaponSlot()
{
	u32 ActiveSlot = inventory().GetActiveSlot();
	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = KNIFE_SLOT;
	
	u32 NumSlotsToCheck = sizeof(SlotsToCheck)/sizeof(u32);	
	for (u32 CurSlot=0; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	};
	if (CurSlot >= NumSlotsToCheck) return;
	for (u32 i=CurSlot+1; i<NumSlotsToCheck; i++)
	{
		if (inventory().ItemFromSlot(SlotsToCheck[i]))
		{
			if (SlotsToCheck[i] == ARTEFACT_SLOT) 
			{
				IR_OnKeyboardPress(kARTEFACT);
			}
			else
				IR_OnKeyboardPress(kWPN_1+(i-KNIFE_SLOT));
			return;
		}
	}
};

void	CActor::OnPrevWeaponSlot()
{
	u32 ActiveSlot = inventory().GetActiveSlot();
	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = KNIFE_SLOT;

	u32 NumSlotsToCheck = sizeof(SlotsToCheck)/sizeof(u32);	
	for (u32 CurSlot=0; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	};
	if (CurSlot >= NumSlotsToCheck) return;
	for (s32 i=s32(CurSlot-1); i>=0; i--)
	{
		if (inventory().ItemFromSlot(SlotsToCheck[i]))
		{
			if (SlotsToCheck[i] == ARTEFACT_SLOT) 
			{
				IR_OnKeyboardPress(kARTEFACT);
			}
			else
				IR_OnKeyboardPress(kWPN_1+(i-KNIFE_SLOT));
			return;
		}
	}
};

float	CActor::GetLookFactor()
{
	if (m_input_external_handler) 
		return m_input_external_handler->mouse_scale_factor();

	
	float factor	= 1.f;

	PIItem pItem	= inventory().ActiveItem();

	if (pItem)
		factor *= pItem->GetControlInertionFactor();

	VERIFY(!fis_zero(factor));

	return factor;
}

void CActor::set_input_external_handler(CActorInputHandler *handler) 
{
	// clear state
	if (handler) 
		mstate_wishful			= 0;

	// release fire button
	if (handler)
		IR_OnKeyboardRelease	(kWPN_FIRE);

	// set handler
	m_input_external_handler	= handler;
}



