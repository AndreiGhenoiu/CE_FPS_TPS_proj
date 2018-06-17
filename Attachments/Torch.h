#pragma once

#include "DefaultComponents/Geometry/BaseMeshComponent.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

// Used to indicate the minimum graphical setting for an effect
namespace Torch
{
	enum class ELightGIMode
	{
		Disabled = IRenderNode::EGIMode::eGM_None,
		StaticLight = IRenderNode::EGIMode::eGM_StaticVoxelization,
		DynamicLight = IRenderNode::EGIMode::eGM_DynamicVoxelization,
		ExcludeForGI = IRenderNode::EGIMode::eGM_HideIfGiIsActive,
	};

	enum class EMiniumSystemSpec
	{
		Disabled = 0,
		Always,
		Medium,
		High,
		VeryHigh
	};
}


class CTorchComponent
	: public IEntityComponent
#ifndef RELEASE
	, public IEntityComponentPreviewer
#endif
{
public:
	//	friend CPlugin_CryDefaultEntities;
	//static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void Initialize() final;

	virtual void   ProcessEvent(SEntityEvent& event) final;
	virtual uint64 GetEventMask() const final;

#ifndef RELEASE
	virtual IEntityComponentPreviewer* GetPreviewer() final { return this; }
#endif
	// ~IEntityComponent

#ifndef RELEASE
	// IEntityComponentPreviewer
	virtual void SerializeProperties(Serialization::IArchive& archive) final {}

	virtual void Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const final;
	// ~IEntityComponentPreviewer
#endif

public:
	CTorchComponent() {}
	CTorchComponent(bool hasParticle) { m_hasParticle = hasParticle; }
	virtual ~CTorchComponent() {}


	struct SOptions
	{
		inline bool operator==(const SOptions &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		float m_attenuationBulbSize = CDLight().m_fAttenuationBulbSize;
		bool m_bIgnoreVisAreas = false;
		bool m_bVolumetricFogOnly = false;
		bool m_bAffectsVolumetricFog = true;
		bool m_bAffectsOnlyThisArea = true;
		bool m_bAmbient = false;
		float m_fogRadialLobe = CDLight().m_fFogRadialLobe;

		Torch::ELightGIMode m_giMode = Torch::ELightGIMode::Disabled;
	};

	struct SColor
	{
		inline bool operator==(const SColor &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		ColorF m_color = ColorF(1.f);
		float m_diffuseMultiplier = 1.f;
		float m_specularMultiplier = 1.f;
	};

	struct SShadows
	{
		inline bool operator==(const SShadows &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		Torch::EMiniumSystemSpec m_castShadowSpec = Torch::EMiniumSystemSpec::Always;
	};

	struct SAnimations
	{
		inline bool operator==(const SAnimations &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		uint32 m_style = 0;
		float m_speed = 1.f;
	};

	virtual void Enable(bool bEnable);
	bool IsEnabled() const { return m_bActive; }

	virtual SOptions& GetOptions() { return m_options; }
	const SOptions& GetOptions() const { return m_options; }

	virtual SColor& GetColorParameters() { return m_color; }
	const SColor& GetColorParameters() const { return m_color; }

	virtual SShadows& GetShadowParameters() { return m_shadows; }
	const SShadows& GetShadowParameters() const { return m_shadows; }

	virtual SAnimations& GetAnimationParameters() { return m_animations; }
	const SAnimations& GetAnimationParameters() const { return m_animations; }

	void LoadLight();
	//effects applied
	IParticleEmitter* SpawnParticleEffect(IParticleEffect* pParticleEffect);
	//sounds 

public:
	bool m_bActive = false;
	float m_radius = 10.f;
	bool m_hasParticle = false;

	SOptions m_options;
	SColor m_color;
	SShadows m_shadows;
	SAnimations m_animations;
	int m_slot = 6;
	IParticleEffect* pParticleEffect;
};