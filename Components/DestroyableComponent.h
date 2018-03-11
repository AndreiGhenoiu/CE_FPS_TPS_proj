#pragma once
#include "StdAfx.h"
#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/CoreAPI.h>

class CDestroyableComponent final : public IEntityComponent
{
public:
	CDestroyableComponent() = default;
	virtual ~CDestroyableComponent() {}

	//IEntityComponent
	virtual void Initialize() override;
	virtual uint64 GetEventMask() const override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	//~IEntityComponent

	//schematyc
	static void ReflectType(Schematyc::CTypeDesc<CDestroyableComponent>& desc);

	void DecrementLife(float bulletMass);
	void DestroyObject(Schematyc::GeomFileName pathToDestroyedObject);
	void ApplyEffects();

	//particle emitting
	IParticleEmitter* SpawnParticleEffect(IParticleEffect* pParticleEffect);
	//sound playing
	void PlaySound();
	void Reset();

	//properties
	float m_life;
	bool m_alive;
	bool m_isGameMode;

	Schematyc::GeomFileName m_intactGeomPath = "Objects/default/primitive_box.cgf";
	Schematyc::GeomFileName m_destroyedGeomPath = "Objects/default/primitive_sphere.cgf";

	EventPhysCollision* pCollision;

};