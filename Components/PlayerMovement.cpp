#include "Player.h"

void CPlayerComponent::InitializeMovement(float frameTime)
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

	if (m_inputFlags & (TInputFlags)EInputFlag::MoveLeft && m_State != EPlayerState::ePS_Interuptable)
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
	if (m_inputFlags & (TInputFlags)EInputFlag::MoveRight && m_State != EPlayerState::ePS_Interuptable)
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
	if (m_inputFlags & (TInputFlags)EInputFlag::MoveForward && m_State != EPlayerState::ePS_Interuptable)
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
	if (m_inputFlags & (TInputFlags)EInputFlag::MoveBack && m_State != EPlayerState::ePS_Interuptable)
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


