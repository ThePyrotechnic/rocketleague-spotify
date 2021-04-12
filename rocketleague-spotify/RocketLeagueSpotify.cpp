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
// https://github.com/yhirose/cpp-httplib

void RocketLeagueSpotify::onLoad() {

	std::ifstream i("C:\\Users\\Alec\\Downloads\\rocketleague-spotify\\rocketleague-spotify\\config.json");
	std::ostringstream tmp;
	tmp << i.rdbuf();
	std::string s = tmp.str();
	cvarManager->log("TEST: " + s);
	//json config;
	//i >> config;
	//std::string spotifyCredential = config["spotifyId:SecretBase64"];
	//cvarManager->log("Spotify Credential: " + spotifyCredential);

	lastEventName = "None";
	lastStatPlayer = "None";
	lastStatVictim = "None";
	bInMenu = true;

	bEnabled = std::make_shared<bool>(false);

	cvarManager->registerCvar("RLS_Enabled", "1", "Enable Rocket League + Spotify integration", true, true, 0, true, 1).bindTo(bEnabled);
	
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.PostGoalScored.StartReplay", std::bind(&RocketLeagueSpotify::ReplayStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Replay_TA.StopPlayback", std::bind(&RocketLeagueSpotify::ReplayEnd, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));


	gameWrapper->RegisterDrawable(bind(&RocketLeagueSpotify::Render, this, std::placeholders::_1));
}

void RocketLeagueSpotify::onUnload() {}

void RocketLeagueSpotify::Render(CanvasWrapper canvas) {
	canvas.SetColor(0, 255, 0, 255);
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (server.IsNull()) { 
		canvas.SetPosition(Vector2{ 0, 0 });
		canvas.DrawString("In Menu", 3, 3);
		bInMenu = true;
		return;
	}

	//if (gameWrapper->GetbMetric() && server) {
	else {
		bInMenu = false;
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
	RocketLeagueSpotify::AuthenticateSpotify();

	// Sometime later
	//if (future_text.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
	//	cvarManager->log("GET RESULT");
	//}

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

	cvarManager->log(lastStatVictim);
}

void RocketLeagueSpotify::DownloadPreview(std::string previewUrl) {
	auto future_preview = cpr::GetCallback([&](cpr::Response res) {
		cvarManager->log(std::to_string(res.status_code));

		auto previewFile = std::fstream("preview.mp3", std::ios::out | std::ios::binary);
		const char* content = res.text.c_str();
		std::stringstream sstream(res.header["Content-Length"]);
		size_t size;
		sstream >> size;
		previewFile.write(content, size);
		previewFile.close();
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
		cpr::Url{ "https://api.spotify.com/v1/tracks/4tcPIwy0UvLYjhXLrMyx89" },
			cpr::Header{ { "Authorization", "Bearer " + spotifyToken } });
}

void RocketLeagueSpotify::AuthenticateSpotify() {
	auto future_token = cpr::PostCallback([&](cpr::Response res) {
		cvarManager->log("AUTH");
		cvarManager->log(std::to_string(res.status_code));

		auto authResponse = json::parse(res.text);
		std::string token = authResponse["access_token"];
		cvarManager->log(token);
		spotifyToken = token;

		return token;
		},
		cpr::Url{ "https://accounts.spotify.com/api/token" },
			cpr::Header{ { "Authorization", "Basic " + spotifyCredential } },
			cpr::Parameters{ { "grant_type", "client_credentials" } });
}