#pragma once

class CacheManager {
public:
	CacheManager();
	CacheManager(std::wstring);

	std::wstring audioCacheDir;

	std::unordered_map<std::wstring, std::wstring> audioCache;

	void RescanCache();

	std::wstring GetCachedSong(std::wstring);
};