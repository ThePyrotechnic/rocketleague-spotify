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

void SpotifyManager::DownloadPreview(Song song) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(song.id);
	std::wstring filePath = cacheManager.GetCachedSong(wSongId);
	if (!filePath.empty()) { cvarManager->log("song was cached!"); return; }

	cpr::Response res = cpr::Get(cpr::Url{ song.previewUrl });
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code == 200) {
		std::fstream previewFile = std::fstream(song.path, std::ios::out | std::ios::binary);
		const char* content = res.text.c_str();
		std::stringstream sstream(res.header["Content-Length"]);
		size_t size;
		sstream >> size;
		previewFile.write(content, size);
		previewFile.close();
		cvarManager->log("Downloaded song");
	}
	else {
		cvarManager->log("Error fetching preview for song: " + song.id);
	}
}

std::wstring SpotifyManager::GetSongPath(std::string songId) {
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(songId);
	return audioDir + wSongId + L".mp3";
}

void SpotifyManager::DownloadPreviews(std::vector<Song> songs) {
	for (Song song : songs) {
		SpotifyManager::DownloadPreview(song);
	}
	cvarManager->getCvar("RLS_GoalSongStatus").setValue("Downloaded " + std::to_string(songs.size()) + " songs!");
}

SpotifyPlaylist SpotifyManager::GetPlaylist(std::string playlistId, bool doRetry) {

	SpotifyPlaylist playlist = SpotifyPlaylist();
	playlist.id = playlistId;

	if (playlistId.empty()) {
		return playlist;
	}

	// https://developer.spotify.com/documentation/web-api/reference/#category-playlists
	cvarManager->log("getting playlist");
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/playlists/" + playlistId + "?market=US" },
		cpr::Header{ { "Authorization", "Bearer " + token } });
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
		if (doRetry) {
			Authenticate();
			return GetPlaylist(playlistId, false);
		}
	}
	else {
		auto playlistResponse = json::parse(res.text);
		auto items = playlistResponse["tracks"]["items"];
		if (!items.is_null()) {
			playlist.name = RocketLeagueSpotify::StrToWStr(playlistResponse["name"]);
			for (int i = 0; i < items.size(); i++) {
				if (!items[i]["track"].is_null() && !items[i]["track"]["preview_url"].is_null()) {

					std::string previewUrl = items[i]["track"]["preview_url"];
					std::string id = items[i]["track"]["id"];
					std::wstring name = RocketLeagueSpotify::StrToWStr(items[i]["track"]["name"]);
					std::wstring artist = RocketLeagueSpotify::StrToWStr(items[i]["track"]["artists"][0]["name"]);
					std::wstring album = RocketLeagueSpotify::StrToWStr(items[i]["track"]["album"]["name"]);
					std::string albumArtUrl = items[i]["track"]["album"]["images"][0]["url"];
					std::wstring path = SpotifyManager::GetSongPath(id);

					Song song(id, previewUrl, name, artist, album, albumArtUrl, path);
					std::wstring output = RocketLeagueSpotify::StrToWStr(id) + L" | " + RocketLeagueSpotify::StrToWStr(previewUrl) + L" | " + name + L" | " + artist + L" | " + album + L" | " + RocketLeagueSpotify::StrToWStr(albumArtUrl);
					cvarManager->log(output);
					cvarManager->log("Got preview URL for song: " + id + ": " + previewUrl);
					playlist.songs.push_back(song);
				}
				else {
					cvarManager->log("No preview URL for song: " + i);
				}
			}
			std::thread t([&](std::vector<Song> songs) {
				SpotifyManager::DownloadPreviews(songs);
				}, playlist.songs);
			t.detach();

			unsigned seed = std::chrono::system_clock::now()
				.time_since_epoch()
				.count();
			//test
			seed = 0;
			playlist.seed = seed;
			shuffle(playlist.songs.begin(), playlist.songs.end(), std::default_random_engine(seed));
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

// revisit when cache upgraded to use Song object

/*
Song SpotifyManager::GetSong(std::string songId) {
	Song song;
	if (songId.empty()) {
		return song;
	}
	std::wstring wSongId = RocketLeagueSpotify::StrToWStr(songId);
	std::wstring filePath = cacheManager.GetCachedSong(wSongId);
	// TODO: get cached song info
	if (!filePath.empty()) {
		song.path = filePath;
		cvarManager->log("song was cached!"); return song;
	}

	// https://developer.spotify.com/documentation/web-api/reference/#category-tracks
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/tracks/" + songId + "?market=US"},
		cpr::Header{ { "Authorization", "Bearer " + token } });
	cvarManager->log("getting song preview url");
	cvarManager->log(std::to_string(res.status_code));
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
		return song;
	}
	else {
		auto track = json::parse(res.text);
		std::string previewUrl;
		if (!track["preview_url"].is_null()) {
			previewUrl = track["preview_url"];
			cvarManager->log("Got preview URL: " + previewUrl);
			song.id = songId;
			song.path = SpotifyManager::GetSongPath(songId);
			song.previewUrl = previewUrl;
			song.name = track["name"];
			song.artist = track["artists"][0]["name"];
			song.album = track["album"]["name"];
			song.albumArtUrl = track["album"]["images"][0]["url"];

			std::thread t(&SpotifyManager::DownloadPreview, song);
		}
		else {
			cvarManager->log("No preview URL available");
		}
	}
	return song;
}
*/
/*
std::vector<Song> SpotifyManager::GetSongs(std::vector<std::string> songIds) {
	std::vector<Song> songs;
	if (songIds.size() == 0) {
		return songs;
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
		auto tracks = json::parse(res.text);
		if (!tracks["tracks"].is_null()) {
			for (int i = 0; i < songIds.size(); i++) {
				if (!tracks["tracks"][i]["preview_url"].is_null()) {
					std::string previewUrl = tracks["tracks"][i]["preview_url"];
					if (!previewUrl.empty()) {

						cvarManager->log("Got preview URL for song: " + songIds[i] + ":" + previewUrl);
						std::string name = tracks["tracks"][i]["name"];
						std::string artist = tracks["tracks"][i]["artists"][0]["name"];
						std::string album = tracks["tracks"][i]["album"]["name"];
						std::string albumArtUrl = tracks["tracks"][i]["album"]["images"][0]["url"];
						std::wstring path = SpotifyManager::GetSongPath(songIds[i]);

						Song song(songIds[i], previewUrl, name, artist, album, albumArtUrl, path);
						songs.push_back(song);
					}
					else {
						cvarManager->log("No preview URL for song: " + songIds[i]);
					}
				}
			}
		}
		std::thread t(&SpotifyManager::DownloadPreviews, songs);
	}
	return songs;
}
*/