#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin {
private:
	std::shared_ptr<bool> bEnabled;

	LinearColor textColor;

	AudioManager audioManager;
	std::string lastEventName;
	std::string lastStatName;
	std::string lastStatPlayer;
	std::string lastStatVictim;
	std::string spotifyCredential;
	std::string spotifyToken;
	bool bInMenu;
	UniqueIDWrapper *playerID;
	std::string playerIDString;
	std::wstring modDir;
	void DownloadPreview(std::string);
	void DownloadSong(std::string songId);
	void AuthenticateSpotify();

public:
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper);

	void HandleStatEvent(ServerWrapper, void*);
	void ReplayStart(std::string);
	void ReplayEnd(std::string);
	void SetMasterVolume(std::string, CVarWrapper);
};