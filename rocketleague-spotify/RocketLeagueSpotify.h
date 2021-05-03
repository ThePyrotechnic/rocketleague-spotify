#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "Audio/AudioManager.h"
#include "Spotify/SpotifyManager.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow {
public:
	//RocketLeagueSpotify();
	void onLoad() override;
	void onUnload() override;

	void FadeIn(int);
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
	std::wstring imageDir;
	void FadeMasterVolume(int, int);
	HSTREAM PlayNextSongForPlayer(std::string);
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

	std::shared_ptr<ImageWrapper> songBgImage;
	std::shared_ptr<ImageWrapper> albumArtBgImage;
	std::shared_ptr<ImageWrapper> albumArtImage;
	bool isInReplay;

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;

	bool isWindowOpen_ = false;

	ImFont* font;
	Song nextSong;
};
