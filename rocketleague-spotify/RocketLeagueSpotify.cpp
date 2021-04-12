#include "stdafx.h"

#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

#include "Audio/AudioManager.h"
#include "RocketLeagueSpotify.h"


BAKKESMOD_PLUGIN(RocketLeagueSpotify, "Rocket League + Spotify", "1.0.0", PLUGINTYPE_FREEPLAY);

// https://github.com/bakkesmodorg/BakkesMod2-Plugins
// https://github.com/bakkesmodorg/BakkesModSDK
// https://github.com/CinderBlocc/GoalSpeedAnywhere


void RocketLeagueSpotify::onLoad() {
	bInMenu = true;

	cvarManager->registerCvar("RLS_Master", "50", "Master volume", true, true, 0, true, 100);

	cvarManager->getCvar("RLS_Master").addOnValueChanged(std::bind(&RocketLeagueSpotify::SetMasterVolume, this, std::placeholders::_1, std::placeholders::_2));
	
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
	assetDir = std::wstring(&appdata[0], &appdata[len - 1]) + LR"(\bakkesmod\bakkesmod\rocketleague-spotify\assets)";
}

void RocketLeagueSpotify::onUnload() {
	BASS_Free();
}

void RocketLeagueSpotify::SetMasterVolume(std::string oldValue, CVarWrapper cvar) {
	audioManager.SetMasterVolume(cvar.getIntValue());
}

void RocketLeagueSpotify::Render(CanvasWrapper canvas) {
	if (gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame()) {
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
}

void RocketLeagueSpotify::ReplayEnd(std::string eventName) {
	audioManager.PlaySoundFromFile(assetDir + LR"(\audio\uuhhh.wav)");
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
	TickerStruct* tArgs = (TickerStruct*)args;

	PriWrapper victim = PriWrapper(tArgs->Victim);
	if (victim) {
		if (PriWrapper(tArgs->Victim).GetUniqueIdWrapper().GetIdString() == playerIDString)
			audioManager.PlaySoundFromFile(assetDir + LR"(\audio\uuhhh.wav)");
	}

}