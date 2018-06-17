#pragma once
#include "Torch.h"

#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CryMath/Angle.h>

namespace Flashlight
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

class CFlashlightComponent
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
	CFlashlightComponent() {}
	virtual ~CFlashlightComponent() {}


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

		Flashlight::ELightGIMode m_giMode = Flashlight::ELightGIMode::Disabled;
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

		Flashlight::EMiniumSystemSpec m_castShadowSpec = Flashlight::EMiniumSystemSpec::Always;
	};

	struct SAnimations
	{
		inline bool operator==(const SAnimations &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		uint32 m_style = 0;
		float m_speed = 1.f;
	};

	struct SProjectorOptions
	{
		inline bool operator==(const SProjectorOptions &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

		void SetTexturePath(const char* szPath);
		const char* GetTexturePath() const { return m_texturePath.c_str(); }
		void SetMaterialPath(const char* szPath);
		const char* GetMaterialPath() const { return m_materialPath.c_str(); }
		bool HasMaterialPath() const { return m_materialPath.size() > 0; }

		float m_nearPlane = 0.f;

		string m_texturePath;
		string m_materialPath = "Assets/materials/softedge.mtl";
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
	
	void LoadFlashlight();

	Matrix34 GetLocalTM();
	void SetLocalOrientation(Quat orientation);

public:
	bool m_bActive = false;
	float m_radius = 10.f;
	CryTransform::CClampedAngle<0, 180> m_angle = 30.0_degrees;

	SProjectorOptions m_projectorOptions;
	SOptions m_options;
	SColor m_color;
	SShadows m_shadows;
	SAnimations m_animations;



};