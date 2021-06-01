#include "stdafx.h"

#include "Helpers.h"


int Helpers::RandomNumber() {
	std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> distribution{ 1 };

	return distribution(generator);
}

std::string Helpers::RandomString(int length) {  // Partially lifted from https://stackoverflow.com/questions/47977829/generate-a-random-string-in-c11
	const std::string allowedCharacters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.-~";

	std::mt19937 generator{ std::random_device{}() };
	std::uniform_int_distribution<int> distribution{ 0, int(allowedCharacters.size() - 1) };  // uses ASCII range
	std::string rand_str(length, '\0');
	for (auto& currentCharacter : rand_str)
		currentCharacter = allowedCharacters[distribution(generator)];

	return rand_str;
}

std::wstring Helpers::StrToWStr(std::string str) {
	std::wstring wstr = std::wstring(str.length(), L' ');
	std::copy(str.begin(), str.end(), wstr.begin());

	return wstr;
}

/*
	The Save/LoadCVars functions are intended to reliably save authentication values.
	This is required because we don't have control over when the BakkesMod config.cfg is saved/loaded
*/
void Helpers::LoadCVars(std::shared_ptr<CVarManagerWrapper> cvarManager, std::wstring filePath) {
	std::ifstream iStream(filePath);
	
	nlohmann::json cVars = nlohmann::json::parse(iStream);
	
	for (auto& [key, val] : cVars.items()) {
		cvarManager->getCvar(key).setValue(std::string(val));
	}
}

void Helpers::SaveCVars(std::shared_ptr<CVarManagerWrapper> cvarManager, std::wstring filePath) {
	const std::string cVarsToSave[]  = { "RLS_CodeVerifier", "RLS_SpotifyAuthCode", "RLS_SpotifyAccessToken", "RLS_SpotifyRefreshToken" };

	nlohmann::json cVars;
	for (std::string key : cVarsToSave) {
		cVars[key] = cvarManager->getCvar(key).getStringValue();
	}

	std::ofstream oStream(filePath);
	oStream << cVars.dump();
}
