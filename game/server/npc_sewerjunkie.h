//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The downtrodden citizens of City 17. Timid when unarmed, they will
//			rise up against their Combine oppressors when given a weapon.
//
//=============================================================================//

#ifndef	NPC_SEWERJUNKIE_H
#define	NPC_SEWERJUNKIE_H

#include "ai_baseactor.h"
#include "npc_playercompanion.h"
#include "ai_behavior_holster.h"
#include "soundent.h"
#include "ai_behavior_functank.h"

class CNPC_Junkie : public CNPC_PlayerCompanion
{
	
public:
	DECLARE_CLASS(CNPC_Junkie, CNPC_PlayerCompanion);
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	void	Precache(void);
	void	Spawn(void);
	void	SelectModel();
	Class_T Classify(void);
	void	Weapon_Equip(CBaseCombatWeapon* pWeapon);

	bool CreateBehaviors(void);

	void HandleAnimEvent(animevent_t* pEvent);

	bool ShouldLookForBetterWeapon() { return false; }

	void OnChangeRunningBehavior(CAI_BehaviorBase* pOldBehavior, CAI_BehaviorBase* pNewBehavior);

	void DeathSound(const CTakeDamageInfo& info);
	void GatherConditions();
	void UseFunc(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;

	int m_iHealthOverride;

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_SEWERJUNKIE_H
