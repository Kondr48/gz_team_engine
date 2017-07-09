#pragma once

enum{
		AF_GODMODE			            = (1<<0),
		AF_INVISIBLE		            = (1<<1),
		AF_ALWAYSRUN		            = (1<<2),
		AF_UNLIMITEDAMMO	            = (1<<3),
		AF_RUN_BACKWARD		            = (1<<4),
		AF_AUTOPICKUP		            = (1<<5),
		AF_PSP				            = (1<<6),
		AF_STRAFE_INERT		            = (1<<7),
		AF_AUTO_LOSS                    = (1<<8),
		AF_AUTO_RELOAD                  = (1<<9),
		AF_ACTOR_PROTECTION_INFO_ENABLE = (1<<10),
};

extern Flags32 psActorFlags;

extern BOOL		GodMode	();	

