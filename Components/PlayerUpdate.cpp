#include "Player.h"

void CPlayerComponent::InitializeUpdate(EPlayerState state, float frameTime)
{
	IPersistantDebug *pPD = gEnv->pGameFramework->GetIPersistantDebug();
	if (pPD)
	{
		pPD->Begin("InteractionVector", false);
		pPD->AddText(10.0f, 1.0f, 2.0f, ColorF(Vec3(0, 0, 0), 0.5f), 1.0f, "is crouch button pressed %d", m_crouchPress);
		pPD->AddText(500.0f, 1.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "is moving %d", m_moving);
	}
	switch (m_State)
	{
	case EPlayerState::ePS_Interuptable:
		pPD->AddText(10.0f, 50.0f, 2.0f, ColorF(Vec3(0, 1, 0), 0.5f), 1.0f, "throwing stone");
		if (!m_throwAnim)
		{
			m_State = EPlayerState::ePS_Standing;
		}

		break;
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