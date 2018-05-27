#pragma once

#include <array>
#include <numeric>

#include <CryEntitySystem/IEntityComponent.h>
#include <CryMath/Cry_Camera.h>

#include <ICryMannequin.h>

#include <DefaultComponents/Cameras/CameraComponent.h>
#include <DefaultComponents/Physics/CharacterControllerComponent.h>
#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>
#include <DefaultComponents/Input/InputComponent.h>

#include "../Attachments/Torch.h"

////////////////////////////////////////////////////////
// Represents a player participating in gameplay
////////////////////////////////////////////////////////
enum EPlayerState
{
	ePS_None,
	ePS_Standing,
	ePS_MovingToStanding,
	ePS_Crouching,
	ePS_MovingToCrouching,
	ePS_Jumping,
	ePS_WeaponMove,
	ePS_WeaponIdle,
	ePS_Interuptable,
	ePS_Last
};


class CPlayerComponent final : public IEntityComponent
{
	enum class EInputFlagType
	{
		Hold = 0,
		Toggle
	};

	typedef uint8 TInputFlags;

	enum class EInputFlag
		: TInputFlags
	{
		MoveLeft = 1 << 0,
		MoveRight = 1 << 1,
		MoveForward = 1 << 2,
		MoveBack = 1 << 3,
		Crouch = 1 << 4,
		WeaponDrawn = 1 << 5
	};

	template<typename T, size_t SAMPLES_COUNT>
	class MovingAverage
	{
		static_assert(SAMPLES_COUNT > 0, "SAMPLES_COUNT shall be larger than zero!");

	public:

		MovingAverage()
			: m_values()
			, m_cursor(SAMPLES_COUNT)
			, m_accumulator()
		{
		}

		MovingAverage& Push(const T& value)
		{
			if (m_cursor == SAMPLES_COUNT)
			{
				m_values.fill(value);
				m_cursor = 0;
				m_accumulator = std::accumulate(m_values.begin(), m_values.end(), T(0));
			}
			else
			{
				m_accumulator -= m_values[m_cursor];
				m_values[m_cursor] = value;
				m_accumulator += m_values[m_cursor];
				m_cursor = (m_cursor + 1) % SAMPLES_COUNT;
			}

			return *this;
		}

		T Get() const
		{
			return m_accumulator / T(SAMPLES_COUNT);
		}

		void Reset()
		{
			m_cursor = SAMPLES_COUNT;
		}

	private:

		std::array<T, SAMPLES_COUNT> m_values;
		size_t m_cursor;

		T m_accumulator;
	};

public:
	CPlayerComponent() = default;
	virtual ~CPlayerComponent() {}

	// IEntityComponent
	virtual void Initialize() override;

	virtual uint64 GetEventMask() const override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~IEntityComponent

	// Reflect type to set a unique identifier for this component
	static void ReflectType(Schematyc::CTypeDesc<CPlayerComponent>& desc)
	{
		desc.SetGUID("{63F4C0C6-32AF-4ACB-8FB0-57D45DD14725}"_cry_guid);
	}

	void Revive();
	void ReviveOnCamChange();

	void Physicalize();
	void PhysicalizeCrouch();

	void PlayThrowSound();
	//raycasting
	void ShootRayFromHead();
	void RayCast(Vec3 origin, Quat dir, IEntity & pSkipEntity);

	void InitializeInput();
	void InitializeUpdate(EPlayerState state, float frameTime);
	void InitializeMovement(float frameTime);
	void InitializeAttachements();

protected:
	void UpdateMovementRequest(float frameTime);
	void UpdateLookDirectionRequest(float frameTime);
	void UpdateAnimation(float frameTime);
	void UpdateCamera(float frameTime);

	void SpawnAtSpawnPoint();

	void CreateWeapon(const char *name);

	void HandleInputFlagChange(TInputFlags flags, int activationMode, EInputFlagType type = EInputFlagType::Hold);

protected:
	Cry::DefaultComponents::CCameraComponent* m_pCameraComponent = nullptr;
	Cry::DefaultComponents::CCharacterControllerComponent* m_pCharacterController = nullptr;
	Cry::DefaultComponents::CAdvancedAnimationComponent* m_pAnimationComponent = nullptr;
	Cry::DefaultComponents::CInputComponent* m_pInputComponent = nullptr;

	FragmentID m_idleFragmentId;
	FragmentID m_walkFragmentId;
	FragmentID m_crouchIdleFragmentId;
	FragmentID m_crouchWalkFragmentId;
	FragmentID m_walkWithWeaponId;
	FragmentID m_idleWithWeaponId;
	FragmentID m_stoneThrowId;
	TagID m_rotateTagId;

	TInputFlags m_inputFlags;
	Vec2 m_mouseDeltaRotation;
	MovingAverage<Vec2, 10> m_mouseDeltaSmoothingFilter;

	FragmentID m_activeFragmentId;
	FragmentID m_desiredFragmentId;

	EPlayerState m_State;
	Quat m_lookOrientation; //!< Should translate to head orientation in the future
	float m_horizontalAngularVelocity;
	MovingAverage<float, 10> m_averagedHorizontalAngularVelocity;

	bool m_isFPS = false;
	bool m_crouchPress;
	bool m_moving;
	bool m_standPhys;
	bool m_crouchPhys;
	bool m_canStand;
	bool m_isWeaponDrawn;
	// Offset the player along the forward axis (normally back)
	// Also offset upwards
	float viewOffsetForward = -1.0f;
	float viewOffsetUp = 2.f;
	const char* pClassName = "";

	bool m_throwAnim;
	CryAudio::ControlId m_gruntThrow;

	CTorchComponent* pTorch;
	IEntity* pTorchEntity;
};
