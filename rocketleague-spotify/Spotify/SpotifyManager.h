#pragma once

#include <bakkesmod/wrappers/cvarmanagerwrapper.h>

#include "Cache/CacheManager.h"

class Song {
public:
	std::string id;
	std::string previewUrl;
	std::wstring name;
	std::wstring artist;
	std::wstring album;
	std::string albumArtUrl;
	std::wstring path;

	Song(std::string id,
		std::string previewUrl,
		std::wstring name,
		std::wstring artist,
		std::wstring album,
		std::string albumArtUrl,
		std::wstring path) {

		this->id = id;
		this->previewUrl = previewUrl;
		this->name = name;
		this->artist = artist;
		this->album = album;
		this->albumArtUrl = albumArtUrl;
		this->path = path;
	}
};

class SpotifyPlaylist {
public:
	std::deque<Song> songs;
	std::wstring name;
	std::string id;
	unsigned seed;
	int index = -1;

	size_t Size() { return songs.size(); }
	void Shuffle(std::default_random_engine rng) {
		std::shuffle(songs.begin(), songs.end(), rng);
	}
};

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