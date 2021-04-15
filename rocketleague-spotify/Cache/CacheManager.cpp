#include "stdafx.h"

#include "CacheManager.h"


CacheManager::CacheManager() {}

CacheManager::CacheManager(std::wstring audioCacheDir) {
	this->audioCacheDir = audioCacheDir;
	RescanCache();
}

void CacheManager::RescanCache() {
	if (audioCacheDir.empty()) return;
	for (auto entry : std::filesystem::directory_iterator(audioCacheDir)) {
		if (entry.is_regular_file()) {
			audioCache.insert({ entry.path().stem().wstring(), entry.path().wstring() });
		}
	}
}


std::wstring CacheManager::GetCachedSong(std::wstring songId) {
	auto result = audioCache.find(songId);
	if (result != audioCache.end()) {
		return result->second;
	}
	return L"";
}