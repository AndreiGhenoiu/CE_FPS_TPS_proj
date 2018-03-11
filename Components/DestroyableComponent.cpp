#include "DestroyableComponent.h"
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>


static void RegisterDestroyableComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CDestroyableComponent));
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterDestroyableComponent);


void CDestroyableComponent::Initialize()
{
	m_isGameMode = false;
	m_alive = true;
	Reset();
}

uint64 CDestroyableComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_UPDATE) | BIT64(ENTITY_EVENT_RESET) | BIT64(ENTITY_EVENT_COLLISION);
}

void CDestroyableComponent::ProcessEvent(SEntityEvent & event)
{
	switch (event.event)
	{

	case ENTITY_EVENT_RESET:
		if (event.nParam[0] == 1)
			m_isGameMode = true;
		else
			m_isGameMode = false;
		Reset();
		break;
	case ENTITY_EVENT_UPDATE:

		break;
	case ENTITY_EVENT_COLLISION:
		if (event.nParam[1] == 0)
			break;
		pCollision = reinterpret_cast<EventPhysCollision*>(event.nParam[0]);
		CryLog("mass is %f", pCollision->mass[0]);
		DecrementLife(pCollision->mass[0]);

		break;
	case ENTITY_EVENT_START_GAME:
		Reset();
		break;


	}
}

void CDestroyableComponent::ReflectType(Schematyc::CTypeDesc<CDestroyableComponent>& desc)
{
	desc.SetGUID("{550B45C5-7AA4-46BF-B89D-6FCFA70FF710}"_cry_guid);
	desc.SetEditorCategory("Interactable Components");
	desc.SetLabel("Destroyable Component");
	desc.SetDescription("Destroyable component properties");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	desc.AddMember(&CDestroyableComponent::m_life, 'ilif', "Life", "Component life", "Component life", 1.0f);
	desc.AddMember(&CDestroyableComponent::m_intactGeomPath, 'inta', "FilePathintact", "Intact", "CGF to load", "Objects/default/primitive_box.cgf");
	desc.AddMember(&CDestroyableComponent::m_destroyedGeomPath, 'dest', "FilePathdestroyed", "Destroyed", "CGF to load", "Objects/default/primitive_sphere.cgf");

}

void CDestroyableComponent::DecrementLife(float bulletMass)
{
	if (m_life <= 0.0f && m_alive)
	{
		DestroyObject(m_destroyedGeomPath);
		ApplyEffects();
		m_alive = false;
		return;
	}
	if (bulletMass == 0.25f && m_alive)
		m_life -= 1.0f;
}

void CDestroyableComponent::DestroyObject(Schematyc::GeomFileName pathToDestroyedObject)
{
	GetEntity()->FreeSlot(0);
	GetEntity()->UnphysicalizeSlot(0);

	GetEntity()->LoadGeometry(0, m_destroyedGeomPath.value);

	SEntityPhysicalizeParams params;
	params.type = PE_RIGID;
	params.mass = 10.0f;

	GetEntity()->Physicalize(params);
}

void CDestroyableComponent::ApplyEffects()
{
	//particle effect 
	IParticleEffect* pParticleEffect = gEnv->pParticleManager->FindEffect("smoke_and_fire.black_smoke.black_smoke");
	SpawnParticleEffect(pParticleEffect);
	PlaySound();

}

IParticleEmitter * CDestroyableComponent::SpawnParticleEffect(IParticleEffect * pParticleEffect)
{
	if (pParticleEffect)
	{
		SEntitySpawnParams params;
		params.vPosition = GetEntity()->GetPos();

		IParticleEmitter* pEmitter = pParticleEffect->Spawn(params.vPosition);
		return pEmitter;
	}
	return NULL;
}

void CDestroyableComponent::PlaySound()
{
	CryAudio::ControlId audioControlId = CryAudio::StringToId("explosion");
	CryAudio::SExecuteTriggerData const data("expl", CryAudio::EOcclusionType::Ignore, GetEntity()->GetWorldPos(), true, audioControlId);
	gEnv->pAudioSystem->ExecuteTriggerEx(data);
}

void CDestroyableComponent::Reset()
{
	GetEntity()->FreeSlot(0);
	GetEntity()->UnphysicalizeSlot(0);

	GetEntity()->LoadGeometry(0, m_intactGeomPath.value);

	SEntityPhysicalizeParams params;
	params.type = PE_RIGID;
	params.mass = 10.0f;

	GetEntity()->Physicalize(params);
}
