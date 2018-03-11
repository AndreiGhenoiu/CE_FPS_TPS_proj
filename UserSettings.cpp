#include "UserSettings.h"
#include <CrySystem/ISystem.h>
#include <CrySystem/IConsole.h>

void CUserSettings::RegisterCVars()
{
	ConsoleRegistrationHelper::RegisterFloat("g_WalkingSpeed", 15.0f, VF_RESTRICTEDMODE, "Adjust player walking speed");
}

void CUserSettings::UnregisterCVars()
{
	IConsole *pConsole = gEnv->pConsole;
	pConsole->UnregisterVariable("g_WalkingSpeed", true);
}
