//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "grenade_frag.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "items.h"
#include "in_buttons.h"
#include "soundent.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER	3.0f //Seconds

#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define GRENADE_RADIUS	4.0f // inches

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponFrag, CBaseHLCombatWeapon);
public:
	DECLARE_SERVERCLASS();

public:
	CWeaponFrag();

	void	Precache(void);
	void	Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter* pOwner);
	void	ItemPostFrame(void);
	void	WeaponIdle(void);
	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);

	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	bool	Reload(void);

	bool	ShouldDisplayHUDHint() { return true; }


	//Stuff for Quick Grenade
	void	QGAttack(void);
	void	QGSuppressAW(void);
	void	ItemHolsterFrame(void);


private:
	void	ThrowGrenade(CBasePlayer* pPlayer);
	void	RollGrenade(CBasePlayer* pPlayer);
	void	LobGrenade(CBasePlayer* pPlayer);
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc);

	bool	m_bRedraw;	//Draw the weapon again after throwing a grenade

	//quick grenade
	bool	m_bInQGSequence = false;
	bool	m_bInQGThrow = false;
	bool	m_bQGWaitDraw = false;
	bool	m_bQGWaitHolster = false;

	float	m_flNextQGTime;
	float	m_flQGDrawbackTime;
	float	m_flQGHolsterTime;


	int		m_AttackPaused;
	bool	m_fDrawbackFinished;

	DECLARE_ACTTABLE();

	DECLARE_DATADESC();
};


BEGIN_DATADESC(CWeaponFrag)
DEFINE_FIELD(m_bRedraw, FIELD_BOOLEAN),
DEFINE_FIELD(m_AttackPaused, FIELD_INTEGER),
DEFINE_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN),

DEFINE_FIELD(m_flNextQGTime, FIELD_TIME),
DEFINE_FIELD(m_flQGDrawbackTime, FIELD_TIME),
DEFINE_FIELD(m_flQGHolsterTime, FIELD_TIME),
DEFINE_FIELD(m_bInQGSequence, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInQGThrow, FIELD_BOOLEAN),
DEFINE_FIELD(m_bQGWaitDraw, FIELD_BOOLEAN),
DEFINE_FIELD(m_bQGWaitHolster, FIELD_BOOLEAN),

END_DATADESC()

acttable_t	CWeaponFrag::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
};

IMPLEMENT_ACTTABLE(CWeaponFrag);

IMPLEMENT_SERVERCLASS_ST(CWeaponFrag, DT_WeaponFrag)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_frag, CWeaponFrag);
PRECACHE_WEAPON_REGISTER(weapon_frag);



CWeaponFrag::CWeaponFrag() :
	CBaseHLCombatWeapon(),
	m_bRedraw(false)
{
	NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::Precache(void)
{
	BaseClass::Precache();

	UTIL_PrecacheOther("npc_grenade_frag");
	PrecacheScriptSound("WeaponFrag.Throw");
	PrecacheScriptSound("WeaponFrag.Roll");

	PrecacheModel("models/weapons/v_quick_grenade.mdl");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFrag::Deploy(void)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponFrag::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	bool fThrewGrenade = false;

	switch (pEvent->event)
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED:
		m_fDrawbackFinished = true;
		break;

	case EVENT_WEAPON_THROW:
		ThrowGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	case EVENT_WEAPON_THROW2:
		RollGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	case EVENT_WEAPON_THROW3:
		LobGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}

#define RETHROW_DELAY	0.5
	if (fThrewGrenade)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!

		// Make a sound designed to scare snipers back into their holes!
		CBaseCombatCharacter* pOwner = GetOwner();

		if (pOwner)
		{
			Vector vecSrc = pOwner->Weapon_ShootPosition();
			Vector	vecDir;

			AngleVectors(pOwner->EyeAngles(), &vecDir);

			trace_t tr;

			UTIL_TraceLine(vecSrc, vecSrc + vecDir * 1024, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr);

			CSoundEnt::InsertSound(SOUND_DANGER_SNIPERONLY, tr.endpos, 384, 0.2, pOwner);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Reload(void)
{
	if (!HasPrimaryAmmo())
		return false;

	if ((m_bRedraw) && (m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		//Redraw the weapon
		SendWeaponAnim(ACT_VM_DRAW);

		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SecondaryAttack(void)
{
	if (m_bRedraw)
		return;

	if (!HasPrimaryAmmo())
		return;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	CBasePlayer* pPlayer = ToBasePlayer(pOwner);

	if (pPlayer == NULL)
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim(ACT_VM_PULLBACK_LOW);

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::PrimaryAttack(void)
{
	if (m_bRedraw)
		return;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
	{
		return;
	}

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());;

	if (!pPlayer)
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim(ACT_VM_PULLBACK_HIGH);

	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponFrag::DecrementAmmo(CBaseCombatCharacter* pOwner)
{
	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

void CWeaponFrag::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle <= gpGlobals->curtime && !m_bRedraw)
	{
		BaseClass::WeaponIdle();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemPostFrame(void)
{
	if (m_fDrawbackFinished)
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());

		if (pOwner)
		{
			switch (m_AttackPaused)
			{
			case GRENADE_PAUSED_PRIMARY:
				if (!(pOwner->m_nButtons & IN_ATTACK))
				{
					SendWeaponAnim(ACT_VM_THROW);
					m_fDrawbackFinished = false;
				}
				break;

			case GRENADE_PAUSED_SECONDARY:
				if (!(pOwner->m_nButtons & IN_ATTACK2))
				{
					//See if we're ducking
					if (pOwner->m_nButtons & IN_DUCK)
					{
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_SECONDARYATTACK);
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_HAULBACK);
					}

					m_fDrawbackFinished = false;
				}
				break;

			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if (m_bRedraw)
	{
		if (IsViewModelSequenceFinished())
		{
			Reload();
		}
	}
}

// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFrag::CheckThrowPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc)
{
	trace_t tr;

	UTIL_TraceHull(vecEye, vecSrc, -Vector(GRENADE_RADIUS + 2, GRENADE_RADIUS + 2, GRENADE_RADIUS + 2), Vector(GRENADE_RADIUS + 2, GRENADE_RADIUS + 2, GRENADE_RADIUS + 2),
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr);

	if (tr.DidHit())
	{
		vecSrc = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::ThrowGrenade(CBasePlayer* pPlayer)
{
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition(pPlayer, vecEye, vecSrc);
	//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vForward * 1200;
	Fraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer, GRENADE_TIMER, false);

	m_bRedraw = true;

	WeaponSound(SINGLE);

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::LobGrenade(CBasePlayer* pPlayer)
{
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector(0, 0, -8);
	CheckThrowPosition(pPlayer, vecEye, vecSrc);

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vForward * 350 + Vector(0, 0, 50);
	Fraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(200, random->RandomInt(-600, 600), 0), pPlayer, GRENADE_TIMER, false);

	WeaponSound(WPN_DOUBLE);

	m_bRedraw = true;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::RollGrenade(CBasePlayer* pPlayer)
{
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.0f), &vecSrc);
	vecSrc.z += GRENADE_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D();
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize(vecFacing);
	trace_t tr;
	UTIL_TraceLine(vecSrc, vecSrc - Vector(0, 0, 16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction != 1.0)
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct(vecFacing, tr.plane.normal, tangent);
		CrossProduct(tr.plane.normal, tangent, vecFacing);
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition(pPlayer, pPlayer->WorldSpaceCenter(), vecSrc);

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vecFacing * 700;
	// put it on its side
	QAngle orientation(0, pPlayer->GetLocalAngles().y, -90);
	// roll it
	AngularImpulse rotSpeed(0, 0, 720);
	Fraggrenade_Create(vecSrc, orientation, vecThrow, rotSpeed, pPlayer, GRENADE_TIMER, false);

	WeaponSound(SPECIAL1);

	m_bRedraw = true;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
}



//-----------------------------------------------------------------------------
// Purpose: When the weapon is holstered
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemHolsterFrame(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	// Must be player held
	if (GetOwner() && GetOwner()->IsPlayer() == false)
		return;

	// We can't be active
	if (GetOwner()->GetActiveWeapon() == this)
		return;

	//Actually, we don't even use grenades when the weapon is holstered...
	if (pOwner->m_bHolsteredAW)
	{
		return;
	}

	if (m_bInQGSequence)
	{
		QGSuppressAW();
	}

	CBaseViewModel* AWVM = pOwner->GetViewModel(0);
	CBaseViewModel* qgVM = pOwner->GetViewModel(1);

	//Deploy Weapon
	if (m_bQGWaitDraw && m_flNextQGTime <= gpGlobals->curtime)
	{
		//Don't deploy if holstered manually.
		if (pOwner->m_bHolsteredAW)
		{
			AWVM->m_flPlaybackRate = 1.0f;
			m_bQGWaitDraw = false;
			m_bInQGSequence = false;
			pOwner->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
			qgVM->SetWeaponModel(NULL, NULL);
			return;
		}

		AWVM->m_flPlaybackRate = 1.0f;
		AWVM->RemoveEffects(EF_NODRAW);
		m_bQGWaitDraw = false;
		m_bInQGSequence = false;
		pOwner->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
		pOwner->GetActiveWeapon()->Deploy();
		qgVM->SetWeaponModel(NULL, NULL);
	}

	//QGAttack() is the equivalent of PrimaryAttack here, it's called when the holster animation is finished.
	if (m_bQGWaitHolster && m_flQGHolsterTime <= gpGlobals->curtime)
	{
		AWVM->AddEffects(EF_NODRAW);
		m_bQGWaitHolster = false;
		QGAttack();
	}

	//Check Quick Grenade button
	if ((pOwner->m_nButtons & IN_GRENADE1) && m_flNextQGTime <= gpGlobals->curtime && !m_bQGWaitHolster)
	{
		////Can't be weapon_usp during yeet.
		//if (!Q_stricmp(pOwner->GetActiveWeapon()->GetName(), "weapon_usp") && pOwner->GetActiveWeapon()->GetActivity() == ACT_VM_THROW)
		//{
		//	return;
		//}

		//Must have ammo
		if (!HasPrimaryAmmo())
		{
			return;
		}

		m_bInQGSequence = true;
		pOwner->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;

		//Is active weapon holstered already?
		if (pOwner->m_bHolsteredAW)
		{
			m_flQGHolsterTime = gpGlobals->curtime + 0.1;
			m_bQGWaitHolster = true;
			return;
		}

		//Holster active weapon
		pOwner->GetActiveWeapon()->Holster();
		// Some weapon's don't have holster anims yet, so detect that

		float flSequenceDuration = 0;
		if (pOwner->GetActiveWeapon()->GetActivity() == ACT_VM_HOLSTER)
		{
			flSequenceDuration = pOwner->GetActiveWeapon()->SequenceDuration();
		}
		else {
			//NOTE: I add this because this could be whats causing the code to holster and not do anythings -Telvy
			flSequenceDuration = pOwner->GetActiveWeapon()->SequenceDuration();
		}

		//Speed up holster animation x2
		AWVM->m_flPlaybackRate = 2.0f;
		m_flQGHolsterTime = gpGlobals->curtime + (flSequenceDuration / 2);
		m_bQGWaitHolster = true;
	}

	if (m_bInQGThrow)
	{
		if (!qgVM)
			return;
		//Is drawback finished
		if (m_flQGDrawbackTime <= gpGlobals->curtime)
		{
			if (!(pOwner->m_nButtons & IN_GRENADE1))
			{
				int	idealSequence = qgVM->SelectWeightedSequence(ACT_VM_THROW);
				if (idealSequence >= 0)
				{
					qgVM->SendViewModelMatchingSequence(idealSequence);
					m_flNextQGTime = gpGlobals->curtime + qgVM->SequenceDuration(idealSequence);
					m_bQGWaitDraw = true;
				}
				ThrowGrenade(pOwner);
				DecrementAmmo(pOwner);
				m_bInQGThrow = false;
			}
		}
	}
}

void CWeaponFrag::QGAttack(void)
{
	if (m_flNextQGTime > gpGlobals->curtime)
	{
		return;
	}

	if (!HasPrimaryAmmo())
	{
		return;
	}

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
	{
		return;
	}

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	CBaseViewModel* qgVM = pPlayer->GetViewModel(1);
	if (qgVM)
	{
		pPlayer->ShowViewModel(true);
		qgVM->RemoveEffects(EF_NODRAW);
		qgVM->SetWeaponModel("models/weapons/v_quick_grenade.mdl", NULL);
		int	idealSequence = qgVM->SelectWeightedSequence(ACT_VM_PULLBACK_HIGH);
		if (idealSequence >= 0)
		{
			qgVM->SendViewModelMatchingSequence(idealSequence);
			m_flQGDrawbackTime = gpGlobals->curtime + qgVM->SequenceDuration(idealSequence);
		}
	}

	m_flNextQGTime = FLT_MAX;
	m_bInQGThrow = true;
}

//Suppress active weapon while throwing the grenade
void CWeaponFrag::QGSuppressAW(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (pOwner)
	{
		pOwner->m_nButtons &= ~(IN_ATTACK | IN_ATTACK2 | IN_RELOAD);
	}
}


