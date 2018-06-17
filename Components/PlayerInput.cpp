#include "Player.h"
#include <DefaultComponents/Input/InputComponent.h>

void CPlayerComponent::InitializeInput()
{
	// Register an action, and the callback that will be sent when it's triggered
	m_pInputComponent->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange((TInputFlags)EInputFlag::MoveLeft, activationMode);  });
	// Bind the 'A' key the "moveleft" action
	m_pInputComponent->BindAction("player", "moveleft", eAID_KeyboardMouse, EKeyId::eKI_A);

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

	//turn on/off the torch
	m_pInputComponent->RegisterAction("player", "torchOn", [this](int activationMode, float value)
	{
		if (activationMode == eIS_Pressed)
		{
			if (!pTorch->IsEnabled())
			{
				pTorch->Enable(true);
				CryLog("turn on torch");
			}
			else
			{
				pTorch->Enable(false);
				CryLog("turn off torch");
			}
		}
	});
	// Bind the torch turn on/off action to T
	m_pInputComponent->BindAction("player", "torchOn", eAID_KeyboardMouse, EKeyId::eKI_T);

	//flashlight 
	//turn on/off the flashlight
	m_pInputComponent->RegisterAction("player", "flashlightOn", [this](int activationMode, float value)
	{
		if (activationMode == eIS_Pressed)
		{
			if (!pFlashlight->IsEnabled())
			{
				pFlashlight->Enable(true);
				CryLog("turn on flashlight");
			}
			else
			{
				pFlashlight->Enable(false);
				CryLog("turn off flashlight");
			}
		}
	});
	// Bind the torch turn on/off action to T
	m_pInputComponent->BindAction("player", "flashlightOn", eAID_KeyboardMouse, EKeyId::eKI_F);


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

	m_pInputComponent->RegisterAction("player", "StoneThrow", [this](int activationMode, float value)
	{
		if (activationMode == eIS_Pressed)
		{
			m_State = ePS_Interuptable;
			m_throwAnim = true;
		}
	});
	m_pInputComponent->BindAction("player", "StoneThrow", eAID_KeyboardMouse, EKeyId::eKI_G);

}