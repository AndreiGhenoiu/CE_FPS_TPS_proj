#pragma once
#include "StdAfx.h"
#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/CoreAPI.h>

#include "SurveillanceCamera.h"
#include "../Attachments/Flashlight.h"

enum CamRotation
{
	Rotate_left,
	Rotate_right
};

enum CamSearch
{
	Searching,
	Found,
	Tracking,
	Resetting
};

class CSurveillaceComponent final : public IEntityComponent
{
public:
	CSurveillaceComponent() = default;
	virtual ~CSurveillaceComponent() {}

	//IEntityComponent

	virtual void Initialize() override;
	virtual uint64 GetEventMask() const override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	void SerializeProperties(Serialization::IArchive& archive);
	//~IEntityComponent


	//Schematyc registration stuff
	//static void Register(Schematyc::CEnvRegistrationScope& componentScope);
	static void ReflectType(Schematyc::CTypeDesc<CSurveillaceComponent>& desc);

	//~Schematyc registration stuff
	void Reset();

	bool m_isGameMode;

	Schematyc::GeomFileName m_intactGeomPath = "Objects/player_use/surveilance_camera/surveillance_camera_mount.cgf";

	int rotator;
	float timer;

	CamRotation m_camRot;
	CamSearch m_camSearch;

	CSurveillanceCameraComponent* m_pSurveillanceCameraComponent;
	CFlashlightComponent* m_pFlashlightComponent;
	IEntity* m_HitEntity;
};
