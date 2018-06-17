#include "StdAfx.h"
#include "Torch.h"

#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <Cry3DEngine/IRenderNode.h>

void CTorchComponent::Initialize()
{
	if (!m_bActive)
	{
		//m_pEntity->FreeSlot(1);
		FreeEntitySlot();


		return;
	}
	//smoke_and_fire.black_smoke.black_smoke
	//Libs/particles/torch.pfx

	LoadLight();
}

void CTorchComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		Initialize();
	}
}

uint64 CTorchComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
}

void CTorchComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
{
	if (context.bSelected)
	{
		Matrix34 slotTransform = GetWorldTransformMatrix();

		Vec3 p0, p1;
		float step = 10.0f / 180 * gf_PI;
		float angle;

		Vec3 pos = slotTransform.GetTranslation();

		// Z Axis
		p0 = pos;
		p1 = pos;
		p0.x += m_radius * sin(0.0f);
		p0.y += m_radius * cos(0.0f);
		for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
		{
			p1.x = pos.x + m_radius * sin(angle);
			p1.y = pos.y + m_radius * cos(angle);
			p1.z = pos.z;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p0, context.debugDrawInfo.color, p1, context.debugDrawInfo.color);
			p0 = p1;
		}

		// X Axis
		p0 = pos;
		p1 = pos;
		p0.y += m_radius * sin(0.0f);
		p0.z += m_radius * cos(0.0f);
		for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
		{
			p1.x = pos.x;
			p1.y = pos.y + m_radius * sin(angle);
			p1.z = pos.z + m_radius * cos(angle);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p0, context.debugDrawInfo.color, p1, context.debugDrawInfo.color);
			p0 = p1;
		}

		// Y Axis
		p0 = pos;
		p1 = pos;
		p0.x += m_radius * sin(0.0f);
		p0.z += m_radius * cos(0.0f);
		for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
		{
			p1.x = pos.x + m_radius * sin(angle);
			p1.y = pos.y;
			p1.z = pos.z + m_radius * cos(angle);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p0, context.debugDrawInfo.color, p1, context.debugDrawInfo.color);
			p0 = p1;
		}
	}
}


void CTorchComponent::Enable(bool bEnable)
{
	m_bActive = bEnable;

	Initialize();
}

void CTorchComponent::LoadLight()
{

	CDLight light;

	light.m_nLightStyle = m_animations.m_style;
	light.SetAnimSpeed(m_animations.m_speed);

	light.SetPosition(ZERO);
	light.m_Flags = DLF_DEFERRED_LIGHT | DLF_POINT;

	light.m_fRadius = m_radius;

	light.SetLightColor(m_color.m_color * m_color.m_diffuseMultiplier);
	light.SetSpecularMult(m_color.m_specularMultiplier);

	light.m_fHDRDynamic = 0.f;

	if (m_options.m_bAffectsOnlyThisArea)
		light.m_Flags |= DLF_THIS_AREA_ONLY;

	if (m_options.m_bIgnoreVisAreas)
		light.m_Flags |= DLF_IGNORES_VISAREAS;

	if (m_options.m_bVolumetricFogOnly)
		light.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;

	if (m_options.m_bAffectsVolumetricFog)
		light.m_Flags |= DLF_VOLUMETRIC_FOG;

	if (m_options.m_bAmbient)
		light.m_Flags |= DLF_AMBIENT;

	if (m_shadows.m_castShadowSpec != Torch::EMiniumSystemSpec::Disabled && (int)gEnv->pSystem->GetConfigSpec() >= (int)m_shadows.m_castShadowSpec)
	{
		light.m_Flags |= DLF_CASTSHADOW_MAPS;

		light.SetShadowBiasParams(1.f, 1.f);
		light.m_fShadowUpdateMinRadius = light.m_fRadius;

		float shadowUpdateRatio = 1.f;
		light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));
	}
	else
		light.m_Flags &= ~DLF_CASTSHADOW_MAPS;

	light.m_fAttenuationBulbSize = m_options.m_attenuationBulbSize;

	light.m_fFogRadialLobe = m_options.m_fogRadialLobe;

	//load particle emitter
	if (m_hasParticle)
	{
		pParticleEffect = gEnv->pParticleManager->FindEffect("smoke_and_fire.fire_small");
		m_pEntity->LoadParticleEmitter(1, pParticleEffect, 0, true);
	}
	// Load the light source into the entity
	m_pEntity->LoadLight(GetOrMakeEntitySlotId(), &light);

	uint32 slotFlags = m_pEntity->GetSlotFlags(GetEntitySlotId());
	//UpdateGIModeEntitySlotFlags((uint8)m_options.m_giMode, slotFlags);
	m_pEntity->SetSlotFlags(GetEntitySlotId(), slotFlags);
}

IParticleEmitter * CTorchComponent::SpawnParticleEffect(IParticleEffect * pParticleEffect)
{
	if (pParticleEffect)
	{

		Vec3 lPos = GetEntity()->GetPos();
		SEntitySpawnParams params;
		params.vPosition = lPos;

		IParticleEmitter* pEmitter = pParticleEffect->Spawn(params.vPosition);

		return pEmitter;
	}

	return NULL;
}

