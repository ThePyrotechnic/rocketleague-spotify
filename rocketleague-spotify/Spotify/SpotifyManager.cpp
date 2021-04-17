#include "stdafx.h"

#include "RocketLeagueSpotify.h"
#include "SpotifyManager.h"


using json = nlohmann::json;

SpotifyManager::SpotifyManager() {}

SpotifyManager::SpotifyManager(std::shared_ptr<CVarManagerWrapper> cvarManager, std::wstring modDir, std::wstring audioDir) {
	this->cvarManager = cvarManager;
	this->modDir = modDir;
	this->audioDir = audioDir;
	std::ifstream i(this->modDir + LR"(\config.json)");
	i >> config;
	credential = config["spotifyId:SecretBase64"];
	this->cvarManager->log("Spotify Credential: " + credential);
	Authenticate();
	cacheManager = CacheManager(audioDir);
}

std::wstring SpotifyManager::DownloadPreview(std::string songId, std::string previewUrl) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(songId);

	std::wstring filePath = cacheManager.GetCachedSong(wSongId);
	if (!filePath.empty()) { cvarManager->log("song was cached!"); return filePath; }

	cpr::Response res = cpr::Get(cpr::Url{ previewUrl });
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code != 200) {
		return L"";
	}

	filePath = audioDir + wSongId + L".mp3";
	std::fstream previewFile = std::fstream(filePath, std::ios::out | std::ios::binary);
	const char* content = res.text.c_str();
	std::stringstream sstream(res.header["Content-Length"]);
	size_t size;
	sstream >> size;
	previewFile.write(content, size);
	previewFile.close();
	cvarManager->log("Downloaded song");

	return filePath;
}

std::wstring SpotifyManager::DownloadSong(std::string songId) {
	if (songId.empty()) {
		return L"";
	}
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(songId);

	std::wstring filePath = cacheManager.GetCachedSong(wSongId);
	if (!filePath.empty()) { cvarManager->log("song was cached!"); return filePath; }

	// https://developer.spotify.com/documentation/web-api/reference/#category-tracks
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/tracks/" + songId + "?market=US"},
		cpr::Header{ { "Authorization", "Bearer " + token } });
	cvarManager->log("getting song preview url");
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
		return L"";
	}
	auto trackResponse = json::parse(res.text);
	std::string previewUrl;
	if (!trackResponse["preview_url"].is_null()) {
		previewUrl = trackResponse["preview_url"];
		cvarManager->log("Got preview URL: " + previewUrl);
		return SpotifyManager::DownloadPreview(songId, previewUrl);
	}
	else {
		cvarManager->log("No preview URL available");
		return L"";
	}
	
}

std::vector<std::wstring> SpotifyManager::DownloadSongs(std::vector<std::string> songIds) {
	std::vector<std::wstring> filePaths;
	if (songIds.size() == 0) {
		return filePaths;
	}

	std::string songIdsStrTemp = "";
	for (std::string songId : songIds) {
		songIdsStrTemp = songIdsStrTemp + songId + "%2C";
	}
	std::string songIdsStr = songIdsStrTemp.substr(0, songIdsStrTemp.size() - 3);
	songIdsStr = songIdsStr + "&market=US";

	// https://developer.spotify.com/documentation/web-api/reference/#category-tracks
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/tracks/" + songIdsStr },
		cpr::Header{ { "Authorization", "Bearer " + token } });
	cvarManager->log("getting song preview urls");
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
	}
	else {
		auto tracksResponse = json::parse(res.text);
		if (!tracksResponse["tracks"].is_null()) {
			for (int i = 0; i < songIds.size(); i++) {
				if (!tracksResponse["tracks"][i]["preview_url"].is_null()) {
					std::string previewUrl = tracksResponse["tracks"][i]["preview_url"];
					if (!previewUrl.empty()) {
						std::wstring filePath = SpotifyManager::DownloadPreview(songIds[i], previewUrl);
						cvarManager->log("Got preview URL for song: " + songIds[i] + ":" + previewUrl);
						filePaths.push_back(filePath);
					}
					else {
						cvarManager->log("No preview URL for song: " + songIds[i]);
					}
				}
			}
		}
	}
	return filePaths;
}

std::vector<std::wstring> SpotifyManager::DownloadPlaylist(std::string playlistId) {
	std::vector<std::wstring> filePaths;
	if (playlistId.empty()) {
		return filePaths;
	}

	// https://developer.spotify.com/documentation/web-api/reference/#category-playlists
	cvarManager->log("getting playlist");
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/playlists/" + playlistId + "?market=US" },
		cpr::Header{ { "Authorization", "Bearer " + token } }); 
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
	}
	else {
		auto playlistResponse = json::parse(res.text);
		auto items = playlistResponse["tracks"]["items"];
		if (!items.is_null()) {
			for (int i = 0; i < items.size(); i++) {
				if (!items[i]["track"].is_null() && !items[i]["track"]["preview_url"].is_null() && 
					!items[i]["track"]["id"].is_null()) {
					std::string previewUrl = items[i]["track"]["preview_url"];
					std::string id = items[i]["track"]["id"];
			
					std::wstring filePath = SpotifyManager::DownloadPreview(items[i]["track"]["id"], previewUrl);
					if (!filePath.empty()) {
						cvarManager->log("Got preview URL for song: " + id + ": " + previewUrl);
						filePaths.push_back(filePath);
					}
				}
				else {
					cvarManager->log("No preview URL for song: " + i);
				}
			}
		}
		else {
			cvarManager->log("Playlist empty or not found");
		}
	}
	return filePaths;
}

int SpotifyManager::Authenticate() {
	cpr::Response res = cpr::Post(
		cpr::Url{ "https://accounts.spotify.com/api/token" },
		cpr::Header{ { "Authorization", "Basic " + credential} },
		cpr::Parameters{ { "grant_type", "client_credentials" } }); 

	cvarManager->log(credential);
	cvarManager->log("AUTH");
	cvarManager->log(std::to_string(res.status_code));
	cvarManager->log(res.text);
	auto authResponse = json::parse(res.text);
	std::string access_token = authResponse["access_token"];
	cvarManager->log(access_token);
	token = access_token;

	return res.status_code;
}