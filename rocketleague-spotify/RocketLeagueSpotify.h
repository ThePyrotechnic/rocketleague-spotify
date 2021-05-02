#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"

#include "Audio/AudioManager.h"
#include "Spotify/SpotifyManager.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin {
public:
	//RocketLeagueSpotify();
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper);
	void FadeIn(int, int=-1);
	void FadeOut(int);
	void HandleStatEvent(ServerWrapper, void*);
	void ReplayStart(std::string);
	void ReplayEnd(std::string);
	void CheckPlayers(std::string);
	void MatchEnded(std::string);
	void OnExitToMainMenu(std::string);
	void AddPlaylistForID(std::string, std::string);
	void CVarMasterVolume(std::string, CVarWrapper);
	void CVarGoalPlaylist(std::string, CVarWrapper);
	void Tick();

	std::shared_ptr<int> fadeInTimeCVar;
	std::shared_ptr<int> fadeOutTimeCVar;
	std::shared_ptr <std::string> goalPlaylistCVar;

	LinearColor textColor;

	AudioManager audioManager;
	SpotifyManager spotifyManager;
	UniqueIDWrapper* playerID;
	std::string playerIDString;
	std::wstring modDir;
	std::wstring audioDir;
	void FadeMasterVolume(int, int);
	HSTREAM PlayNextSongForPlayer(std::string, int=-1, int=-1, bool=true);
	static std::wstring StrToWStr(std::string);
	clock_t lastFadeTime = 0;
	double timeSinceFade = 0.f;
	double fadeDuration = 0.f;
	double fadeTarget = 0.f;
	double deltaTime = 0.f;
	std::vector<HSTREAM> replaySounds;
	std::string lastScorerId;
	std::string MVPID;
	unsigned int seed;
	std::vector<std::string> connectedPlayers;

	std::unordered_map<std::string, SpotifyPlaylist> loadedPlaylists;
};
