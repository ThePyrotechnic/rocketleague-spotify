#include "stdafx.h"
#include "Audio/AudioManager.h"

#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include "RocketLeagueSpotify.h"

using json = nlohmann::json;

BAKKESMOD_PLUGIN(RocketLeagueSpotify, "Rocket League + Spotify", "1.0.0", PLUGINTYPE_FREEPLAY);

// https://github.com/bakkesmodorg/BakkesMod2-Plugins
// https://github.com/bakkesmodorg/BakkesModSDK
// https://github.com/CinderBlocc/GoalSpeedAnywhere
// https://github.com/whoshuu/cpr
// https://github.com/nlohmann/json
// cl_settings_refreshplugins when making changes to the RocketLeagueSpotify.set file

void RocketLeagueSpotify::onLoad() {
	bInMenu = true;
	fadeInTimeCVar = std::make_shared<int>(6);
	fadeOutTimeCVar = std::make_shared<int>(64);
	goalSongId = "";
	goalSongFilePath = L"";

	cvarManager->registerCvar("RLS_Master", "50", "Master volume", true, true, 0, true, 100);
	cvarManager->registerCvar("RLS_FadeInTime", "6", "Fade-in duration in seconds", true, true, 1, true, 10000).bindTo(fadeInTimeCVar);
	cvarManager->registerCvar("RLS_FadeOutTime", "64", "Fade-out duration in seconds", true, true, 1, true, 10000).bindTo(fadeOutTimeCVar);
	cvarManager->registerCvar("RLS_GoalSong", "4tcPIwy0UvLYjhXLrMyx89", "Goal song Spotify ID");
	cvarManager->registerCvar("RLS_GoalSongStatus", "Initializing ...", "Goal song status");

	cvarManager->getCvar("RLS_Master").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarMasterVolume, this, std::placeholders::_1, std::placeholders::_2));
	cvarManager->getCvar("RLS_GoalSong").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarGoalSong, this, std::placeholders::_1, std::placeholders::_2));
	
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.PostGoalScored.StartReplay", std::bind(&RocketLeagueSpotify::ReplayStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Replay_TA.StopPlayback", std::bind(&RocketLeagueSpotify::ReplayEnd, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));

	gameWrapper->RegisterDrawable(bind(&RocketLeagueSpotify::Render, this, std::placeholders::_1));

	playerID = new UniqueIDWrapper(gameWrapper->GetUniqueID());
	playerIDString = playerID->GetIdString();
	cvarManager->log(L"ERROR: " + std::to_wstring(BASS_ErrorGetCode()));

	wchar_t* appdata;
	size_t len;
	_wdupenv_s(&appdata, &len, L"APPDATA");
	modDir = std::wstring(&appdata[0], &appdata[len - 1]) + LR"(\bakkesmod\bakkesmod\rocketleague-spotify)";

	std::ifstream i(modDir + LR"(\config.json)");
	//std::ostringstream tmp;
	//tmp << i.rdbuf();
	//std::string s = tmp.str();
	//cvarManager->log("TEST: " + s);
	json config;
	i >> config;
	spotifyCredential = config["spotifyId:SecretBase64"];
	cvarManager->log("Spotify Credential: " + spotifyCredential);
	RocketLeagueSpotify::AuthenticateSpotify();

	for (std::string songId : randomSongs) {
		RocketLeagueSpotify::DownloadSong(songId);
		std::wstring wSongId = std::wstring(songId.length(), L' ');
		std::copy(songId.begin(), songId.end(), wSongId.begin());
		std::wstring filePath = modDir + LR"(\assets\audio\)" + wSongId + L".mp3";
		songPaths.push_back(filePath);
	}
	//RocketLeagueSpotify::DownloadSong(cvarManager->getCvar("RLS_GoalSong").getStringValue());

	Tick();
	
	//cvarManager->registerNotifier("play_song", [this](std::vector<std::string> params) {
	//	audioManager.PlaySoundFromFile(modDir + LR"(\assets\audio\preview.mp3)");
	//}, "Plays a song", PERMISSION_ALL);

	//cvarManager->registerNotifier("fade_in", [this](std::vector<std::string> params) {
	//	FadeIn(*fadeInTimeCVar); 
	//}, "Fades in the audio", PERMISSION_ALL);

	//cvarManager->registerNotifier("fade_out", [this](std::vector<std::string> params) {
	//	FadeOut(*fadeOutTimeCVar);
	//}, "Fades out the audio", PERMISSION_ALL);

}

void RocketLeagueSpotify::onUnload() {
	BASS_Free();
}

void RocketLeagueSpotify::Tick() {
	deltaTime = 0.01667;  // ~60 fps

	gameWrapper->SetTimeout([this](GameWrapper* gw) {
		this->Tick();
	}, deltaTime);

	if (fadeDuration == 0) return;

	timeSinceFade += deltaTime;

	double fadeProgress = timeSinceFade / fadeDuration;

	if (fadeProgress < 0.9) {
		double newVolume = (audioManager.GetMasterVolume() * (1.f - (fadeProgress))) + (fadeTarget * (fadeProgress));
		//cvarManager->log("(" + std::to_string(audioManager.GetMasterVolume()) + " * (1 - " + std::to_string(fadeProgress) + ")) + (" + std::to_string(fadeTarget) + " * " + std::to_string(fadeProgress) + ") = " + std::to_string(newVolume));
		audioManager.SetMasterVolume(newVolume);
	}
	else {
		fadeDuration = 0;
	}
}

void RocketLeagueSpotify::FadeMasterVolume(int target, int timeToFade) {
	timeSinceFade = 0.f;
	fadeTarget = target;
	fadeDuration = timeToFade;
}

void RocketLeagueSpotify::FadeIn(int timeToFade) {
	audioManager.SetMasterVolume(0);
	FadeMasterVolume(cvarManager->getCvar("RLS_Master").getIntValue(), timeToFade);
}

void RocketLeagueSpotify::FadeOut(int timeToFade) {
	FadeMasterVolume(0, timeToFade);
}

void RocketLeagueSpotify::CVarMasterVolume(std::string oldValue, CVarWrapper cvar) {
	audioManager.SetMasterVolume(cvar.getIntValue());
}

void RocketLeagueSpotify::CVarGoalSong(std::string oldValue, CVarWrapper cvar) {
	std::string newSong = cvar.getStringValue();

	if (newSong.empty()) return;

	DownloadSong(newSong);
}

void RocketLeagueSpotify::Render(CanvasWrapper canvas) {
	if (gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame()) {
		bInMenu = false;
	}
	else {
		bInMenu = true;
	};
}

struct TickerStruct {
	// person who got a stat
	uintptr_t Receiver;
	// person who is victim of a stat (only exists for demos afaik)
	uintptr_t Victim;
	// wrapper for the stat event
	uintptr_t StatEvent;
	
};

void RocketLeagueSpotify::ReplayStart(std::string eventName) {
	if (goalSongFilePath.empty()) return;

	int randomIndex = rand() % songPaths.size();

	FadeIn(*fadeInTimeCVar);
	HSTREAM s = audioManager.PlaySoundFromFile(songPaths[randomIndex]);
	replaySounds.push_back(s);
}

void RocketLeagueSpotify::ReplayEnd(std::string eventName) {
	FadeOut(*fadeOutTimeCVar);
	gameWrapper->SetTimeout([this](GameWrapper* gw) {
		for (HSTREAM s : replaySounds) {
			audioManager.StopSound(s);
		}
		replaySounds.clear();
	}, 5);
}

void RocketLeagueSpotify::HandleStatEvent(ServerWrapper caller, void* args) {

}

void RocketLeagueSpotify::DownloadPreview(std::string songId, std::string previewUrl) {
	auto future_preview = cpr::GetCallback([&](cpr::Response res) {
		cvarManager->log(std::to_string(res.status_code));

		std::wstring wSongId = std::wstring(songId.length(), L' ');
		std::copy(songId.begin(), songId.end(), wSongId.begin());
		std::wstring filePath = modDir + LR"(\assets\audio\)" + wSongId + L".mp3";
		std::fstream previewFile = std::fstream(filePath, std::ios::out | std::ios::binary);
		const char* content = res.text.c_str();
		std::stringstream sstream(res.header["Content-Length"]);
		size_t size;
		sstream >> size;
		previewFile.write(content, size);
		previewFile.close();
		cvarManager->log("Downloaded song");
		cvarManager->getCvar("RLS_GoalSongStatus").setValue("Downloaded!");
		goalSongFilePath = filePath;
		}, cpr::Url{ previewUrl });
}

void RocketLeagueSpotify::DownloadSong(std::string songId) {
	if (songId.empty()) {
		cvarManager->getCvar("RLS_GoalSongStatus").setValue("Invalid ID");
		return;
	}

	auto future_track = cpr::GetCallback([&](cpr::Response res) {
		cvarManager->log("getting song preview url");
		cvarManager->getCvar("RLS_GoalSongStatus").setValue("Downloading ...");
		cvarManager->log(std::to_string(res.status_code));
		if (res.status_code != 200) {
			cvarManager->log("Bad response");
			cvarManager->getCvar("RLS_GoalSongStatus").setValue("Invalid ID. Error: " + std::to_string(res.status_code));
			return std::string("");
		}
		auto trackResponse = json::parse(res.text);
		std::string previewUrl;
		if (!trackResponse["preview_url"].is_null()) {
			previewUrl = trackResponse["preview_url"];
			cvarManager->log("Got preview URL: " + previewUrl);
			RocketLeagueSpotify::DownloadPreview(songId, previewUrl);
		}
		else {
			cvarManager->getCvar("RLS_GoalSongStatus").setValue("No preview available :(");
			cvarManager->log("No preview URL available");
		}
		return previewUrl;
		},
		cpr::Url{ "https://api.spotify.com/v1/tracks/" + songId },
			cpr::Header{ { "Authorization", "Bearer " + spotifyToken } });
}

void RocketLeagueSpotify::AuthenticateSpotify() {
	auto future_token = cpr::PostCallback([&](cpr::Response res) {
		cvarManager->log(spotifyCredential);
		cvarManager->log("AUTH");
		cvarManager->log(std::to_string(res.status_code));
		cvarManager->log(res.text);
		auto authResponse = json::parse(res.text);
		std::string token = authResponse["access_token"];
		cvarManager->log(token);
		spotifyToken = token;

		return token;
		},
		cpr::Url{ "https://accounts.spotify.com/api/token" },
			cpr::Header{ { "Authorization", "Basic " + spotifyCredential} },
			cpr::Parameters{ { "grant_type", "client_credentials" } });
	// Sometime later
	//if (future_text.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
	//	cvarManager->log("GET RESULT");
	//}
}