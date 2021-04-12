#include "stdafx.h"
#include "Audio/AudioManager.h"
#include <fstream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include "bakkesmod/wrappers/http/HttpWrapper.h"
#include "bakkesmod/core/http_structs.h"
#include "RocketLeagueSpotify.h"

using json = nlohmann::json;

BAKKESMOD_PLUGIN(RocketLeagueSpotify, "Rocket League + Spotify", "1.0.0", PLUGINTYPE_FREEPLAY);

// https://github.com/bakkesmodorg/BakkesMod2-Plugins
// https://github.com/bakkesmodorg/BakkesModSDK
// https://github.com/CinderBlocc/GoalSpeedAnywhere
// https://github.com/whoshuu/cpr
// https://github.com/nlohmann/json

void RocketLeagueSpotify::onLoad() {
	bInMenu = true;

	cvarManager->registerCvar("RLS_Master", "50", "Master volume", true, true, 0, true, 100);

	cvarManager->getCvar("RLS_Master").addOnValueChanged(std::bind(&RocketLeagueSpotify::SetMasterVolume, this, std::placeholders::_1, std::placeholders::_2));
	
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.PostGoalScored.StartReplay", std::bind(&RocketLeagueSpotify::ReplayStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Replay_TA.StopPlayback", std::bind(&RocketLeagueSpotify::ReplayEnd, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));
	gameWrapper->HookEvent("Function Engine.Interaction.Tick", std::bind(&RocketLeagueSpotify::Tick, this));

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
}

void RocketLeagueSpotify::onUnload() {
	BASS_Free();
}

void RocketLeagueSpotify::Tick() {
	if (bInMenu || fadeDuration == 0) return;
	timeSinceFade += gameWrapper->GetOnlineGame().GetTimeSinceLastTick();
	float fadeProgress = timeSinceFade/fadeDuration;
	audioManager.SetMasterVolume((audioManager.GetMasterVolume() * (1.0 - fadeProgress)) + (fadeTarget * fadeProgress));
}

void RocketLeagueSpotify::FadeMasterVolume(int target, int timeToFade) {
	timeSinceFade = 0;
	fadeTarget = target;
	fadeDuration = fadeDuration;
}

void RocketLeagueSpotify::SetMasterVolume(std::string oldValue, CVarWrapper cvar) {
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
	lastEventName = eventName;
	audioManager.PlaySoundFromFile(modDir + LR"(\assets\audio\preview.mp3)");
}

void RocketLeagueSpotify::ReplayEnd(std::string eventName) {
	audioManager.PlaySoundFromFile(modDir + LR"(\assets\audio\uuhhh.wav)");
}

//	"Demolition"
//	"Extermination"
//	"Goal"
//	"Win"
//	"MVP"
//	"Aerial Goal"
//	"Backwards Goal"
//	"Bicycle Goal"
//	"Long Goal"
//	"Turtle Goal"
//	"Pool Shot"
//	"Overtime Goal"
//	"Hat Trick"
//	"Assist"
//	"Playmaker"
//	"Save"
//	"Epic Save"
//	"Savior"
//	"Shot on Goal"
//	"Center Ball"
//	"Clear Ball"
//	"First Touch"
//	"Damage"
//	"Ultra Damage"
//	"Low Five"
//	"High Five"
//	"Swish Goal"
//	"Bicycle Hit"

void RocketLeagueSpotify::HandleStatEvent(ServerWrapper caller, void* args) {

}

void RocketLeagueSpotify::DownloadPreview(std::string previewUrl) {
	auto future_preview = cpr::GetCallback([&](cpr::Response res) {
		cvarManager->log(std::to_string(res.status_code));

		auto previewFile = std::fstream(modDir + LR"(\assets\audio\preview.mp3)", std::ios::out | std::ios::binary);
		const char* content = res.text.c_str();
		std::stringstream sstream(res.header["Content-Length"]);
		size_t size;
		sstream >> size;
		previewFile.write(content, size);
		previewFile.close();
		cvarManager->log("Downloaded song");
		}, cpr::Url{ previewUrl });
}

void RocketLeagueSpotify::DownloadSong(std::string songId) {
	auto future_track = cpr::GetCallback([&](cpr::Response res) {
		cvarManager->log("getting song preview url");
		cvarManager->log(std::to_string(res.status_code));

		auto trackResponse = json::parse(res.text);
		std::string previewUrl = trackResponse["preview_url"];
		if (!previewUrl.empty()) {
			cvarManager->log(previewUrl);
			RocketLeagueSpotify::DownloadPreview(previewUrl);
		}
		else {
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

		RocketLeagueSpotify::DownloadSong("4tcPIwy0UvLYjhXLrMyx89");

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