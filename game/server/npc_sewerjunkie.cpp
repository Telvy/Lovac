//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Protagonist Major Cast
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"
#include "npc_sewerjunkie.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_sewerjunkie_health("sk_sewerjunkie_health", "40");

//=========================================================
// NPC activities
//=========================================================

LINK_ENTITY_TO_CLASS(npc_sewerjunkie, CNPC_Junkie);

//---------------------------------------------------------
// 
//---------------------------------------------------------
//IMPLEMENT_SERVERCLASS_ST(CNPC_Junkie, DT_NPC_Protagonist_MC)
//END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_Junkie)
DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_USEFUNC(UseFunc),
DEFINE_KEYFIELD(m_iHealthOverride, FIELD_INTEGER, "HealthOverride"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Junkie::Spawn(void)
{
	Precache();

	if (m_iHealthOverride > 0)
	{
		m_iHealth = m_iHealthOverride;
	}
	else
	{
		m_iHealth = sk_sewerjunkie_health.GetFloat();
	}

	//m_iszIdleExpression = MAKE_STRING("scenes/Expressions/BarneyIdle.vcd");
	//m_iszAlertExpression = MAKE_STRING("scenes/Expressions/BarneyAlert.vcd");
	//m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	NPCInit();

	SetUse(&CNPC_Junkie::UseFunc);
}

void CNPC_Junkie::Precache(void)
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
void CNPC_Junkie::SelectModel()
{
	//If no model specified, it will load this one.
	if (!GetModelName())
	{
		SetModelName(MAKE_STRING("models/humans/Group01/male_07.mdl"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Junkie::Classify(void)
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Junkie::Weapon_Equip(CBaseCombatWeapon* pWeapon)
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
void CNPC_Junkie::HandleAnimEvent(animevent_t* pEvent)
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
void CNPC_Junkie::DeathSound(const CTakeDamageInfo& info)
{
	// Sentences don't play on dead NPCs
	SentenceStop();

}

bool CNPC_Junkie::CreateBehaviors(void)
{
	BaseClass::CreateBehaviors();
	AddBehavior(&m_FuncTankBehavior);

	return true;
}

void CNPC_Junkie::OnChangeRunningBehavior(CAI_BehaviorBase* pOldBehavior, CAI_BehaviorBase* pNewBehavior)
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
void CNPC_Junkie::GatherConditions()
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
void CNPC_Junkie::UseFunc(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
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

AI_BEGIN_CUSTOM_NPC(npc_sewerjunkie, CNPC_Junkie)

AI_END_CUSTOM_NPC()
