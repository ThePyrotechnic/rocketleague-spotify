#pragma once
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>

class Helpers {
public:
	static std::string RandomString(int);
	static int RandomNumber();
	static std::wstring StrToWStr(std::string);
	static void LoadCVars(std::shared_ptr<CVarManagerWrapper>, std::wstring);
	static void SaveCVars(std::shared_ptr<CVarManagerWrapper>, std::wstring);
};
