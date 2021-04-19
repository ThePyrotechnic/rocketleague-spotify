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
// togglemenu devtools for events

void RocketLeagueSpotify::onLoad() {
	bInMenu = true;
	fadeInTimeCVar = std::make_shared<int>(6);
	fadeOutTimeCVar = std::make_shared<int>(64);
	goalPlaylistCVar = std::make_shared <std::string>("1FLmlXw521Ap4RT1chb1wi");

	// Joining any game: "Function Engine.PlayerInput.InitInputSystem"
	// Game is starting: "Function OnlineGameJoinGame_X.WaitForAllPlayers.BeginState"
	// Game ended: "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"
	// Function TAGame.GameEvent_TA.EventPlayerAdded
	// Function TAGame.GameEvent_TA.EventPlayerRemoved

	cvarManager->registerCvar("RLS_Master", "25", "Master volume", true, true, 0, true, 100);
	cvarManager->registerCvar("RLS_FadeInTime", "6", "Fade-in duration", true, true, 1, true, 10000).bindTo(fadeInTimeCVar);
	cvarManager->registerCvar("RLS_FadeOutTime", "64", "Fade-out duration", true, true, 1, true, 10000).bindTo(fadeOutTimeCVar);
	cvarManager->registerCvar("RLS_GoalPlaylistStatus", "Initializing ...", "Goal playlist status");
	cvarManager->registerCvar("RLS_GoalPlaylist", "1FLmlXw521Ap4RT1chb1wi", "Goal Playlist").bindTo(goalPlaylistCVar);
	cvarManager->registerCvar("RLS_WebEndpoint", "http://localhost:8000", "Web API endpoint");

	cvarManager->getCvar("RLS_Master").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarMasterVolume, this, std::placeholders::_1, std::placeholders::_2));
	cvarManager->getCvar("RLS_GoalPlaylist").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarGoalPlaylist, this, std::placeholders::_1, std::placeholders::_2));
	
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.InitGame", std::bind(&RocketLeagueSpotify::BeginWaitingForPlayersEvent, this, std::placeholders::_1));
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	//gameWrapper->HookEventPost(" Function TAGame.GameEvent_TA.EventPlayerAdded", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.EventPlayerRemoved", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&RocketLeagueSpotify::MatchEnded, this, std::placeholders::_1));
	gameWrapper->HookEventPost("Function GameEvent_TA.Countdown.BeginState", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.PostGoalScored.StartReplay", std::bind(&RocketLeagueSpotify::ReplayStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Replay_TA.StopPlayback", std::bind(&RocketLeagueSpotify::ReplayEnd, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));

	gameWrapper->RegisterDrawable(bind(&RocketLeagueSpotify::Render, this, std::placeholders::_1));

	playerID = new UniqueIDWrapper(gameWrapper->GetUniqueID());
	playerIDString = playerID->GetIdString();
	cvarManager->log(L"ERROR: " + std::to_wstring(BASS_ErrorGetCode()));

	wchar_t* appdata;
	size_t len;
	_wdupenv_s(&appdata, &len, L"APPDATA");
	modDir = gameWrapper->GetBakkesModPathW() + LR"(\rocketleague-spotify)";
	audioDir = modDir + LR"(\assets\audio\)";

	spotifyManager = SpotifyManager(cvarManager, modDir, audioDir);
	
	CVarGoalPlaylist("", cvarManager->getCvar("RLS_GoalPlaylist"));  // Initialize playlist

	Tick();
}

void RocketLeagueSpotify::onUnload() {
	BASS_Free();
}

void RocketLeagueSpotify::Tick() {
	deltaTime = 0.01667;  // ~60 fps
	if (gameWrapper->GetOnlineGame().IsNull()) { seed = 0; connectedPlayers.clear();  }

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

void RocketLeagueSpotify::CheckPlayers(std::string e) {
	cvarManager->log("In event: " + e);
	
	if (seed != 0) { cvarManager->log("Match already started");  return; }

	gameWrapper->SetTimeout([this](GameWrapper* gw)
	{
		if (gameWrapper->GetOnlineGame().IsNull()) { cvarManager->log("Game is null"); return; }

		std::string guid = gameWrapper->GetOnlineGame().GetMatchGUID();
		const char* guid_c = guid.c_str();
		for (int a = 0; a < guid.length(); a++) {
			seed += guid_c[a];
		}
		//seed = sscanf_s(, "%u", &seed);
		cvarManager->log("Set seed: " + std::to_string(seed));

		ArrayWrapper<PriWrapper> players = gameWrapper->GetOnlineGame().GetPRIs();
		std::string commaSeparatedIDs;
		cvarManager->log("Player count: " + std::to_string(players.Count()));
		for (int a = 0; a < players.Count() - 1; a++) {
			PriWrapper p = players.Get(a);
			if (!p.IsNull()) {
				UniqueIDWrapper id = p.GetUniqueIdWrapper();
				std::string currentID = id.GetIdString();
				cvarManager->log(currentID + ": " + p.GetPlayerName().ToString());
				commaSeparatedIDs += currentID + ",";
			}
		}
		PriWrapper p = players.Get(players.Count() - 1);
		if (!p.IsNull()) {
			commaSeparatedIDs += p.GetUniqueIdWrapper().GetIdString();
		}
		cvarManager->log("Retrieving data for " + commaSeparatedIDs);

		std::string endpoint = cvarManager->getCvar("RLS_WebEndpoint").getStringValue() + "/users/";
		cpr::Response res = cpr::Get(
			cpr::Url{ endpoint },
			cpr::Parameters{ { "user_ids", commaSeparatedIDs } }
		);
		cvarManager->log(res.text);

		if (res.status_code != 200) {
			if (res.status_code == 404) cvarManager->log("No registered users out of: " + commaSeparatedIDs);
			else cvarManager->log("API Error: " + std::to_string(res.status_code));
			return;
		}

		json data = json::parse(res.text, nullptr, false);
		if (data.is_discarded()) {
			cvarManager->log("Invalid json from response");
			return;
		}

		for (int a = 0; a < data.size(); a++) {
			cvarManager->log(data[a].dump());
			AddPlaylistForID(data[a]["id"], data[a]["goal_music_uri"]);
			connectedPlayers.push_back(data[a]["id"]);
		}
		std::sort(connectedPlayers.begin(), connectedPlayers.end());
		auto rng = std::default_random_engine(seed);
		for (std::string id : connectedPlayers) {
			std::shuffle(songPaths[id].begin(), songPaths[id].end(), rng);
		}
	}, .05f);
}

void RocketLeagueSpotify::MatchEnded(std::string e) {
	seed = 0;
	connectedPlayers.clear();
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

void RocketLeagueSpotify::CVarGoalPlaylist(std::string oldValue, CVarWrapper cvar) {
	AddPlaylistForID(playerIDString, cvar.getStringValue());
}

void RocketLeagueSpotify::AddPlaylistForID(std::string IDString, std::string playlistID) {
	if (playlistID.length() != 22) {
		cvarManager->getCvar("RLS_GoalPlaylistStatus").setValue("Playlist ID must be 22 characters");
		return;
	}
	bool isPlayer = false;
	if (IDString._Equal(playerIDString)) isPlayer = true;

	if (isPlayer) cvarManager->getCvar("RLS_GoalPlaylistStatus").setValue("Downloading playlist ...");
	std::vector<std::wstring> filePaths = spotifyManager.DownloadPlaylist(playlistID);
	if (isPlayer) cvarManager->getCvar("RLS_GoalPlaylistStatus").setValue("Downloaded " + std::to_string(filePaths.size()) + " songs!");

	if (filePaths.size() == 0) {
		cvarManager->log("New playlist is empty. Not overriding current playlist");
	}

	songPaths[IDString].clear();
	for (std::wstring filePath : filePaths) {
		songPaths[IDString].push_back(filePath);
	}

	if (!isPlayer || gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame()) {
		cvarManager->log("Won't update playlist in online game");
		return;
	}
	json data = {
		{"id", playerIDString},
		{"goal_music_uri", playlistID}
	};

	cpr::Response res = cpr::Put(
		cpr::Url{ cvarManager->getCvar("RLS_WebEndpoint").getStringValue() + "/users/" },
		cpr::Header{ { "Content-Type", "application/json" } },
		cpr::Body{ data.dump() });
}

void RocketLeagueSpotify::Render(CanvasWrapper canvas) {}

struct TickerStruct {
	// person who got a stat
	uintptr_t Receiver;
	// person who is victim of a stat (only exists for demos afaik)
	uintptr_t Victim;
	// wrapper for the stat event
	uintptr_t StatEvent;
	
};

void RocketLeagueSpotify::ReplayStart(std::string eventName) {
	if (songPaths[lastScorerId].size() == 0) { cvarManager->log("Empty playlist for " + lastScorerId); return; }

	std::wstring nextSong = songPaths[lastScorerId].front();

	FadeIn(*fadeInTimeCVar);
	cvarManager->log(L": " + nextSong);
	HSTREAM s = audioManager.PlaySoundFromFile(nextSong);
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
	TickerStruct* tArgs = (TickerStruct*)args;
	std::string statName = StatEventWrapper(tArgs->StatEvent).GetLabel().ToString();

	if (statName._Equal("Goal")) {
		PriWrapper receiver = PriWrapper(tArgs->Receiver);
		UniqueIDWrapper receiverId = receiver.GetUniqueIdWrapper();
		lastScorerId = receiverId.GetIdString();

		std::wstring nextSong = songPaths[lastScorerId].back();
		songPaths[lastScorerId].push_front(nextSong);
		songPaths[lastScorerId].pop_back();

		cvarManager->log(receiver.GetPlayerName().ToString() + " scored (" + receiverId.GetIdString() + ": " + statName + ")");
	}
}
