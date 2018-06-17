#include "StdAfx.h"
#include "Player.h"

#include "Bullet.h"
#include "SpawnPoint.h"

#include <CryRenderer/IRenderAuxGeom.h>

void CPlayerComponent::Initialize()
{
	
	// Create the camera component, will automatically update the viewport every frame
	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	
	// The character controller is responsible for maintaining player physics
	m_pCharacterController = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	// Offset the default character controller up by one unit
	m_pCharacterController->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, 1.f)));

	// Create the advanced animation component, responsible for updating Mannequin and animating the player
	m_pAnimationComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();
	
	// Set the player geometry, this also triggers physics proxy creation
	m_pAnimationComponent->SetMannequinAnimationDatabaseFile("Animations/Mannequin/ADB/FirstPerson.adb");
	m_pAnimationComponent->SetCharacterFile("Objects/Characters/mixamo/pants_guy.cdf");

	m_pAnimationComponent->SetControllerDefinitionFile("Animations/Mannequin/ADB/FirstPersonControllerDefinition.xml");
	m_pAnimationComponent->SetDefaultScopeContextName("FirstPersonCharacter");
	// Queue the idle fragment to start playing immediately on next update
	m_pAnimationComponent->SetDefaultFragmentName("Idle");

	// Disable movement coming from the animation (root joint offset), we control this entirely via physics
	m_pAnimationComponent->SetAnimationDrivenMotion(false);

	// Load the character and Mannequin data from file
	m_pAnimationComponent->LoadFromDisk();

	// Acquire fragment and tag identifiers to avoid doing so each update
	m_idleFragmentId = m_pAnimationComponent->GetFragmentId("Idle");
	m_walkFragmentId = m_pAnimationComponent->GetFragmentId("Walk");
	m_crouchIdleFragmentId = m_pAnimationComponent->GetFragmentId("CrouchIdle");
	m_crouchWalkFragmentId = m_pAnimationComponent->GetFragmentId("CrouchWalk");
	m_walkWithWeaponId = m_pAnimationComponent->GetFragmentId("WeaponMove");
	m_idleWithWeaponId = m_pAnimationComponent->GetFragmentId("WeaponIdle");
	m_stoneThrowId = m_pAnimationComponent->GetFragmentId("StoneThrow");
	m_rotateTagId = m_pAnimationComponent->GetTagId("Rotate");

	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are triggered
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();

	InitializeInput();
	InitializeAttachements();

	m_State = ePS_Standing;
	m_crouchPress = false;
	m_moving = false;
	m_standPhys = true;
	m_crouchPhys = false;
	m_canStand = true;
	m_isWeaponDrawn = false;
	m_gruntThrow = CryAudio::StringToId("grunt_throw");

	if (ICharacterInstance *pCharacter = m_pAnimationComponent->GetCharacter())
	{
		auto *pBarrelOutAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("weapon");
		pBarrelOutAttachment->HideAttachment(1);
		auto *pStoneAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("stone");
		pStoneAttachment->HideAttachment(1);
	}

	Revive();
}

uint64 CPlayerComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_UPDATE) | BIT64(ENTITY_EVENT_ANIM_EVENT);
}

void CPlayerComponent::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_START_GAME:
	{
		// Revive the entity when gameplay starts
		Revive();
	}
	break;
	
	case ENTITY_EVENT_ANIM_EVENT:
	{	
		const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);

		if (pAnimEvent)
		{
			if (pAnimEvent->m_EventName && stricmp(pAnimEvent->m_EventName, "throw") == 0)
			{
				PlayThrowSound();
				CryLog("throw event");
				if (ICharacterInstance *pCharacter = m_pAnimationComponent->GetCharacter())
				{
					auto *pStoneAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("stone");
					pStoneAttachment->HideAttachment(1);

					if (pStoneAttachment != nullptr)
					{
						Vec3 dir = GetEntity()->GetWorldRotation().GetColumn1();
						CryLog("bullet direction %f, %f, %f", dir.x, dir.y, dir.z);

						Vec3 stoneOrigin = pStoneAttachment->GetAttWorldAbsolute().GetColumn3();
						CryLog("bulletorigin %f, %f, %f", stoneOrigin.x, stoneOrigin.y, stoneOrigin.z);

						SEntitySpawnParams spawnParams;
						spawnParams.vPosition = stoneOrigin;
						const float bulletScale = 0.1f;
						spawnParams.vScale = Vec3(bulletScale);

						// Spawn the entity
						if (IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
						{
							// See Bullet.cpp, bullet is propelled in  the rotation and position the entity was spawned with
							pEntity->CreateComponentClass<CBulletComponent>();
							// Apply an impulse so that the bullet flies forward
							if (auto *pPhysics = pEntity->GetPhysics())
							{
								pe_action_impulse impulseAction;

								const float initialVelocity = 50.f;
								// Set the actual impulse, in this cause the value of the initial velocity CVar in bullet's forward direction
								impulseAction.impulse = dir * initialVelocity;

								// Send to the physical entity
								pPhysics->Action(&impulseAction);
							}
						}
					}

				}
				
			}
			if (pAnimEvent->m_EventName && stricmp(pAnimEvent->m_EventName, "throwEnd") == 0)
			{
				m_throwAnim = false;
				CryLog("throw event ended");
			}
		}
	}
		break;
	case ENTITY_EVENT_UPDATE:
	{
		SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];

		// Start by updating the movement request we want to send to the character controller
		// This results in the physical representation of the character moving
		UpdateMovementRequest(pCtx->fFrameTime);

		// Process mouse input to update look orientation.
		UpdateLookDirectionRequest(pCtx->fFrameTime);

		// Update the animation state of the character
		UpdateAnimation(pCtx->fFrameTime);

		// Update the camera component offset
		UpdateCamera(pCtx->fFrameTime);

		ShootRayFromHead();

		InitializeUpdate(m_State, pCtx->fFrameTime);


	}
	break;
	}
}

void CPlayerComponent::InitializeAttachements()
{
	if (ICharacterInstance *pCharacter = m_pAnimationComponent->GetCharacter())
	{

		auto *pTorchtAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("torch");
		if (pTorchtAttachment != nullptr)
		{
			SEntitySpawnParams spawnParams;
			spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

			// Spawn the torch
			if (pTorchEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
			{
				pTorch = pTorchEntity->CreateComponentClass<CTorchComponent>();

				EntityId torchId = pTorchEntity->GetId();
				CEntityAttachment* pTorchEntAttachment = new CEntityAttachment();

				pTorchEntAttachment->SetEntityId(torchId);
				pTorchtAttachment->AddBinding(pTorchEntAttachment);

				pTorch->m_color.m_color = ColorF(1.0f, 0.75f, 0.25f);
				pTorch->m_color.m_diffuseMultiplier = 0.2f;
				pTorch->m_animations.m_style = 34;
				pTorch->m_animations.m_speed = 0.2;
				pTorch->m_radius = 6.0f;
			}
		}

		auto *pFlashlightAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("flashlight");
		if (pFlashlightAttachment != nullptr)
		{
			SEntitySpawnParams spawnParams;
			spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

			//Spawn the flashlight
			if (pFlashlightEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
			{
				pFlashlight = pFlashlightEntity->CreateComponentClass<CFlashlightComponent>();

				EntityId flashlightId = pFlashlightEntity->GetId();
				CEntityAttachment* pFlashlightEntAttachment = new CEntityAttachment();

				pFlashlightEntAttachment->SetEntityId(flashlightId);
				pFlashlightAttachment->AddBinding(pFlashlightEntAttachment);

				pFlashlight->m_bActive = false;
				pFlashlight->m_color.m_diffuseMultiplier = 3.f;
				pFlashlight->m_options.m_attenuationBulbSize = 1.f;
			}

		}

	}
}


void CPlayerComponent::UpdateMovementRequest(float frameTime)
{
	InitializeMovement(frameTime);
}

void CPlayerComponent::UpdateLookDirectionRequest(float frameTime)
{
	const float rotationSpeed = 0.002f;
	const float rotationLimitsMinPitch = -0.84f;
	const float rotationLimitsMaxPitch = 1.5f;

	// Apply smoothing filter to the mouse input
	m_mouseDeltaRotation = m_mouseDeltaSmoothingFilter.Push(m_mouseDeltaRotation).Get();

	// Update angular velocity metrics
	m_horizontalAngularVelocity = (m_mouseDeltaRotation.x * rotationSpeed) / frameTime;
	m_averagedHorizontalAngularVelocity.Push(m_horizontalAngularVelocity);

	// Start with updating look orientation from the latest input
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	// Yaw
	ypr.x += m_mouseDeltaRotation.x * rotationSpeed;

	// Pitch
	// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
	ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);

	// Roll (skip)
	ypr.z = 0;

	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	if (pFlashlight->IsEnabled())
	{
		Vec3 vDir = m_lookOrientation.GetColumn1();

		Quat currentFlashlightOrientation = Quat(pFlashlight->GetLocalTM());

		Ang3 ypr_current = CCamera::CreateAnglesYPR(Matrix33(currentFlashlightOrientation));

		Quat targetFlashlightOrientation = Quat::CreateRotationVDir(vDir);

		Ang3 ypr_target = CCamera::CreateAnglesYPR(Matrix33(targetFlashlightOrientation));

		ypr_target.x = ypr_current.x;
		ypr_target.z = ypr_current.z;
		ypr_target.y *= -1.f;

		targetFlashlightOrientation = Quat(CCamera::CreateOrientationYPR(ypr_target));

		pFlashlight->SetLocalOrientation(targetFlashlightOrientation);

	}

	// Reset the mouse delta accumulator every frame
	m_mouseDeltaRotation = ZERO;
}

void CPlayerComponent::UpdateAnimation(float frameTime)
{
	const float angularVelocityTurningThreshold = 0.174; // [rad/s]

	// Update tags and motion parameters used for turning
	const bool isTurning = std::abs(m_averagedHorizontalAngularVelocity.Get()) > angularVelocityTurningThreshold;
	//m_pAnimationComponent->SetTagWithId(m_rotateTagId, isTurning);
	if (isTurning)
	{
		// TODO: This is a very rough predictive estimation of eMotionParamID_TurnAngle that could easily be replaced with accurate reactive motion 
		// if we introduced IK look/aim setup to the character's model and decoupled entity's orientation from the look direction derived from mouse input.

		const float turnDuration = 1.0f; // Expect the turning motion to take approximately one second.
		m_pAnimationComponent->SetMotionParameter(eMotionParamID_TurnAngle, m_horizontalAngularVelocity * turnDuration);
	}

	// Update active fragment
	//const auto& desiredFragmentId = m_pCharacterController->IsWalking() ? m_walkFragmentId : m_idleFragmentId;
	switch (m_State) {
		case ePS_MovingToCrouching:
			m_desiredFragmentId = m_crouchWalkFragmentId;
			break;
		case ePS_MovingToStanding:
			m_desiredFragmentId = m_walkFragmentId;
			break;
		case ePS_Standing:
			m_desiredFragmentId = m_idleFragmentId;
			break;
		case ePS_Crouching:
			m_desiredFragmentId = m_crouchIdleFragmentId;
			break;
		case ePS_WeaponMove:
			m_desiredFragmentId = m_walkWithWeaponId;
			break;
		case ePS_WeaponIdle:
			m_desiredFragmentId = m_idleWithWeaponId;
			break;
		case ePS_Interuptable:
			m_desiredFragmentId = m_stoneThrowId;
			break;
		default:
			break;
	}


	if (m_activeFragmentId != m_desiredFragmentId)
	{
		m_activeFragmentId = m_desiredFragmentId;
		m_pAnimationComponent->QueueFragmentWithId(m_activeFragmentId);
	}

	// Update entity rotation as the player turns
	// We only want to affect Z-axis rotation, zero pitch and roll
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));
	ypr.y = 0;
	ypr.z = 0;
	const Quat correctedOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	// Send updated transform to the entity, only orientation changes
	GetEntity()->SetPosRotScale(GetEntity()->GetWorldPos(), correctedOrientation, Vec3(1, 1, 1));
}

void CPlayerComponent::UpdateCamera(float frameTime)
{
	IEntity &playerEntity = *GetEntity();

	IPersistantDebug *pPD = gEnv->pGameFramework->GetIPersistantDebug();
	if (pPD)
	{
		pPD->Begin("cameraVector", false);
		pPD->AddText(500.0f, 1.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "is moving %d", m_moving);
	}

	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	// Ignore z-axis rotation, that's set by CPlayerAnimations
	ypr.x = 0;

	// Start with changing view rotation to the requested mouse look orientation
	Matrix34 localTransform = IDENTITY;
	localTransform.SetRotation33(CCamera::CreateOrientationYPR(ypr));

	Vec3 targetWorldPos = playerEntity.GetWorldPos();
	Vec3 destination;
	Vec3 origin;

	if (m_isFPS)
	{
		viewOffsetForward = 0.1f;
		if (m_State == ePS_Crouching || m_State == ePS_MovingToCrouching)
			viewOffsetUp = 1.5f;
	}
	else
	{

		if (m_State == ePS_Crouching || m_State == ePS_MovingToCrouching)
			viewOffsetUp = 1.5f;
		else
			viewOffsetUp = 2.f;

		destination = targetWorldPos;
		origin = targetWorldPos;

		origin += playerEntity.GetWorldRotation().GetColumn1() * -0.2f;
		origin += playerEntity.GetWorldRotation().GetColumn2() * 2.0f;

		destination += playerEntity.GetWorldRotation().GetColumn1() * -0.9f;
		destination += playerEntity.GetWorldRotation().GetColumn2() * 2.0f;

		static IPhysicalEntity* pSkipEntities[10];
		pSkipEntities[0] = playerEntity.GetPhysics();

		Vec3 dir = destination - origin;

		ray_hit hit;
		int hits = gEnv->pPhysicalWorld->RayWorldIntersection(origin, dir, ent_static | ent_sleeping_rigid | ent_rigid | ent_independent | ent_terrain,
			rwi_stop_at_pierceable | rwi_colltype_any, &hit, 1, pSkipEntities, 2);

		if (hit.pCollider)
		{
			destination = hit.pt;
			viewOffsetForward = -1.0f * crymath::abs(targetWorldPos.y - destination.y);
			if (viewOffsetForward > -0.3f)
				viewOffsetForward = -0.3f;
			pPD->AddText(10.0f, 190.0f, 2.0f, ColorF(Vec3(0, 0, 0), 0.5f), 0.10f, "hit at %f, %f, %f", destination.x, destination.y, destination.z);

		}
		else
			viewOffsetForward = -1.0f;

		//pPD->AddSphere(origin, 0.1f, ColorF(Vec3(1, 0, 0), 0.5f), 1.0f);
		//pPD->AddSphere(destination, 0.1f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f);
	}

	RayCast(destination, m_lookOrientation, playerEntity);

	localTransform.SetTranslation(Vec3(0, viewOffsetForward, viewOffsetUp));

	m_pCameraComponent->SetTransformMatrix(localTransform);
}



void CPlayerComponent::Revive()
{
	// Find a spawn point and move the entity there
	SpawnAtSpawnPoint();

	// Unhide the entity in case hidden by the Editor
	GetEntity()->Hide(false);

	// Make sure that the player spawns upright
	GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), IDENTITY, GetEntity()->GetWorldPos()));

	// Apply character to the entity
	m_pAnimationComponent->ResetCharacter();
	Physicalize();
	//m_pCharacterController->Physicalize();

	// Reset input now that the player respawned
	m_inputFlags = 0;
	m_mouseDeltaRotation = ZERO;
	m_mouseDeltaSmoothingFilter.Reset();

	m_activeFragmentId = FRAGMENT_ID_INVALID;

	m_lookOrientation = IDENTITY;
	m_horizontalAngularVelocity = 0.0f;
	m_averagedHorizontalAngularVelocity.Reset();
}

void CPlayerComponent::ReviveOnCamChange()
{
	// Find a spawn point and move the entity there
	//SpawnAtSpawnPoint();

	// Unhide the entity in case hidden by the Editor
	//GetEntity()->Hide(false);

	// Make sure that the player spawns upright
	//GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1, 1, 1), IDENTITY, GetEntity()->GetWorldPos()));

	// Apply character to the entity
	m_pAnimationComponent->ResetCharacter();
	m_pCharacterController->Physicalize();

	// Reset input now that the player respawned
	m_inputFlags = 0;
	m_mouseDeltaRotation = ZERO;
	m_mouseDeltaSmoothingFilter.Reset();

	m_activeFragmentId = FRAGMENT_ID_INVALID;

	//m_lookOrientation = IDENTITY;
	//m_horizontalAngularVelocity = 0.0f;
	//m_averagedHorizontalAngularVelocity.Reset();
}

void CPlayerComponent::Physicalize()
{
	m_standPhys = true;
	m_crouchPhys = false;
	// Physicalize the player as type Living.
	// This physical entity type is specifically implemented for players
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_LIVING;

	physParams.mass = 100.0f;
	CryLog("physicalize stand");
	pe_player_dimensions playerDimensions;

	// Prefer usage of a cylinder instead of capsule
	//if(!m_isCrouching)
	playerDimensions.bUseCapsule = 1;
	//else
	//	playerDimensions.bUseCapsule = 0;

	// Specify the size of our cylinder
	playerDimensions.sizeCollider = Vec3(0.35f, 0.35f, 0.5f);

	// Keep pivot at the player's feet (defined in player geometry) 
	playerDimensions.heightPivot = 0.f;
	// Offset collider upwards
	playerDimensions.heightCollider = 1.f;
	playerDimensions.groundContactEps = 0.004f;

	physParams.pPlayerDimensions = &playerDimensions;

	pe_player_dynamics playerDynamics;
	playerDynamics.kAirControl = 0.f;
	playerDynamics.mass = physParams.mass;
	physParams.pPlayerDynamics = &playerDynamics;

	GetEntity()->Physicalize(physParams);
}

void CPlayerComponent::PhysicalizeCrouch()
{
	m_standPhys = false;
	m_crouchPhys = true;

	CryLog("physicalize crouch");
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_LIVING;

	physParams.mass = 100.0f;

	pe_player_dimensions playerDimensions;

	// Prefer usage of a cylinder instead of capsule
	//if(!m_isCrouching)
	playerDimensions.bUseCapsule = 1;
	//else
	//	playerDimensions.bUseCapsule = 0;

	// Specify the size of our cylinder
	playerDimensions.sizeCollider = Vec3(0.35f, 0.35f, 0.2f);

	// Keep pivot at the player's feet (defined in player geometry) 
	playerDimensions.heightPivot = 0.f;
	// Offset collider upwards
	playerDimensions.heightCollider = 0.8f;
	playerDimensions.groundContactEps = 0.004f;

	physParams.pPlayerDimensions = &playerDimensions;

	pe_player_dynamics playerDynamics;
	playerDynamics.kAirControl = 0.f;
	playerDynamics.mass = physParams.mass;
	physParams.pPlayerDynamics = &playerDynamics;

	GetEntity()->Physicalize(physParams);
}

void CPlayerComponent::PlayThrowSound()
{
	CryLog("grunt throw sound play");

	CryAudio::SExecuteTriggerData const data("throw", CryAudio::EOcclusionType::Ignore, GetEntity()->GetWorldPos(), true, m_gruntThrow);
	gEnv->pAudioSystem->ExecuteTriggerEx(data);
}

void CPlayerComponent::ShootRayFromHead()
{
	IEntity &playerEntity = *GetEntity();
	Vec3 pos = ZERO;
	if (m_crouchPress)
		pos = GetEntity()->GetPos() + Vec3(0.0f, 0.10f, 1.5f);
	else
		pos = GetEntity()->GetPos() + Vec3(0.0f, 0.10f, 2.0f);

	ray_hit hit;
	Vec3 dir = Vec3(0.0f, 0.0f, 1.0f);// GetEntity()->GetForwardDir() * distance;
	int objTypes = ent_all;//ent_static | ent_rigid | ent_terrain; //change to ent_all if no detection
	//const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;


	static IPhysicalEntity* pSkipEntities[10];
	pSkipEntities[0] = playerEntity.GetPhysics();

	gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir, ent_static | ent_sleeping_rigid | ent_rigid | ent_independent | ent_terrain,
		rwi_stop_at_pierceable | rwi_colltype_any, &hit, 1, pSkipEntities, 2);
	//CryLog("distance to hit %f", hit.dist);
	if (hit.dist < 1.0f && hit.dist > 0.0f)
	{
		m_canStand = false;
	}
	else
	{
		m_canStand = true;
	}
	IPersistantDebug *pPD = gEnv->pGameFramework->GetIPersistantDebug();
	if (pPD)
	{
		pPD->Begin("InteractionVector", false);
		pPD->AddSphere(hit.pt, 0.1f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f);

		//aux geometry
		if (auto pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom())
			//pAuxGeom->DrawSphere(Pos, radius, color);

		if (m_canStand)
			pPD->AddText(10.0f, 20.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "can stand");
		else
			pPD->AddText(10.0f, 20.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "can't stand");

	}
}

void CPlayerComponent::RayCast(Vec3 origin, Quat dir, IEntity & pSkipEntity)
{
	ray_hit hit;
	float m_InterationDistance = 10.0f;
	Vec3 finalDir = dir* Vec3(0.0f, m_InterationDistance, 0.0f);
	int objTypes = ent_all;
	const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;
	static IPhysicalEntity* pSkipEntities[10];
	pSkipEntities[0] = pSkipEntity.GetPhysics();

	gEnv->pPhysicalWorld->RayWorldIntersection(origin, finalDir, objTypes, flags, &hit, 1, pSkipEntities, 2);

	IPersistantDebug *pPD = gEnv->pGameFramework->GetIPersistantDebug();

	if (hit.pCollider)
	{
		if (pPD)
		{
			pPD->Begin("Raycast", false);
			pPD->AddSphere(hit.pt, 0.25f, ColorF(Vec3(1, 1, 0), 0.5f), 1.0f);
		}
		IPhysicalEntity *pHitEntity = hit.pCollider;
		IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pHitEntity);
		if (pEntity)
		{
			IEntityClass* pClass = pEntity->GetClass();
			pClassName = pClass->GetName();
			if (pPD)
				pPD->AddText3D(hit.pt, 3.0f, ColorF(Vec3(1, 1, 0), 0.5f), 1.0f, pClassName);
		}
	}

}



void CPlayerComponent::SpawnAtSpawnPoint()
{
	// We only handle default spawning below for the Launcher
	// Editor has special logic in CEditorGame
	if (gEnv->IsEditor())
		return;

	// Spawn at first default spawner
	auto *pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();
	pEntityIterator->MoveFirst();

	while (!pEntityIterator->IsEnd())
	{
		IEntity *pEntity = pEntityIterator->Next();

		if (auto* pSpawner = pEntity->GetComponent<CSpawnPointComponent>())
		{
			pSpawner->SpawnEntity(m_pEntity);
			break;
		}
	}
}

void CPlayerComponent::HandleInputFlagChange(TInputFlags flags, int activationMode, EInputFlagType type)
{
	switch (type)
	{
	case EInputFlagType::Hold:
	{
		if (activationMode == eIS_Released)
		{
			m_inputFlags &= ~flags;
		}
		else
		{
			m_inputFlags |= flags;
		}
	}
	break;
	case EInputFlagType::Toggle:
	{
		if (activationMode == eIS_Released)
		{
			// Toggle the bit(s)
			m_inputFlags ^= flags;
		}
	}
	break;
	}
}