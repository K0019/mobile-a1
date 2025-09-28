#pragma once

class CrashHandler
{
private:
	CrashHandler() = default;
public:
	static void SetupCrashHandler();
};
