#pragma once

#include <bakkesmod/wrappers/cvarmanagerwrapper.h>

#include "Helpers.h"
#include "Cache/CacheManager.h"
#include "Spotify/Song.h"
#include "Spotify/SpotifyPlaylist.h"


class SpotifyManager {
public:
	SpotifyManager();
	SpotifyManager(std::shared_ptr<CVarManagerWrapper>, std::wstring, std::wstring, std::wstring);
	std::shared_ptr<CVarManagerWrapper> cvarManager;
	std::wstring audioDir;
	std::wstring imageDir;
	std::wstring modDir;
	nlohmann::json config;
	CacheManager cacheManager;

	std::wstring GetAudioPath(std::string);
	std::wstring GetImagePath(std::string);

	void DownloadPreview(Song);
	void DownloadFiles(std::deque<Song>);
	void DownloadImage(Song);
	SpotifyPlaylist GetPlaylist(std::string, bool=true, bool=true);
	void ParsePlaylist(SpotifyPlaylist&, nlohmann::json&);

	void StartSpotifyAuthFlow();
	void RefreshAuthCode();
	void ExchangeCodeForAccess(std::string);

private:
	static const std::string clientID;
};