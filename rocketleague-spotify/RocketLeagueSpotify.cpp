#include "stdafx.h"

#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include "bakkesmod/wrappers/GuiManagerWrapper.h"
#include "RocketLeagueSpotify.h"


using json = nlohmann::json;

BAKKESMOD_PLUGIN(RocketLeagueSpotify, "Rocket League + Spotify", "1.0.0", PLUGINTYPE_FREEPLAY);

// https://github.com/bakkesmodorg/BakkesMod2-Plugins
// https://github.com/bakkesmodorg/BakkesModSDK
// https://github.com/CinderBlocc/GoalSpeedAnywhere
// https://github.com/whoshuu/cpr
// https://github.com/nlohmann/json
// cl_settings_refreshplugins when making changes to the RocketLeagueSpotify.set file
// togglemenu devtools for events\
// https://bakkesmod.fandom.com/wiki/Plugin_settings_files

void RocketLeagueSpotify::onLoad() {
	fadeInTimeCVar = std::make_shared<int>(6);
	fadeOutTimeCVar = std::make_shared<int>(64);
	goalPlaylistCVar = std::make_shared <std::string>("7iVjuGSG2HPjvbJjFmKIaL");

	// Joining any game: "Function Engine.PlayerInput.InitInputSystem"
	// Game is starting: "Function OnlineGameJoinGame_X.WaitForAllPlayers.BeginState"
	// Game ended: "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"
	// Function TAGame.GameEvent_TA.EventPlayerAdded
	// Function TAGame.GameEvent_TA.EventPlayerRemoved

	cvarManager->registerCvar("RLS_Master", "25", "Master volume", true, true, 0, true, 100);
	cvarManager->registerCvar("RLS_FadeInTime", "6", "Fade-in duration", true, true, 1, true, 10000).bindTo(fadeInTimeCVar);
	cvarManager->registerCvar("RLS_FadeOutTime", "64", "Fade-out duration", true, true, 1, true, 10000).bindTo(fadeOutTimeCVar);
	cvarManager->registerCvar("RLS_GoalPlaylistStatus", "Initializing ...", "Goal playlist status");
	cvarManager->registerCvar("RLS_GoalPlaylist", "7iVjuGSG2HPjvbJjFmKIaL", "Goal Playlist").bindTo(goalPlaylistCVar);
	cvarManager->registerCvar("RLS_WebEndpoint", "https://rocketleague-spotify-api.herokuapp.com", "Web API endpoint");

	cvarManager->getCvar("RLS_Master").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarMasterVolume, this, std::placeholders::_1, std::placeholders::_2));
	cvarManager->getCvar("RLS_GoalPlaylist").addOnValueChanged(std::bind(&RocketLeagueSpotify::CVarGoalPlaylist, this, std::placeholders::_1, std::placeholders::_2));
	
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.InitGame", std::bind(&RocketLeagueSpotify::BeginWaitingForPlayersEvent, this, std::placeholders::_1));
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	//gameWrapper->HookEventPost(" Function TAGame.GameEvent_TA.EventPlayerAdded", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.EventPlayerRemoved", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&RocketLeagueSpotify::MatchEnded, this, std::placeholders::_1));
	gameWrapper->HookEventPost("Function TAGame.GFxShell_TA.ExitToMainMenu", std::bind(&RocketLeagueSpotify::OnExitToMainMenu, this, std::placeholders::_1));
	gameWrapper->HookEventPost("Function GameEvent_TA.Countdown.BeginState", std::bind(&RocketLeagueSpotify::CheckPlayers, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&RocketLeagueSpotify::ReplayStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Replay_TA.StopPlayback", std::bind(&RocketLeagueSpotify::ReplayEnd, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueSpotify::HandleStatEvent, this, std::placeholders::_1, std::placeholders::_2));

	playerID = new UniqueIDWrapper(gameWrapper->GetUniqueID());
	playerIDString = playerID->GetIdString();

	wchar_t* appdata;
	size_t len;
	_wdupenv_s(&appdata, &len, L"APPDATA");
	modDir = gameWrapper->GetBakkesModPathW() + LR"(\rocketleague-spotify)";
	audioDir = modDir + LR"(\assets\audio\)";
	imageDir = modDir + LR"(\assets\images\)";

	spotifyManager = SpotifyManager(cvarManager, modDir, audioDir, imageDir);
	
	CVarGoalPlaylist("", cvarManager->getCvar("RLS_GoalPlaylist"));  // Initialize playlist

	Tick();

	songBgImage = std::make_shared <ImageWrapper>(imageDir + LR"(goal_song_bg.png)", true, true);
	albumArtBgImage = std::make_shared <ImageWrapper>(imageDir + LR"(art_bg.png)", true, true);
}

void RocketLeagueSpotify::onUnload() {
	BASS_Free();
}

void RocketLeagueSpotify::Tick() {
	deltaTime = 0.01667;  // ~60 fps
	if (gameWrapper->GetOnlineGame().IsNull()) { MatchEnded(""); }

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
	if (gameWrapper->GetOnlineGame().IsNull()) { cvarManager->log("Game is null"); return; }
	if (seed != 0) { return; }

	std::string guid = gameWrapper->GetOnlineGame().GetMatchGUID();
	const char* guid_c = guid.c_str();
	for (int a = 0; a < guid.length(); a++) {
		seed += guid_c[a];
	}
	//seed = sscanf_s(, "%u", &seed);
	cvarManager->log("Set seed: " + std::to_string(seed));

	ArrayWrapper<PriWrapper> players = gameWrapper->GetOnlineGame().GetPRIs();
	std::string commaSeparatedIDs;
	for (int a = 0; a < players.Count() - 1; a++) {
		PriWrapper p = players.Get(a);
		if (!p.IsNull()) {
			UniqueIDWrapper id = p.GetUniqueIdWrapper();
			std::string currentID = id.GetIdString();
			commaSeparatedIDs += currentID + ",";
		}
		else {
			cvarManager->log("Player was null");
		}
	}
	PriWrapper p = players.Get(players.Count() - 1);
	if (!p.IsNull()) {
		commaSeparatedIDs += p.GetUniqueIdWrapper().GetIdString();
	}
	cvarManager->log("Retrieving data for " + commaSeparatedIDs);

	std::thread t([&](std::string _commaSeparatedIDs) {
		spotifyManager.cacheManager.RescanCache();
		std::string endpoint = cvarManager->getCvar("RLS_WebEndpoint").getStringValue() + "/users/";
		cpr::Response res = cpr::Get(
			cpr::Url{ endpoint },
			cpr::Parameters{ { "user_ids", _commaSeparatedIDs } }
		);

		if (res.status_code != 200) {
			if (res.status_code == 404) cvarManager->log("No registered users out of: " + _commaSeparatedIDs);
			else cvarManager->log("API Error: " + std::to_string(res.status_code));
			return;
		}

		json data = json::parse(res.text, nullptr, false);
		if (data.is_discarded()) {
			cvarManager->log("Invalid json from response");
			return;
		}

		for (int a = 0; a < data.size(); a++) {
			AddPlaylistForID(data[a]["id"], data[a]["goal_music_uri"]);
			connectedPlayers.push_back(data[a]["id"]);
		}
		std::sort(connectedPlayers.begin(), connectedPlayers.end());
		std::default_random_engine rng = std::default_random_engine(seed);
		for (std::string id : connectedPlayers) {
			loadedPlaylists[id].Shuffle(rng);
		}
	}, commaSeparatedIDs);
	t.detach();
}

void RocketLeagueSpotify::MatchEnded(std::string e) {
	if (!MVPID._Equal("")) {
		for (HSTREAM s : replaySounds) {
			audioManager.StopSound(s);
		}
		replaySounds.clear();

		DWORD s = PlayNextSongForPlayer(MVPID);
		
		MVPID = "";

		gameWrapper->SetTimeout([s, this](GameWrapper* gw) {
			FadeOut(*fadeOutTimeCVar);
			gameWrapper->SetTimeout([s, this](GameWrapper* gw) {
				audioManager.StopSound(s);
			}, 5);
		}, 20);
	}

	seed = 0;
	connectedPlayers.clear();
}

void RocketLeagueSpotify::OnExitToMainMenu(std::string e) {
	FadeOut(*fadeOutTimeCVar);
}

void RocketLeagueSpotify::FadeMasterVolume(int target, int timeToFade) {
	timeSinceFade = 0.f;
	fadeTarget = target;
	fadeDuration = timeToFade;
}

void RocketLeagueSpotify::FadeIn(int timeToFade, int targetVolume/* = -1 */) {
	audioManager.SetMasterVolume(0);
	if (targetVolume == -1) FadeMasterVolume(cvarManager->getCvar("RLS_Master").getIntValue(), timeToFade);
	else FadeMasterVolume(targetVolume, timeToFade);
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
	SpotifyPlaylist playlist = spotifyManager.GetPlaylist(playlistID);
	if (isPlayer) cvarManager->getCvar("RLS_GoalPlaylistStatus").setValue("Downloaded " + std::to_string(playlist.Size()) + " songs!");

	if (playlist.Size() == 0) {
		cvarManager->log("New playlist is empty. Not overriding current playlist");
	}

	loadedPlaylists[IDString] = playlist;

	if (!isPlayer || gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame()) {
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

	if (res.status_code != 200) {
		cvarManager->log("An error occured while syncing the playlist: " + res.status_code);
		cvarManager->log(res.text);
	}
	else {
		cvarManager->log("Synced playlist with the server");
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

HSTREAM RocketLeagueSpotify::PlayNextSongForPlayer(std::string ID, int timeToFade/* = -1 */, int targetVolume/* = -1 */, bool pop/* = true */) {
	if (loadedPlaylists.find(ID) == loadedPlaylists.end() || loadedPlaylists[ID].Size() == 0) {
		cvarManager->log("Empty playlist for " + ID);
		return NULL;
	}

	nextSong = loadedPlaylists[ID].songs.front();

	if (timeToFade == -1) FadeIn(*fadeInTimeCVar, targetVolume);
	else FadeIn(timeToFade, targetVolume);

	cvarManager->log(L"Now playing: " + nextSong.artist + L", \"" + nextSong.name + L"\"");
	HSTREAM s = audioManager.PlaySoundFromFile(nextSong.audioPath);

	if (pop) {
		loadedPlaylists[ID].songs.push_back(nextSong);
		loadedPlaylists[ID].songs.pop_front();
	}
	
	return s;
}

void RocketLeagueSpotify::ReplayStart(std::string eventName) {
	DWORD s = PlayNextSongForPlayer(lastScorerId);
	if (s != NULL) {
		isSongPlaying = true;
		replaySounds.push_back(s);
		albumArtImage = std::make_shared <ImageWrapper>(nextSong.imagePath, true, true);
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

void RocketLeagueSpotify::ReplayEnd(std::string eventName) {
	FadeOut(*fadeOutTimeCVar);
	gameWrapper->SetTimeout([this](GameWrapper* gw) {
		for (HSTREAM s : replaySounds) {
			audioManager.StopSound(s);
		}
		replaySounds.clear();
	}, 5);
	if (isSongPlaying) {
		isSongPlaying = false;
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

void RocketLeagueSpotify::HandleStatEvent(ServerWrapper caller, void* args) {
	TickerStruct* tArgs = (TickerStruct*)args;
	std::string statName = StatEventWrapper(tArgs->StatEvent).GetLabel().ToString();
	
	PriWrapper receiver = PriWrapper(tArgs->Receiver);
	UniqueIDWrapper receiverId = receiver.GetUniqueIdWrapper();

	if (statName._Equal("Goal")) {
		lastScorerId = receiverId.GetIdString();
	}
	else if (statName._Equal("MVP")) {
		MVPID = receiverId.GetIdString();
	}
	else if (statName._Equal("Epic Save")) {
		DWORD s = PlayNextSongForPlayer(receiverId.GetIdString(), *fadeInTimeCVar * 4, cvarManager->getCvar("RLS_Master").getIntValue() / 4, false);
		gameWrapper->SetTimeout([this, s](GameWrapper* gw) {
			FadeOut(*fadeOutTimeCVar * 4);

			gameWrapper->SetTimeout([this, s](GameWrapper* gw) {
				audioManager.StopSound(s);
			}, 5);
		}, 3);
	}
}

// Do ImGui rendering here
void RocketLeagueSpotify::Render()
{
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
	ImGuiIO& io = ImGui::GetIO();
	if (!font) {
		auto gui = gameWrapper->GetGUIManager();
		font = gui.GetFont("myfont");
	}
	if (!ImGui::Begin("RLS", &isWindowOpen_, windowFlags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}
	auto gui = gameWrapper->GetGUIManager();
	auto font = gui.GetFont("myfont");
	ImVec2 windowSize = ImVec2(io.DisplaySize.x, io.DisplaySize.y);
	ImGui::SetWindowSize(windowSize);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	float x = io.DisplaySize.x, y = io.DisplaySize.y;
	float resScale;
	if (y < 1440) {
		resScale = 1.5;
	}
	else if (y < 2160) {
		resScale = 2;
	}
	else {
		resScale = 2.5;
	}
	if (auto songBgTex = songBgImage->GetImGuiTex()) {
		drawList->AddImage(songBgTex, { x * (381.0f / 512.0f), y * (401.0f / 480.0f) }, { x * (159.0f / 160.0f), y * (11.0f / 12.0f) });
	}
	if (auto artTex = albumArtImage->GetImGuiTex()) {
		drawList->AddImage(artTex, { x * (111.0f / 160.0f), y * (1199.0f / 1440.0f) }, { x * (1901.0f / 2560.0f), y * (331.0f / 360.0f) });
	}
	if (auto artBgTex = albumArtBgImage->GetImGuiTex()) {
		drawList->AddImage(artBgTex, { x * (443.0f / 640.0f), y * (239.0f / 288.0f) }, { x * (381.0f / 512.0f), y * (83.0f / 90.0f) });
	}
	
	ImVec2 titlePos = ImVec2(x * (961.0f / 1280.0f), y * (1221.0f / 1440.0f));
	ImVec2 artistPos = ImVec2(x * (961.0f / 1280.0f), y * (1269.0f / 1440.0f));
	std::string songName(nextSong.name.begin(), nextSong.name.end());
	if (songName.length() > 32) {
		songName = songName.substr(0, 31) + "...";
	}
	std::string songArtist(nextSong.artist.begin(), nextSong.artist.end());
	drawList->AddText(font, int(resScale * 1.8f * ImGui::GetFontSize()), titlePos, IM_COL32_WHITE, songName.c_str());
	drawList->AddText(font, int(resScale * 1.4f * ImGui::GetFontSize()), artistPos, IM_COL32(67, 174, 254, 255), songArtist.c_str());

	ImGui::End();

	if (!isWindowOpen_) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string RocketLeagueSpotify::GetMenuName()
{
	return "RLS";
}

// Title to give the menu
std::string RocketLeagueSpotify::GetMenuTitle()
{
	return "RLS";
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void RocketLeagueSpotify::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
	auto gui = gameWrapper->GetGUIManager();
	auto [res, fnt] = gui.LoadFont("myfont", "BourgeoisMedium.ttf", 200);

	if (res == 0) {
		cvarManager->log("Failed to load the font!");
	}
	else if (res == 1) {
		cvarManager->log("The font will be loaded");
	}
	else if (res == 2 && fnt) {
		font = fnt;
	}
}

// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool RocketLeagueSpotify::ShouldBlockInput()
{
	return false;
}

// Return true if window should be interactive
bool RocketLeagueSpotify::IsActiveOverlay()
{
	return false;
}

// Called when window is opened
void RocketLeagueSpotify::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void RocketLeagueSpotify::OnClose()
{
	isWindowOpen_ = false;
}