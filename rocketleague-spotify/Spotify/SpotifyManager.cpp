#include "stdafx.h"

#include "SpotifyManager.h"
#include "RocketLeagueSpotify.h"

using json = nlohmann::json;

SpotifyManager::SpotifyManager() {}

SpotifyManager::SpotifyManager(std::shared_ptr<CVarManagerWrapper> cvarManager, std::wstring modDir, std::wstring audioDir, std::wstring imageDir) {
	this->cvarManager = cvarManager;
	this->modDir = modDir;
	this->audioDir = audioDir;
	this->imageDir = imageDir;
	std::ifstream i(this->modDir + LR"(\config.json)");
	i >> config;
	credential = config["spotifyId:SecretBase64"];
	Authenticate();
	cacheManager = CacheManager(audioDir, imageDir);
}

void SpotifyManager::DownloadPreview(Song song) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(song.id);
	std::wstring filePath = cacheManager.GetCachedAudio(wSongId);
	if (!filePath.empty()) { return; }

	cpr::Response res = cpr::Get(cpr::Url{ song.previewUrl });
	if (res.status_code == 200) {
		std::fstream previewFile = std::fstream(song.audioPath, std::ios::out | std::ios::binary);
		const char* content = res.text.c_str();
		std::stringstream sstream(res.header["Content-Length"]);
		size_t size;
		sstream >> size;
		previewFile.write(content, size);
		previewFile.close();
	}
}

void SpotifyManager::DownloadImage(Song song) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(song.id);
	std::wstring filePath = cacheManager.GetCachedImage(wSongId);
	if (!filePath.empty()) { return; }

	cpr::Response res = cpr::Get(cpr::Url{ song.albumArtUrl });
	if (res.status_code == 200) {
		std::fstream imageFile = std::fstream(song.imagePath, std::ios::out | std::ios::binary);
		const char* content = res.text.c_str();
		std::stringstream sstream(res.header["Content-Length"]);
		size_t size;
		sstream >> size;
		imageFile.write(content, size);
		imageFile.close();
	}
}

std::wstring SpotifyManager::GetAudioPath(std::string songId) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(songId);
	return audioDir + wSongId + L".mp3";
}

std::wstring SpotifyManager::GetImagePath(std::string songId) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(songId);
	return imageDir + wSongId + L".jpg";
}

void SpotifyManager::DownloadFiles(std::deque<Song> songs) {
	for (Song song : songs) {
		DownloadPreview(song);
		DownloadImage(song);
	}
}


void SpotifyManager::ParsePlaylist(SpotifyPlaylist &playlist, json &items) {
	for (int i = 0; i < items.size(); i++) {
		if (!items[i]["track"].is_null() && !items[i]["track"]["preview_url"].is_null()) {

			std::string previewUrl = items[i]["track"]["preview_url"];
			std::string id = items[i]["track"]["id"];
			std::wstring name = RocketLeagueSpotify::StrToWStr(items[i]["track"]["name"]);
			std::wstring artist = RocketLeagueSpotify::StrToWStr(items[i]["track"]["artists"][0]["name"]);
			std::wstring album = RocketLeagueSpotify::StrToWStr(items[i]["track"]["album"]["name"]);
			std::string albumArtUrl = items[i]["track"]["album"]["images"][0]["url"];
			std::wstring audioPath = SpotifyManager::GetAudioPath(id);
			std::wstring imagePath = SpotifyManager::GetImagePath(id);

			Song song(id, previewUrl, name, artist, album, albumArtUrl, audioPath, imagePath);
			std::wstring output = RocketLeagueSpotify::StrToWStr(id) + L" | " + RocketLeagueSpotify::StrToWStr(previewUrl) + L" | " + name + L" | " + artist + L" | " + album + L" | " + RocketLeagueSpotify::StrToWStr(albumArtUrl);
			// cvarManager->log(output);
			playlist.songs.push_back(song);
		}
		else {
			cvarManager->log("No preview URL for song: " + i);
		}
	}
}

SpotifyPlaylist SpotifyManager::GetPlaylist(std::string playlistId, bool doRetry) {

	SpotifyPlaylist playlist = SpotifyPlaylist();
	playlist.id = playlistId;

	if (playlistId.empty()) {
		return playlist;
	}

	// https://developer.spotify.com/documentation/web-api/reference/#category-playlists
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/playlists/" + playlistId + "?market=US" },
		cpr::Header{ { "Authorization", "Bearer " + token } });
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
		if (doRetry) {
			Authenticate();
			return GetPlaylist(playlistId, false);
		}
	}
	else {
		json playlistResponse = json::parse(res.text);
		json items = playlistResponse["tracks"]["items"];
		if (!items.is_null()) {
			playlist.name = RocketLeagueSpotify::StrToWStr(playlistResponse["name"]);

			ParsePlaylist(playlist, items);

			json paginationURL = playlistResponse["tracks"]["next"];
			while (!paginationURL.is_null()) {
				cpr::Response res = cpr::Get(
					cpr::Url{ paginationURL },
					cpr::Header{ { "Authorization", "Bearer " + token } });
				if (res.status_code != 200) {
					cvarManager->log("Bad response");
					if (doRetry) {
						Authenticate();
					}
				}
				else {
					json paginationResponse = json::parse(res.text);
					items = paginationResponse["items"];
					paginationURL = paginationResponse["next"];
					if (!items.is_null()) {
						ParsePlaylist(playlist, items);
					}
				}
			}

			std::thread t([&](std::deque<Song> songs) {
				DownloadFiles(songs);
			}, playlist.songs);
			t.detach();
		}
		else {
			cvarManager->log("Playlist empty or not found");
		}
	}

	return playlist;
}

int SpotifyManager::Authenticate() {
	cpr::Response res = cpr::Post(
		cpr::Url{ "https://accounts.spotify.com/api/token" },
		cpr::Header{ { "Authorization", "Basic " + credential} },
		cpr::Parameters{ { "grant_type", "client_credentials" } });

	auto authResponse = json::parse(res.text);
	std::string access_token = authResponse["access_token"];
	token = access_token;

	return res.status_code;
}