#pragma once

#include <bakkesmod/wrappers/cvarmanagerwrapper.h>

#include "Cache/CacheManager.h"
#include "Spotify/Song.h"
#include "Spotify/SpotifyPlaylist.h"


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
	//SpotifyPlaylist GetPlaylist(std::string);
	//Song GetSong(std::string);
	//std::vector<Song> GetSongs(std::vector<std::string>);
	std::wstring GetSongPath(std::string);

	void DownloadPreview(Song);
	void DownloadPreviews(std::deque<Song>);
	SpotifyPlaylist GetPlaylist(std::string playlistId, bool doRetry = true);
};