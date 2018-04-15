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
	m_rotateTagId = m_pAnimationComponent->GetTagId("Rotate");

	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are triggered
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();

	// Register an action, and the callback that will be sent when it's triggered
	m_pInputComponent->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveLeft, activationMode);  });
	// Bind the 'A' key the "moveleft" action
	m_pInputComponent->BindAction("player", "moveleft", eAID_KeyboardMouse,	EKeyId::eKI_A);

	m_pInputComponent->RegisterAction("player", "moveright", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveRight, activationMode);  });
	m_pInputComponent->BindAction("player", "moveright", eAID_KeyboardMouse, EKeyId::eKI_D);

	m_pInputComponent->RegisterAction("player", "moveforward", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveForward, activationMode);  });
	m_pInputComponent->BindAction("player", "moveforward", eAID_KeyboardMouse, EKeyId::eKI_W);

	m_pInputComponent->RegisterAction("player", "moveback", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveBack, activationMode);  });
	m_pInputComponent->BindAction("player", "moveback", eAID_KeyboardMouse, EKeyId::eKI_S);

	//crouching
	m_pInputComponent->RegisterAction("player", "crouch", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::Crouch, activationMode);  });
	m_pInputComponent->BindAction("player", "crouch", eAID_KeyboardMouse, EKeyId::eKI_C);

	//weapon drawn
	m_pInputComponent->RegisterAction("player", "weapondrawn", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::WeaponDrawn, activationMode);  });
	m_pInputComponent->BindAction("player", "weapondrawn", eAID_KeyboardMouse, EKeyId::eKI_Mouse2);

	m_pInputComponent->RegisterAction("player", "mouse_rotateyaw", [this](int activationMode, float value) { m_mouseDeltaRotation.x -= value; });
	m_pInputComponent->BindAction("player", "mouse_rotateyaw", eAID_KeyboardMouse, EKeyId::eKI_MouseX);

	m_pInputComponent->RegisterAction("player", "mouse_rotatepitch", [this](int activationMode, float value) { m_mouseDeltaRotation.y -= value; });
	m_pInputComponent->BindAction("player", "mouse_rotatepitch", eAID_KeyboardMouse, EKeyId::eKI_MouseY);

	// Register the shoot action
	m_pInputComponent->RegisterAction("player", "camswitch", [this](int activationMode, float value)
	{
		// Only fire on press, not release
		if (activationMode == eIS_Pressed)
		{
			if (m_isFPS)
			{
				CryLog("changing to fps %d", m_isFPS);
				ReviveOnCamChange();
				m_pAnimationComponent->SetCharacterFile("Objects/Characters/mixamo/pants_guy.cdf");
				m_pAnimationComponent->LoadFromDisk();
				m_pAnimationComponent->ResetCharacter();

				m_isFPS = false;
			}
			else
			{
				CryLog("changing to 3rd  person %i", m_isFPS);
				ReviveOnCamChange();
				m_pAnimationComponent->SetCharacterFile("Objects/Characters/mixamo/pants_guy_nohead.cdf");
				m_pAnimationComponent->LoadFromDisk();
				m_pAnimationComponent->ResetCharacter();
				m_isFPS = true;
			}
		}
	});

	// Bind the shoot action to left mouse click
	m_pInputComponent->BindAction("player", "camswitch", eAID_KeyboardMouse, EKeyId::eKI_Tab);

	// Register the shoot action
	m_pInputComponent->RegisterAction("player", "shoot", [this](int activationMode, float value)
	{
		// Only fire on press, not release
		if (activationMode == eIS_Pressed && m_isWeaponDrawn == true)
		{
			if (ICharacterInstance *pCharacter = m_pAnimationComponent->GetCharacter())
			{
				auto *pBarrelOutAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("weapon");

				if (pBarrelOutAttachment != nullptr)
				{
					QuatTS bulletOrigin = pBarrelOutAttachment->GetAttWorldAbsolute();

					SEntitySpawnParams spawnParams;
					spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

					spawnParams.vPosition = bulletOrigin.t;
					spawnParams.qRotation = bulletOrigin.q;

					const float bulletScale = 0.05f;
					spawnParams.vScale = Vec3(bulletScale);

					// Spawn the entity
					if (IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
					{
						// See Bullet.cpp, bullet is propelled in  the rotation and position the entity was spawned with
						pEntity->CreateComponentClass<CBulletComponent>();
					}
				}
			}
		}
	});

	// Bind the shoot action to left mouse click
	m_pInputComponent->BindAction("player", "shoot", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);

	m_State = ePS_Standing;
	m_crouchPress = false;
	m_moving = false;
	m_standPhys = true;
	m_crouchPhys = false;
	m_canStand = true;
	m_isWeaponDrawn = false;

	Revive();
}

uint64 CPlayerComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_UPDATE);
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

		IPersistantDebug *pPD = gEnv->pGameFramework->GetIPersistantDebug();
		if (pPD)
		{
			pPD->Begin("InteractionVector", false);
			pPD->AddText(10.0f, 1.0f, 2.0f, ColorF(Vec3(0, 0, 0), 0.5f), 1.0f, "is crouch button pressed %d", m_crouchPress);
			pPD->AddText(500.0f, 1.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "is moving %d", m_moving);
		}
		switch (m_State)
		{
		case EPlayerState::ePS_Standing:
			pPD->AddText(10.0f, 50.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "standing");
			if (m_moving)
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponMove;
				}
				else
				{
					if (!m_crouchPress)
					{
						if (!m_standPhys)
							Physicalize();
						m_State = EPlayerState::ePS_MovingToStanding;
					}
					else
					{
						if (!m_crouchPhys)
							PhysicalizeCrouch();
						m_State = EPlayerState::ePS_MovingToCrouching;
					}
				}
			}
			else
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponIdle;
				}
				else
				{
					if (!m_crouchPress)
					{
						m_State = EPlayerState::ePS_Standing;
					}
					else
					{
						if (!m_crouchPhys)
							PhysicalizeCrouch();
						m_State = EPlayerState::ePS_Crouching;
					}
				}
			}
			break;
		case EPlayerState::ePS_Crouching:
			pPD->AddText(10.0f, 90.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "crouching");
			if (m_moving)
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponMove;
				}
				else
				{
					if (m_crouchPress)
					{
						if (!m_crouchPhys)
							PhysicalizeCrouch();
						m_State = EPlayerState::ePS_MovingToCrouching;
					}
					else
					{
						if (!m_canStand)
						{
							if (!m_crouchPhys)
								PhysicalizeCrouch();
							m_State = EPlayerState::ePS_MovingToCrouching;
						}
						else
						{
							if (!m_standPhys)
								Physicalize();
							m_State = EPlayerState::ePS_MovingToStanding;
						}
					}
				}
			}
			else
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponIdle;
				}
				else
				{
					if (!m_crouchPress)
					{
						if (m_canStand)
						{
							if (!m_standPhys)
								Physicalize();
							m_State = EPlayerState::ePS_Standing;
						}
						else
						{
							if (!m_crouchPhys)
								PhysicalizeCrouch();
							m_State = EPlayerState::ePS_Crouching;
						}

					}
					else
					{
						m_State = EPlayerState::ePS_Crouching;
					}
				}
			}
			break;

		case EPlayerState::ePS_MovingToStanding:
			pPD->AddText(10.0f, 110.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "standing moving");
			if (m_moving)
			{

				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponMove;
				}
				else
				{
					if (m_crouchPress)
					{
						if (!m_crouchPhys)
							PhysicalizeCrouch();
						m_State = EPlayerState::ePS_MovingToCrouching;
					}
				}
			}
			else
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponIdle;
				}
				else
				{
					if (!m_crouchPress)
						m_State = EPlayerState::ePS_Standing;
					else
						m_State = EPlayerState::ePS_Crouching;
				}
			}
			break;

		case EPlayerState::ePS_MovingToCrouching:
			pPD->AddText(10.0f, 130.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "crouching moving");
			if (m_moving)
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponMove;
				}
				else
				{
					if (!m_crouchPress)
					{
						if (m_canStand)
						{
							if (!m_standPhys)
								Physicalize();
							m_State = EPlayerState::ePS_MovingToStanding;
						}
						else
						{
							if (!m_crouchPhys)
								PhysicalizeCrouch();
							m_State = EPlayerState::ePS_MovingToCrouching;
						}

					}
				}

			}
			else
			{
				if (m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_WeaponIdle;
				}
				else
				{
					if (!m_crouchPress)
					{
						if (m_canStand)
							m_State = EPlayerState::ePS_Standing;
						else
							m_State = EPlayerState::ePS_Crouching;
					}
					else
						m_State = EPlayerState::ePS_Crouching;
				}

			}
			break;
		case  EPlayerState::ePS_WeaponMove:
			if (m_moving)
			{
				if (!m_isWeaponDrawn)
					m_State = EPlayerState::ePS_MovingToStanding;
			}
			else
			{
				if (!m_isWeaponDrawn)
					m_State = EPlayerState::ePS_Standing;
				else
					m_State = EPlayerState::ePS_WeaponIdle;
			}
			break;
		case EPlayerState::ePS_WeaponIdle:
			if (m_moving)
			{
				if (!m_isWeaponDrawn)
				{
					m_State = EPlayerState::ePS_MovingToStanding;
				}
				else
				{
					m_State = EPlayerState::ePS_WeaponMove;
				}
			}
			else
			{
				if (!m_isWeaponDrawn)
					m_State = EPlayerState::ePS_Standing;
			}
		default:
			break;
		}


	}
	break;
	}
}

void CPlayerComponent::UpdateMovementRequest(float frameTime)
{
	// Don't handle input if we are in air
	if (!m_pCharacterController->IsOnGround())
		return;

	Vec3 velocity = ZERO;

	float moveSpeed = gEnv->pConsole->GetCVar("g_WalkingSpeed")->GetFVal();
	float crouchSpeed = 10.0f;
	m_moving = false;

	if (m_inputFlags & (TInputFlags)EInputFlag::Crouch)
	{
		m_crouchPress = true;
		//moveSpeed = 10.0f;
	}
	else
	{
		m_crouchPress = false;
	}

	if (m_inputFlags & (TInputFlags)EInputFlag::WeaponDrawn)
	{
		m_isWeaponDrawn = true;
		if (ICharacterInstance *pCharacter = m_pAnimationComponent->GetCharacter())
		{
			auto *pBarrelOutAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("weapon");
			pBarrelOutAttachment->HideAttachment(0);
		}
	}
	else
	{
		if (ICharacterInstance *pCharacter = m_pAnimationComponent->GetCharacter())
		{
			auto *pBarrelOutAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("weapon");
			pBarrelOutAttachment->HideAttachment(1);
		}
		m_isWeaponDrawn = false;
	}

	if (m_inputFlags & (TInputFlags)EInputFlag::MoveLeft)
	{
		m_moving = true;
		if (m_crouchPress)
		{
			velocity.x -= crouchSpeed * frameTime;
		}
		else
		{
			velocity.x -= moveSpeed * frameTime;
			
		}
	}
	if (m_inputFlags & (TInputFlags)EInputFlag::MoveRight)
	{
		m_moving = true;
		if (m_crouchPress)
		{
			velocity.x += crouchSpeed * frameTime;
		}
		else
		{
			velocity.x += moveSpeed * frameTime;
		}
	}
	if (m_inputFlags & (TInputFlags)EInputFlag::MoveForward)
	{
		m_moving = true;
		if (m_crouchPress)
		{
			velocity.y += crouchSpeed * frameTime;
		}
		else
		{
			velocity.y += moveSpeed * frameTime;
		}
	}
	if (m_inputFlags & (TInputFlags)EInputFlag::MoveBack)
	{
		m_moving = true;
		if (m_crouchPress)
		{
			velocity.y -= crouchSpeed * frameTime;
		}
		else
		{
			velocity.y -= moveSpeed * frameTime;
		}
	}

	m_pCharacterController->AddVelocity(GetEntity()->GetWorldRotation() * velocity);
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