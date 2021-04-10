#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

#include "RocketLeagueSpotify.h"


BAKKESMOD_PLUGIN(RocketLeagueSpotify, "Rocket League + Spotify", "1.0.0", PLUGINTYPE_FREEPLAY);

// https://github.com/bakkesmodorg/BakkesMod2-Plugins
// https://github.com/bakkesmodorg/BakkesModSDK
// https://github.com/CinderBlocc/GoalSpeedAnywhere


void RocketLeagueSpotify::onLoad() {

	lastEventName = "None";
	lastStatPlayer = "None";
	lastStatVictim = "None";

	bEnabled = std::make_shared<bool>(false);

	cvarManager->registerCvar("RLS_Enabled", "1", "Enable Rocket League + Spotify integration", true, true, 0, true, 1).bindTo(bEnabled);
	
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.PostGoalScored.StartReplay", std::bind(&RocketLeagueSpotify::ReplayStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Replay_TA.StopPlayback", std::bind(&RocketLeagueSpotify::ReplayEnd, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));


	gameWrapper->RegisterDrawable(bind(&RocketLeagueSpotify::Render, this, std::placeholders::_1));
}

void RocketLeagueSpotify::onUnload() {}

void RocketLeagueSpotify::Render(CanvasWrapper canvas) {
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (server.IsNull()) return;

	if (gameWrapper->GetbMetric() && server) {
		canvas.SetPosition(Vector2{ 0, 0 });
		canvas.DrawString(lastEventName, 3, 3);
		canvas.SetPosition(Vector2{ 0, 100 });
		canvas.DrawString(lastStatPlayer + ": " + lastStatName + " | Victim: " + lastStatVictim, 3, 3);
	}
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
}

void RocketLeagueSpotify::ReplayEnd(std::string eventName) {
	lastEventName = eventName;
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

	lastStatPlayer = PriWrapper(tArgs->Receiver).GetPlayerName().ToString();
	lastStatName = StatEventWrapper(tArgs->StatEvent).GetLabel().ToString();

	auto victim = PriWrapper(tArgs->Victim);
	if (victim) lastStatVictim = PriWrapper(tArgs->Victim).GetPlayerName().ToString();
	else lastStatVictim = "None";
}