//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  Gangster enemy for Lovac: 2063
//
//=============================================================================//

#ifndef	NPC_GANGSTER_H
#define	NPC_GANGSTER_H

#include "ai_baseactor.h"
#include "npc_playercompanion.h"
#include "ai_behavior_holster.h"
#include "ai_behavior_functank.h"

class CNPC_Gangster : public CNPC_PlayerCompanion
{

public:
	DECLARE_CLASS(CNPC_Gangster, CNPC_PlayerCompanion);
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

	Disposition_t	IRelationType(CBaseEntity* pTarget);

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;

	int m_iHealthOverride;

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_SEWERJUNKIE_H