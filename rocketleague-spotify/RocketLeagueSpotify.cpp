#include "bakkesmod/wrappers/includes.h"

#include "RocketLeagueSpotify.h"


BAKKESMOD_PLUGIN(RocketLeagueSpotify, "Rocket League + Spotify", "1.0.0", PLUGINTYPE_FREEPLAY);


void RocketLeagueSpotify::onLoad() {
	// Game start		Function TAGame.Goal_TA.BeginPlay
	// Game end
	// Replay start
	// Replay end
	// Blown up			Function TAGame.Car_TA.Demolish
	// Respawned
	// Ball enters goal	Function TAGame.Ball_TA.OnHitGoal
	// Player scores	Function TAGame.PRI_TA.EventScoredGoal

	lastEventName = "None";

	bEnabled = std::make_shared<bool>(false);

	cvarManager->registerCvar("RLS_Enabled", "1", "Enable Rocket League + Spotify integration", true, true, 0, true, 1).bindTo(bEnabled);

	//gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal", std::bind(&RocketLeagueSpotify::PlaySound, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Car_TA.OnJumpPressed", std::bind(&RocketLeagueSpotify::PlaySound, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Car_TA.Demolish", std::bind(&RocketLeagueSpotify::PlaySound, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Car_TA.Demolish2", std::bind(&RocketLeagueSpotify::PlaySound, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.PRI_TA.EventScoredGoal", std::bind(&RocketLeagueSpotify::PlaySound, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.OnCarSpawned", std::bind(&RocketLeagueSpotify::PlaySound, this, std::placeholders::_1));
	
	gameWrapper->RegisterDrawable(bind(&RocketLeagueSpotify::Render, this, std::placeholders::_1));
}

void RocketLeagueSpotify::onUnload() {}

void RocketLeagueSpotify::Render(CanvasWrapper canvas) {
	if (gameWrapper->GetbMetric()) {
		canvas.DrawString(lastEventName, 3, 3);
	}
}

void RocketLeagueSpotify::PlaySound(std::string eventName) {
	lastEventName = eventName;
}