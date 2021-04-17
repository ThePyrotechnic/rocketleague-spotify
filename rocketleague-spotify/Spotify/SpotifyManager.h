#pragma once
//#include "bakkesmod/plugin/bakkesmodplugin.h"
//#include "bakkesmod/wrappers/includes.h"
//#include "nlohmann/json.hpp"
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>

#include "Cache/CacheManager.h"


class SpotifyManager {
public:
	SpotifyManager();
	SpotifyManager(std::shared_ptr<CVarManagerWrapper>, std::wstring, std::wstring);
	std::shared_ptr<CVarManagerWrapper> cvarManager;
	std::wstring audioDir;
	std::wstring modDir;
	std::string token;
	std::string credential;
	nlohmann::json config;
	CacheManager cacheManager;

	int Authenticate();
	std::vector<std::wstring> DownloadPlaylist(std::string);
	std::wstring DownloadPreview(std::string, std::string);
	std::wstring DownloadSong(std::string);
	std::vector<std::wstring> DownloadSongs(std::vector<std::string>);

};