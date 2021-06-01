#include "stdafx.h"

#include "SpotifyManager.h"

using json = nlohmann::json;

const std::string SpotifyManager::clientID = "98b5267390d64abbb05714be9456cc32";

SpotifyManager::SpotifyManager() {}

SpotifyManager::SpotifyManager(std::shared_ptr<CVarManagerWrapper> cvarManager, std::wstring modDir, std::wstring audioDir, std::wstring imageDir) {
	this->cvarManager = cvarManager;
	this->modDir = modDir;
	this->audioDir = audioDir;
	this->imageDir = imageDir;
	//std::ifstream i(this->modDir + LR"(\config.json)");
	//i >> config;
	//credential = config["spotifyId:SecretBase64"];
	//Authenticate();
	cacheManager = CacheManager(audioDir, imageDir);
}

void SpotifyManager::DownloadPreview(Song song) {
	std::wstring wSongId = Helpers::StrToWStr(song.id);
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
	std::wstring wSongId = Helpers::StrToWStr(song.id);
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
	std::wstring wSongId = Helpers::StrToWStr(songId);
	return audioDir + wSongId + L".mp3";
}

std::wstring SpotifyManager::GetImagePath(std::string songId) {
	std::wstring wSongId = Helpers::StrToWStr(songId);
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
			std::wstring name = Helpers::StrToWStr(items[i]["track"]["name"]);
			std::wstring artist = Helpers::StrToWStr(items[i]["track"]["artists"][0]["name"]);
			std::wstring album = Helpers::StrToWStr(items[i]["track"]["album"]["name"]);
			std::string albumArtUrl = "";
			if (!items[i]["track"]["album"]["images"].is_null() && items[i]["track"]["album"]["images"].is_array() && !items[i]["track"]["album"]["images"][0].is_null())
				albumArtUrl = items[i]["track"]["album"]["images"][0]["url"];
			std::wstring audioPath = SpotifyManager::GetAudioPath(id);
			std::wstring imagePath = SpotifyManager::GetImagePath(id);

			Song song(id, previewUrl, name, artist, album, albumArtUrl, audioPath, imagePath);
			std::wstring output = Helpers::StrToWStr(id) + L" | " + Helpers::StrToWStr(previewUrl) + L" | " + name + L" | " + artist + L" | " + album + L" | " + Helpers::StrToWStr(albumArtUrl);
			// cvarManager->log(output);
			playlist.songs.push_back(song);
		}
		else {
			if (!items[i]["track"].is_null()) {
				std::string name = items[i]["track"]["name"];
				cvarManager->log("No preview URL for song: " + name);
			}
			else
				cvarManager->log("No preview URL for song: " + std::to_string(i));

		}
	}
}

SpotifyPlaylist SpotifyManager::GetPlaylist(std::string playlistId, bool doRetry/* = true */, bool downloadSongs/* = true */) {

	SpotifyPlaylist playlist = SpotifyPlaylist();
	playlist.id = playlistId;

	if (playlistId.empty()) {
		return playlist;
	}

	std::string accessToken = cvarManager->getCvar("RLS_SpotifyAccessToken").getStringValue();
	cvarManager->log("Request access token: " + accessToken);

	// https://developer.spotify.com/documentation/web-api/reference/#category-playlists
	cpr::Response res = cpr::Get(
		cpr::Url{ "https://api.spotify.com/v1/playlists/" + playlistId + "?market=US" },
		cpr::Header{ { "Authorization", "Bearer " + accessToken } });
	if (res.status_code != 200) {
		cvarManager->log("Bad response");
		if (doRetry) {
			RefreshAuthCode();
			return GetPlaylist(playlistId, false);
		}
	}
	else {
		json playlistResponse = json::parse(res.text);
		json items = playlistResponse["tracks"]["items"];
		if (!items.is_null()) {
			playlist.name = Helpers::StrToWStr(playlistResponse["name"]);

			ParsePlaylist(playlist, items);

			json paginationURL = playlistResponse["tracks"]["next"];
			while (!paginationURL.is_null()) {
				cpr::Response res = cpr::Get(
					cpr::Url{ paginationURL },
					cpr::Header{ { "Authorization", "Bearer " + accessToken } });
				if (res.status_code != 200) {
					cvarManager->log("Bad response");
					if (doRetry) {
						RefreshAuthCode();
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

			if (downloadSongs) {
				std::thread t([&](std::deque<Song> songs) {
					DownloadFiles(songs);
				}, playlist.songs);
				t.detach();
			}
		}
		else {
			cvarManager->log("Playlist empty or not found");
		}
	}

	return playlist;
}

void SpotifyManager::RefreshAuthCode() {
	std::string accessToken = cvarManager->getCvar("RLS_SpotifyAccessToken").getStringValue();
	std::string refreshToken = cvarManager->getCvar("RLS_SpotifyRefreshToken").getStringValue();
	cvarManager->log("Old access: " + accessToken);
	cvarManager->log("Old refresh: " + refreshToken);

	cpr::Response res = cpr::Post(
		cpr::Url{ "https://accounts.spotify.com/api/token" },
		cpr::Payload{
			{ "grant_type", "refresh_token" },
			{ "refresh_token", refreshToken },
			{ "client_id", clientID }
		}
	);
	json refreshResponse;
	if (res.status_code == 200) {
		cvarManager->log(res.text);

		refreshResponse = json::parse(res.text);
		accessToken = refreshResponse["access_token"];
		refreshToken = refreshResponse["refresh_token"];
		cvarManager->getCvar("RLS_SpotifyAccessToken").setValue(accessToken);
		cvarManager->getCvar("RLS_SpotifyRefreshToken").setValue(refreshToken);
		cvarManager->getCvar("RLS_IsAuthenticatedSpotify").setValue(true);
		cvarManager->log("New access: " + accessToken);
		cvarManager->log("New refresh: " + refreshToken);
	}
	else {
		cvarManager->log(res.text);
		cvarManager->getCvar("RLS_IsAuthenticatedSpotify").setValue(false);
	}
}

void SpotifyManager::ExchangeCodeForAccess(std::string authCode) {
	std::string codeVerifier = cvarManager->getCvar("RLS_CodeVerifier").getStringValue();

	cpr::Response res = cpr::Post(
		cpr::Url{ "https://accounts.spotify.com/api/token" },
		cpr::Payload{
			{ "client_id", clientID },
			{ "grant_type", "authorization_code" },
			{ "code", authCode },
			{ "redirect_uri", "http://localhost" },
			{ "code_verifier", codeVerifier }
		}
	);
	json accessResponse;
	if (res.status_code == 200) {
		cvarManager->log(res.text);

		accessResponse = json::parse(res.text);
		std::string accessToken = accessResponse["access_token"];
		std::string refreshToken = accessResponse["refresh_token"];
		cvarManager->getCvar("RLS_SpotifyAccessToken").setValue(accessToken);
		cvarManager->getCvar("RLS_SpotifyRefreshToken").setValue(refreshToken);
		cvarManager->getCvar("RLS_IsAuthenticatedSpotify").setValue(true);
		cvarManager->log("New access: " + accessToken);
		cvarManager->log("New refresh: " + refreshToken);
	}
	else {
		cvarManager->log(res.text);
		cvarManager->getCvar("RLS_IsAuthenticatedSpotify").setValue(false);
	}
}


void SpotifyManager::StartSpotifyAuthFlow() {
	// https://developer.spotify.com/documentation/general/guides/authorization-guide/#authorization-code-flow-with-proof-key-for-code-exchange-pkce
	
	std::string codeVerifier = cvarManager->getCvar("RLS_CodeVerifier").getStringValue();
	std::string state = Helpers::RandomString(16);

	// https://www.cryptopp.com/wiki/Pipelining
	std::string codeChallenge;
	CryptoPP::SHA256 hash;
	CryptoPP::StringSource pipe(codeVerifier, true,
		new CryptoPP::HashFilter(hash,
			new CryptoPP::Base64URLEncoder(
				new CryptoPP::StringSink(codeChallenge))));

	std::string uri = "https://accounts.spotify.com/authorize";
	uri += "?response_type=code";
	uri += "&client_id=" + clientID;
	uri += "&redirect_uri=http://localhost";
	uri += "&scope=playlist-read-private";
	uri += "&state=" + state;
	uri += "&code_challenge=" + codeChallenge;
	uri += "&code_challenge_method=S256";
	ShellExecute(NULL, NULL, Helpers::StrToWStr(uri).c_str(), NULL, NULL, SW_SHOW);
}