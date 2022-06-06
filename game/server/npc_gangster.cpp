//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Gangster enemy for Lovac: 2063
//
//=============================================================================//

#include "cbase.h"

#include "npc_citizen17.h"
#include "npc_gangster.h"


#include "ammodef.h"
#include "globalstate.h"
#include "soundent.h"
#include "BasePropDoor.h"
#include "weapon_rpg.h"
#include "hl2_player.h"
#include "items.h"

#include "eventqueue.h"

#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_hint.h"
#include "ai_interactions.h"
#include "ai_looktarget.h"
#include "sceneentity.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_gangster_health("sk_gangster_health", "80");

//=========================================================
// NPC activities
//=========================================================

LINK_ENTITY_TO_CLASS(npc_gangster, CNPC_Gangster);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_Gangster)
DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_USEFUNC(UseFunc),
DEFINE_KEYFIELD(m_iHealthOverride, FIELD_INTEGER, "HealthOverride"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Gangster::Spawn(void)
{
	Precache();

	if (m_iHealthOverride > 0)
	{
		m_iHealth = m_iHealthOverride;
	}
	else
	{
		m_iHealth = sk_gangster_health.GetFloat();
	}

	//m_iszIdleExpression = MAKE_STRING("scenes/Expressions/BarneyIdle.vcd");
	//m_iszAlertExpression = MAKE_STRING("scenes/Expressions/BarneyAlert.vcd");
	//m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	NPCInit();

	SetUse(&CNPC_Gangster::UseFunc);
}

void CNPC_Gangster::Precache(void)
{
	SelectModel();
	BaseClass::Precache();

	PrecacheScriptSound("NPC_Barney.FootstepLeft");
	PrecacheScriptSound("NPC_Barney.FootstepRight");

	PrecacheInstancedScene("scenes/Expressions/BarneyIdle.vcd");
	PrecacheInstancedScene("scenes/Expressions/BarneyAlert.vcd");
	PrecacheInstancedScene("scenes/Expressions/BarneyCombat.vcd");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Gangster::SelectModel()
{
	//If no model specified, it will load this one.
	if (!GetModelName())
	{
		SetModelName(MAKE_STRING("models/humans/gangsters/smiley.mdl"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Gangster::Classify(void)
{
	return	CLASS_COMBINE;
}

Disposition_t CNPC_Gangster::IRelationType(CBaseEntity* pTarget)
{
	return D_HT;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Gangster::Weapon_Equip(CBaseCombatWeapon* pWeapon)
{
	BaseClass::Weapon_Equip(pWeapon);

	if (hl2_episodic.GetBool() && FClassnameIs(pWeapon, "weapon_ar2"))
	{
		// Allow Barney to defend himself at point-blank range in c17_05.
		pWeapon->m_fMinRange1 = 0.0f;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Gangster::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case NPC_EVENT_LEFTFOOT:
	{
		EmitSound("NPC_Barney.FootstepLeft", pEvent->eventtime);
	}
	break;
	case NPC_EVENT_RIGHTFOOT:
	{
		EmitSound("NPC_Barney.FootstepRight", pEvent->eventtime);
	}
	break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Gangster::DeathSound(const CTakeDamageInfo& info)
{
	// Sentences don't play on dead NPCs
	SentenceStop();

}

bool CNPC_Gangster::CreateBehaviors(void)
{
	BaseClass::CreateBehaviors();
	AddBehavior(&m_FuncTankBehavior);

	return true;
}

void CNPC_Gangster::OnChangeRunningBehavior(CAI_BehaviorBase* pOldBehavior, CAI_BehaviorBase* pNewBehavior)
{
	if (pNewBehavior == &m_FuncTankBehavior)
	{
		m_bReadinessCapable = false;
	}
	else if (pOldBehavior == &m_FuncTankBehavior)
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior(pOldBehavior, pNewBehavior);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Gangster::GatherConditions()
{
	BaseClass::GatherConditions();

	// Handle speech AI. Don't do AI speech if we're in scripts unless permitted by the EnableSpeakWhileScripting input.
	if (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ||
		((m_NPCState == NPC_STATE_SCRIPT) && CanSpeakWhileScripting()))
	{
		DoCustomSpeechAI();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Gangster::UseFunc(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_bDontUseSemaphore = true;
	SpeakIfAllowed(TLK_USE);
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput(pActivator, pCaller);
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_gangster, CNPC_Gangster)

AI_END_CUSTOM_NPC()