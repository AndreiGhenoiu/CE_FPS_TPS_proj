#pragma once
#include "StdAfx.h"
#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/CoreAPI.h>

class CSurveillanceCameraComponent final : public IEntityComponent
{
public:
	CSurveillanceCameraComponent() {};
	virtual ~CSurveillanceCameraComponent() {}

	//IEntityComponent

	virtual void Initialize() override;
	virtual uint64 GetEventMask() const override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	void SerializeProperties(Serialization::IArchive& archive);
	//~IEntityComponent

	//Schematyc registration stuff
	//static void Register(Schematyc::CEnvRegistrationScope& componentScope);
	static void ReflectType(Schematyc::CTypeDesc<CSurveillanceCameraComponent>& desc);

	IEntity* Raycast(Vec3 dir, Vec3 pos, float rayLength);
	void Reset();

	bool m_isGameMode;
	bool m_foundPlayer;
	Vec3 hitLocation;

	Schematyc::GeomFileName m_surveillGeomPath = "Objects/player_use/surveilance_camera/surveillance_camera.cgf";
};