#pragma once

class CUserSettings
{
public:
	CUserSettings() {}
	~CUserSettings() { UnregisterCVars(); }

	void RegisterCVars();
	void UnregisterCVars();

};