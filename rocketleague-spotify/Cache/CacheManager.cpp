#include "stdafx.h"

#include "CacheManager.h"


CacheManager::CacheManager() {}

CacheManager::CacheManager(std::wstring audioCacheDir, std::wstring imageCacheDir) {
	this->audioCacheDir = audioCacheDir;
	this->imageCacheDir = imageCacheDir;
	RescanCache();
}

void CacheManager::RescanCache() {
	if (!audioCacheDir.empty()) {
		for (auto entry : std::filesystem::directory_iterator(audioCacheDir)) {
			if (entry.is_regular_file()) {
				audioCache.insert({ entry.path().stem().wstring(), entry.path().wstring() });
			}
		}
	}
	if (!imageCacheDir.empty()) {
		for (auto entry : std::filesystem::directory_iterator(imageCacheDir)) {
			if (entry.is_regular_file()) {
				imageCache.insert({ entry.path().stem().wstring(), entry.path().wstring() });
			}
		}
	}
}


std::wstring CacheManager::GetCachedAudio(std::wstring songId) {
	auto result = audioCache.find(songId);
	if (result != audioCache.end()) {
		return result->second;
	}
	return L"";
}

std::wstring CacheManager::GetCachedImage(std::wstring songId) {
	auto result = imageCache.find(songId);
	if (result != imageCache.end()) {
		return result->second;
	}
	return L"";
}