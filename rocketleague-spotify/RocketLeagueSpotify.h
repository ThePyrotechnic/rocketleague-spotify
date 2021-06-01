#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "Helpers.h"
#include "Audio/AudioManager.h"
#include "Spotify/SpotifyManager.h"
#include "Spotify/SpotifyPlaylist.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow {
public:
	void onLoad() override;
	void onUnload() override;

	void FadeIn(int, int=-1);
	void FadeOut(int);
	void HandleStatEvent(ServerWrapper, void*);
	void ReplayStart(std::string);
	void ReplayEnd(std::string);
	void InitGame(std::string);
	void MatchEnded(std::string);
	void OnExitToMainMenu(std::string);
	void AddPlaylistForID(std::string, std::string);
	void CVarMasterVolume(std::string, CVarWrapper);
	void CVarGoalPlaylist(std::string, CVarWrapper);
	void Tick();
	void CleanUp(std::string);
	void LoadPlaylists(std::string);
	void ForceQuit();
	void SetSyncStatus();
	void CVarSyncStatus(std::string, CVarWrapper);
	void CVarAuthChanged(std::string, CVarWrapper);
	
	void SpotifyAuth();
	void SpotifyDigestAuth();

	std::shared_ptr<int> fadeInTimeCVar;
	std::shared_ptr<int> fadeOutTimeCVar;
	std::shared_ptr<std::string> goalPlaylistCVar;
	std::shared_ptr<bool> playInTrainingCVar;
	std::shared_ptr<bool> stopInMenuCVar;
	std::shared_ptr<bool> forceQuitEnabledCVar;
	std::shared_ptr<bool> ownPlaylistOnlyCVar;
	std::shared_ptr<bool> partyMembersOnlyCVar;
	std::shared_ptr<bool> downloadEnemyPlaylistsCVar;
	std::shared_ptr<bool> ownPlaylistForEnemiesCVar;
	std::shared_ptr<bool> useOwnForMissingCVar;

	std::shared_ptr<std::string> codeVerifierCVar;

	std::shared_ptr<bool> isAuthenticatedSpotifyCVar;
	std::shared_ptr<std::string> spotifyAccessTokenCVar;
	std::shared_ptr<std::string> spotifyRefreshTokenCVar;
	std::shared_ptr<std::string> spotifyAuthCodeCVar;

	LinearColor textColor;

	AudioManager audioManager;
	SpotifyManager spotifyManager;
	UniqueIDWrapper* playerID;
	std::string playerIDString;
	std::wstring modDir;
	std::wstring audioDir;
	std::wstring imageDir;
	std::wstring cvarConfigDir;
	void FadeMasterVolume(int, int);
	HSTREAM PlayNextSongForPlayer(std::string, int=-1, int=-1, bool=true);
	clock_t lastFadeTime = 0;
	double timeSinceFade = 0.f;
	double fadeDuration = 0.f;
	double fadeTarget = 0.f;
	double deltaTime = 0.f;
	std::vector<HSTREAM> replaySounds;
	std::string lastScorerId;
	std::string MVPID;
	unsigned int seed;
	std::vector<std::string> playersWithPlaylists;
	std::unordered_map<std::string, PriWrapper> playerRefs;

	std::unordered_map<std::string, SpotifyPlaylist> loadedPlaylists;

	std::shared_ptr<ImageWrapper> songBgImage;
	std::shared_ptr<ImageWrapper> albumArtBgImage;
	std::shared_ptr<ImageWrapper> albumArtImage;
	bool isSongPlaying;

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
