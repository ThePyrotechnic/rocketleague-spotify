#pragma once

class CacheManager {
public:
	CacheManager();
	CacheManager(std::wstring, std::wstring);

	std::wstring audioCacheDir;
	std::wstring imageCacheDir;

	std::unordered_map<std::wstring, std::wstring> audioCache;
	std::unordered_map<std::wstring, std::wstring> imageCache;

	void RescanCache();

	std::wstring GetCachedAudio(std::wstring);
	std::wstring GetCachedImage(std::wstring);
};