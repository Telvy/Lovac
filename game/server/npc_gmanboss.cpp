//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The G-Man, misunderstood servant of the people (but more business casual B) )
//
// $NoKeywords: $
//
//=============================================================================//


//-----------------------------------------------------------------------------
// Generic NPC - purely for scripted sequence work.
//-----------------------------------------------------------------------------
#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_behavior_follow.h"
#include "ai_playerally.h"
//#include "npc_gmanboss.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// NPC's Anim Events Go Here
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_GManBoss : public CAI_PlayerAlly
{
public:
	DECLARE_CLASS(CNPC_GManBoss, CAI_PlayerAlly);
	DECLARE_DATADESC();

	void	Spawn(void);
	void	Precache(void);
	Class_T Classify(void);
	void	HandleAnimEvent(animevent_t* pEvent);
	virtual Disposition_t IRelationType(CBaseEntity* pTarget);
	int		GetSoundInterests(void);
	bool	CreateBehaviors(void);
	int		SelectSchedule(void);
	void	SelectModel(void);

private:
	CAI_FollowBehavior	m_FollowBehavior;
	//GmanBossType_t m_GmanType;
};

LINK_ENTITY_TO_CLASS(npc_gmanboss, CNPC_GManBoss);

BEGIN_DATADESC(CNPC_GManBoss)
// (auto saved by AI)
//	DEFINE_FIELD( m_FollowBehavior, FIELD_EMBEDDED ),	(auto saved by AI)
END_DATADESC()

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_GManBoss::Classify(void)
{
	return CLASS_PLAYER_ALLY_VITAL;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_GManBoss::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_GManBoss::GetSoundInterests(void)
{
	return NULL;
}


//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_GManBoss::Spawn()
{
	Precache();

	SetModel(STRING(GetModelName()));

	BaseClass::Spawn();

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	SetBloodColor(BLOOD_COLOR_RED);
	m_iHealth = 8;
	m_flFieldOfView = 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;
	SetImpactEnergyScale(0.0f); // no physics damage on the gman

	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD);
	CapabilitiesAdd(bits_CAP_FRIENDLY_DMG_IMMUNE);
	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL);

	NPCInit();
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_GManBoss::Precache()
{
	if (!GetModelName())
	{
		SetModelName(MAKE_STRING("models/gman_shirt.mdl"));
	}

	PrecacheModel(STRING(GetModelName()));

	//SelectModel();

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// The G-Man isn't scared of anything.
//-----------------------------------------------------------------------------
Disposition_t CNPC_GManBoss::IRelationType(CBaseEntity* pTarget)
{
	return D_NU;
}


//=========================================================
// Purpose:
//=========================================================
bool CNPC_GManBoss::CreateBehaviors()
{
	AddBehavior(&m_FollowBehavior);

	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_GManBoss::SelectSchedule(void)
{
	if (!BehaviorSelectSchedule())
	{
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Select between normal gmanboss or damaged gmanboss
//-----------------------------------------------------------------------------
//void CNPC_GManBoss::SelectModel()
//{
//	switch (m_GmanType)
//	{
//	case GT_DEFAULT:
//		SetModelName(MAKE_STRING("models/gman_shirt.mdl"));
//		break;
//	case GT_DAMAGED:
//		SetModelName(MAKE_STRING("models/gman_shirt_damaged.mdl"));
//		break;
//
//	default:
//		break;
//
//	}
//
//}