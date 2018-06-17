#include "StdAfx.h"
#include "SurveillanceComponent.h"
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>


static void RegisterSurveillanceComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CSurveillaceComponent));
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterSurveillanceComponent);

void CSurveillaceComponent::Initialize()
{
	Reset();

	//CGamePlugin* pPlugin = gEnv->pSystem->GetIPluginManager()->QueryPlugin<CGamePlugin>();
	//m_pPlayerComponent = pPlugin->m_pPlayer;
	m_pSurveillanceCameraComponent = GetEntity()->CreateComponentClass<CSurveillanceCameraComponent>();
	m_pFlashlightComponent = GetEntity()->CreateComponentClass<CFlashlightComponent>();

	m_pFlashlightComponent->m_color.m_color = ColorF(1.0f, 1.f, 1.f);
	m_pFlashlightComponent->m_color.m_diffuseMultiplier = 5.f;
	m_pFlashlightComponent->Enable(true);

	Matrix34 flashlightTM = Matrix34::CreateIdentity();
	flashlightTM.SetRotationZ(DEG2RAD(90.f));
	m_pFlashlightComponent->SetTransformMatrix(flashlightTM);
}

uint64 CSurveillaceComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_UPDATE) | BIT64(ENTITY_EVENT_RESET) | BIT64(ENTITY_EVENT_COLLISION);
}

void CSurveillaceComponent::ProcessEvent(SEntityEvent & event)
{

	switch (event.event)
	{
	case ENTITY_EVENT_RESET:
	{
		if (event.nParam[0] == 1)
		{
			m_pFlashlightComponent->m_color.m_color = ColorF(1.0f, 1.f, 1.f);
			m_pFlashlightComponent->m_color.m_diffuseMultiplier = 5.f;
			m_pFlashlightComponent->m_animations.m_style = 0;
			m_pFlashlightComponent->Enable(true);
			m_isGameMode = true;
		}
		else
			m_isGameMode = false;
		Reset();
		CryLog("reset event ");
	}
	break;

	case ENTITY_EVENT_UPDATE:
		if (m_isGameMode)
		{
			float fFrametime = gEnv->pTimer->GetFrameTime();

			if (m_camSearch == Searching)
			{
				Quat rot(GetEntity()->GetRotation());
				Quat multi;

				if (m_camRot == Rotate_left)
				{
					rotator = 1;
					if (RAD2DEG(rot.GetRotZ()) > 45.f)
						m_camRot = Rotate_right;
				}
				else if (m_camRot == Rotate_right)
				{
					rotator = -1;
					if (RAD2DEG(rot.GetRotZ()) < -45.f)
						m_camRot = Rotate_left;
				}

				multi = Quat(Vec3(0.f, 0.f, rotator * fFrametime * 0.2f));
				GetEntity()->SetRotation(rot * multi);

				m_HitEntity = m_pSurveillanceCameraComponent->Raycast(GetEntity()->GetWorldRotation().GetColumn1(), GetEntity()->GetWorldPos(), 10.0f);

				if (m_pSurveillanceCameraComponent->m_foundPlayer)
					m_camSearch = Found;
			}
			else if (m_camSearch == Found)
			{
				m_pFlashlightComponent->Enable(false);
				m_pFlashlightComponent->m_color.m_color = ColorF(1.0f, 0.f, 0.f);
				m_pFlashlightComponent->m_color.m_diffuseMultiplier = 20.f;
				m_pFlashlightComponent->m_animations.m_style = 7;
				m_pFlashlightComponent->Enable(true);
				m_camSearch = Tracking;
			}
			else if (m_camSearch == Tracking)
			{
				if (m_HitEntity)
				{
					Vec3 playerPosition = m_HitEntity->GetPos() + Vec3(0.f,0.f,1.5f);
					Vec3 camPosition = GetEntity()->GetPos();

					Vec3 vDir = playerPosition - camPosition;
					m_pEntity->SetRotation(Quat::CreateRotationVDir(vDir));

					if (vDir.len() > 10.f)
					{
						timer = 5.f;
						m_camSearch = Resetting;
					}

				}
			}
			else if(m_camSearch == Resetting)
			{
				timer -= fFrametime;
				if (timer < 0.f)
				{
					m_pSurveillanceCameraComponent->m_foundPlayer = false;

					m_pFlashlightComponent->Enable(false);
					m_pFlashlightComponent->m_color.m_color = ColorF(1.0f, 1.f, 1.f);
					m_pFlashlightComponent->m_color.m_diffuseMultiplier = 5.f;
					m_pFlashlightComponent->m_animations.m_style = 0;
					m_pFlashlightComponent->Enable(true);
					m_camSearch = Searching;
				}
			}
		}
		else
		{
			m_pFlashlightComponent->Enable(false);
			m_pFlashlightComponent->m_color.m_color = ColorF(1.0f, 1.f, 1.f);
			m_pFlashlightComponent->m_color.m_diffuseMultiplier = 5.f;
			m_pFlashlightComponent->m_animations.m_style = 0;
			m_pFlashlightComponent->Enable(true);
		}

	}
}

void CSurveillaceComponent::SerializeProperties(Serialization::IArchive & archive)
{
	if (archive.openBlock("CSurveillaceComponent", "CSurveillaceComponent"))
	{
		//archive(m_life, "Life", "Life");
		archive.closeBlock();
	}
}

void CSurveillaceComponent::ReflectType(Schematyc::CTypeDesc<CSurveillaceComponent>& desc)
{
	desc.SetGUID("{28C839EB-7013-403C-B910-11CCB1BE768F}"_cry_guid);
	desc.SetEditorCategory("Interactable Components");
	desc.SetLabel("Test movements");
	desc.SetDescription("Test movements properties");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	desc.AddMember(&CSurveillaceComponent::m_intactGeomPath, 'inta', "FilePathIntact", "Intact", "Determines the CGF to load", "Objects/default/primitive_box.cgf");

}

void CSurveillaceComponent::Reset()
{
	m_camSearch = Searching;
	m_camRot = Rotate_left;

	rotator = -1;

	int componentSlot = GetOrMakeEntitySlotId();
	GetEntity()->FreeSlot(componentSlot);
	GetEntity()->UnphysicalizeSlot(componentSlot);

	GetEntity()->LoadGeometry(GetOrMakeEntitySlotId(), m_intactGeomPath.value);
	SEntityPhysicalizeParams params;
	//params.type = PE_RIGID;
	//params.mass = 10.0f;

	GetEntity()->Physicalize(params);


	//m_life = 10.0f;
	CryLog("TutoringGame: test movements initialized!");
}
