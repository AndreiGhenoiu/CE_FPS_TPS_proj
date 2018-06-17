#include "StdAfx.h"
#include "Flashlight.h"

#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <Cry3DEngine/IRenderNode.h>

#include <array>

void CFlashlightComponent::Initialize()
{
	if (!m_bActive)
	{
		FreeEntitySlot();

		return;
	}
	LoadFlashlight();
}

void CFlashlightComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		Initialize();
	}
}

uint64 CFlashlightComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
}

void CFlashlightComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
{
	if (context.bSelected)
	{
		Matrix34 slotTransform = GetWorldTransformMatrix();

		float distance = m_radius;
		float size = distance * tan(m_angle.ToRadians());

		std::array<Vec3, 4> points =
		{ {
				Vec3(size, distance, size),
				Vec3(-size, distance, size),
				Vec3(-size, distance, -size),
				Vec3(size, distance, -size)
			} };

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[0]), context.debugDrawInfo.color);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[1]), context.debugDrawInfo.color);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[2]), context.debugDrawInfo.color);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[3]), context.debugDrawInfo.color);

		Vec3 p1 = slotTransform.TransformPoint(points[0]);
		Vec3 p2;
		for (int i = 0; i < points.size(); i++)
		{
			int j = (i + 1) % points.size();
			p2 = slotTransform.TransformPoint(points[j]);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p1, context.debugDrawInfo.color, p2, context.debugDrawInfo.color);
			p1 = p2;
		}
	}
}

void CFlashlightComponent::Enable(bool bEnable)
{
	m_bActive = bEnable;

	Initialize();
}

void CFlashlightComponent::LoadFlashlight()
{

	CDLight flashlight;

	flashlight.m_nLightStyle = m_animations.m_style;
	flashlight.SetAnimSpeed(m_animations.m_speed);

	flashlight.SetPosition(ZERO);
	flashlight.m_Flags = DLF_DEFERRED_LIGHT | DLF_PROJECT;

	flashlight.m_fRadius = m_radius;

	flashlight.m_fLightFrustumAngle = m_angle.ToDegrees();
	flashlight.m_fProjectorNearPlane = m_projectorOptions.m_nearPlane;

	flashlight.SetLightColor(m_color.m_color * m_color.m_diffuseMultiplier);
	flashlight.SetSpecularMult(m_color.m_specularMultiplier);

	flashlight.m_fHDRDynamic = 0.f;

	if (m_options.m_bAffectsOnlyThisArea)
		flashlight.m_Flags |= DLF_THIS_AREA_ONLY;

	if (m_options.m_bIgnoreVisAreas)
		flashlight.m_Flags |= DLF_IGNORES_VISAREAS;

	if (m_options.m_bVolumetricFogOnly)
		flashlight.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;

	if (m_options.m_bAffectsVolumetricFog)
		flashlight.m_Flags |= DLF_VOLUMETRIC_FOG;

	if (m_options.m_bAmbient)
		flashlight.m_Flags |= DLF_AMBIENT;

	//TODO: Automatically add DLF_FAKE when using beams or flares

	if (m_shadows.m_castShadowSpec != Flashlight::EMiniumSystemSpec::Disabled && (int)gEnv->pSystem->GetConfigSpec() >= (int)m_shadows.m_castShadowSpec)
	{
		flashlight.m_Flags |= DLF_CASTSHADOW_MAPS;

		flashlight.SetShadowBiasParams(1.f, 1.f);
		flashlight.m_fShadowUpdateMinRadius = flashlight.m_fRadius;

		float shadowUpdateRatio = 1.f;
		flashlight.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));
	}
	else
		flashlight.m_Flags &= ~DLF_CASTSHADOW_MAPS;

	flashlight.m_fAttenuationBulbSize = m_options.m_attenuationBulbSize;

	flashlight.m_fFogRadialLobe = m_options.m_fogRadialLobe;

	const char* szProjectorTexturePath = m_projectorOptions.GetTexturePath();
	if (szProjectorTexturePath[0] == '\0')
	{
		szProjectorTexturePath = "%ENGINE%/EngineAssets/Textures/lights/flashlight_projector.dds";
	}

	const char* pExt = PathUtil::GetExt(szProjectorTexturePath);
	if (!stricmp(pExt, "swf") || !stricmp(pExt, "gfx") || !stricmp(pExt, "usm") || !stricmp(pExt, "ui"))
	{
		flashlight.m_pLightDynTexSource = gEnv->pRenderer->EF_LoadDynTexture(szProjectorTexturePath, false);
	}
	else
	{
		flashlight.m_pLightImage = gEnv->pRenderer->EF_LoadTexture(szProjectorTexturePath, FT_DONT_STREAM);
	}

	if ((flashlight.m_pLightImage == nullptr || !flashlight.m_pLightImage->IsTextureLoaded()) && flashlight.m_pLightDynTexSource == nullptr)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "flashlight projector texture %s not found, disabling projector component for entity %s", szProjectorTexturePath, m_pEntity->GetName());
		FreeEntitySlot();
		return;
	}

	// Load the flashlight source into the entity
	m_pEntity->LoadLight(GetOrMakeEntitySlotId(), &flashlight);

	if (m_projectorOptions.HasMaterialPath())
	{
		// Allow setting a specific material for the flashlight in this slot, for example to set up beams
		if (IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_projectorOptions.GetMaterialPath(), false))
		{
			m_pEntity->SetSlotMaterial(GetEntitySlotId(), pMaterial);
		}
	}
	m_projectorOptions.SetMaterialPath(m_projectorOptions.m_materialPath);
	// Fix flashlight orientation to point along the forward axis
	// This has to be done since lights in the engine currently emit from the right axis for some reason.
	//m_pEntity->SetSlotLocalTM(GetEntitySlotId(), Matrix34::Create(Vec3(1.f), Quat::CreateRotationY(gf_PI * -0.5f), ZERO));


	//m_pEntity->SetRotation(m_pPlayer->m_lookOrientation);

	uint32 slotFlags = m_pEntity->GetSlotFlags(GetEntitySlotId());
	//UpdateGIModeEntitySlotFlags((uint8)m_options.m_giMode, slotFlags);
	m_pEntity->SetSlotFlags(GetEntitySlotId(), slotFlags);
}

void CFlashlightComponent::SProjectorOptions::SetTexturePath(const char * szPath)
{
	m_texturePath = szPath;
}
void CFlashlightComponent::SProjectorOptions::SetMaterialPath(const char * szPath)
{
	m_materialPath = szPath;
}

Matrix34 CFlashlightComponent::GetLocalTM()
{
	return m_pEntity->GetSlotLocalTM(GetEntitySlotId(), false);
}

void CFlashlightComponent::SetLocalOrientation(Quat orientation)
{
	m_pEntity->SetSlotLocalTM(GetEntitySlotId(), Matrix34(orientation));
}
