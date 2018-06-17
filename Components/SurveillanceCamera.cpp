#include "SurveillanceCamera.h"

static void RegisterSurveillanceCamera(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CSurveillanceCameraComponent));
	}
}
CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterSurveillanceCamera);

void CSurveillanceCameraComponent::Initialize()
{
	SetComponentFlags(GetComponentFlags() | EEntityComponentFlags::NoSave);
	Reset();
}

uint64 CSurveillanceCameraComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_UPDATE) | BIT64(ENTITY_EVENT_RESET) | BIT64(ENTITY_EVENT_COLLISION);
	return uint64();
}

void CSurveillanceCameraComponent::ProcessEvent(SEntityEvent & event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_RESET:
		{
			if (event.nParam[0] == 1)
				m_isGameMode = true;
			else
				m_isGameMode = false;
		}
		break;
		case ENTITY_EVENT_UPDATE:
			if (m_isGameMode)
			{
			}
			break;
		case ENTITY_EVENT_START_GAME:
		{
			Reset();
		}
		break;

	}
}

void CSurveillanceCameraComponent::SerializeProperties(Serialization::IArchive & archive)
{
	if (archive.openBlock("SurveillanceComponent", "SurveillanceComponent"))
	{
		//archive(m_life, "Life", "Life");
		archive.closeBlock();
	}
}

void CSurveillanceCameraComponent::ReflectType(Schematyc::CTypeDesc<CSurveillanceCameraComponent>& desc)
{
	desc.SetGUID("{81325ECE-4DDE-45FC-B7B7-0374E7DF2440}"_cry_guid);
	desc.SetEditorCategory("Interactable Components");
	desc.SetLabel("Surveillance camera Component");
	desc.SetDescription("Surveillance camera Component properties");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	desc.AddMember(&CSurveillanceCameraComponent::m_surveillGeomPath, 'inta', "FilePathIntact", "Intact", "Determines the CGF to load", "Objects/default/primitive_pyramid.cgf");

}

IEntity* CSurveillanceCameraComponent::Raycast(Vec3 dir, Vec3 pos, float rayLength)
{
	ray_hit hit;
	static const unsigned int rayFlags = rwi_stop_at_pierceable | rwi_colltype_any;
	gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir * rayLength, ent_living, rayFlags, &hit, 1, m_pEntity->GetPhysicalEntity());

	if (hit.pCollider)
	{
		IPhysicalEntity *pHitEntity = hit.pCollider;
		IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pHitEntity);

		m_foundPlayer = true;
		hitLocation = hit.pt;
		return pEntity;
	}
	return NULL;
}

void CSurveillanceCameraComponent::Reset()
{
	//m_camSearch = Searching;
	m_foundPlayer = false;
	//rotator = -1;

	int componentSlot = GetOrMakeEntitySlotId();

	GetEntity()->FreeSlot(componentSlot);
	GetEntity()->UnphysicalizeSlot(componentSlot);

	GetEntity()->LoadGeometry(componentSlot, m_surveillGeomPath.value);

	SEntityPhysicalizeParams params;
	//params.type = PE_RIGID;
	//params.mass = 10.0f;

	GetEntity()->Physicalize(params);

	//m_life = 10.0f;
	CryLog("TutoringGame: surveillance component initialized!");
}
