#include "stdafx.h"

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

//RocketLeagueSpotify::RocketLeagueSpotify() {
//	spotifyManager = SpotifyManager();
//}

void RocketLeagueSpotify::onLoad() {
	bInMenu = true;
	fadeInTimeCVar = std::make_shared<int>(6);
	fadeOutTimeCVar = std::make_shared<int>(64);
	goalPlaylistCVar = std::make_shared <std::string>("1FLmlXw521Ap4RT1chb1wi");
	goalSongId = "";
	goalSongFilePath = L"";

	// Joining any game: "Function Engine.PlayerInput.InitInputSystem"
	// Game is starting: "Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState"
	// Game ended: "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"

	cvarManager->registerCvar("RLS_Master", "25", "Master volume", true, true, 0, true, 100);
	cvarManager->registerCvar("RLS_FadeInTime", "6", "Fade-in duration in seconds", true, true, 1, true, 10000).bindTo(fadeInTimeCVar);
	cvarManager->registerCvar("RLS_FadeOutTime", "64", "Fade-out duration in seconds", true, true, 1, true, 10000).bindTo(fadeOutTimeCVar);
	cvarManager->registerCvar("RLS_GoalSong", "4tcPIwy0UvLYjhXLrMyx89", "Goal song Spotify ID (unused)");
	cvarManager->registerCvar("RLS_GoalSongStatus", "Initializing ...", "Goal song status");
	cvarManager->registerCvar("RLS_GoalPlaylist", "1FLmlXw521Ap4RT1chb1wi", "Goal Playlist").bindTo(goalPlaylistCVar);

	cvarManager->getCvar("RLS_Master").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarMasterVolume, this, std::placeholders::_1, std::placeholders::_2));

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
	audioDir = modDir + LR"(\assets\audio\)";

	spotifyManager = SpotifyManager(cvarManager, modDir, audioDir);

	//std::ostringstream tmp;
	//tmp << i.rdbuf();
	//std::string s = tmp.str();
	//cvarManager->log("TEST: " + s);
	cvarManager->getCvar("RLS_GoalSongStatus").setValue("Downloading playlist ...");
	SpotifyPlaylist playlist = spotifyManager.GetPlaylist(*goalPlaylistCVar);
	cvarManager->getCvar("RLS_GoalSongStatus").setValue("Downloaded playlist: " + playlist.id);
	cvarManager->getCvar("RLS_GoalSongStatus").setValue("Downloaded " + std::to_string(playlist.songs.size()) + " songs!");
	playlists[*goalPlaylistCVar] = playlist;
	
	Tick();
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

std::wstring RocketLeagueSpotify::StrToWStr(std::string str) {
	std::wstring wstr = std::wstring(str.length(), L' ');
	std::copy(str.begin(), str.end(), wstr.begin());
	
	return wstr;
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
	cvarManager->log("here");
	int index = playlists[*goalPlaylistCVar].nextIndex();
	cvarManager->log(L"playing: " + playlists[*goalPlaylistCVar].songs[index].name);

	FadeIn(*fadeInTimeCVar);
	cvarManager->log(std::to_wstring(index) + playlists[*goalPlaylistCVar].songs[index].path);
	HSTREAM s = audioManager.PlaySoundFromFile(playlists[*goalPlaylistCVar].songs[index].path);
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
