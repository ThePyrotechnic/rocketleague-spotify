#include "stdafx.h"

#include "Audio/AudioManager.h"
#include "Spotify/SpotifyManager.h"

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

RocketLeagueSpotify::RocketLeagueSpotify() {
	spotifyManager = SpotifyManager();
}

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

	spotifyManager = SpotifyManager(cvarManager, modDir);

	
	//std::ostringstream tmp;
	//tmp << i.rdbuf();
	//std::string s = tmp.str();
	//cvarManager->log("TEST: " + s);
	std::string playlistId = "1FLmlXw521Ap4RT1chb1wi";
	std::vector<std::wstring> filePaths = spotifyManager.DownloadPlaylist(playlistId);
	for (std::wstring filePath : filePaths) {
		songPaths.push_back(filePath);
	}

	//for (std::string songId : randomSongs) {
	//	spotifyManager.DownloadSong(songId);
	//	std::wstring wSongId = std::wstring(songId.length(), L' ');
	//	std::copy(songId.begin(), songId.end(), wSongId.begin());
	//	std::wstring filePath = modDir + LR"(\assets\audio\)" + wSongId + L".mp3";
	//	songPaths.push_back(filePath);
	//}
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

	spotifyManager.DownloadSong(newSong);
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
	//if (goalSongFilePath.empty()) return;
	// if Spotify.getGoalPath is empty

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
