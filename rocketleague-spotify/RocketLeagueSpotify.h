#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin {
private:
	std::shared_ptr<bool> bEnabled;

	LinearColor textColor;

	AudioManager audioManager;
	bool bInMenu;
	std::string lastEventName;
	std::string lastStatName;
	PriWrapper *lastStatPlayer;
	PriWrapper *lastStatVictim;
	UniqueIDWrapper *playerID;
	std::string playerIDString;
	std::wstring assetDir;

public:
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper);

	void HandleStatEvent(ServerWrapper, void*);
	void ReplayStart(std::string);
	void ReplayEnd(std::string);
	void SetMasterVolume(std::string, CVarWrapper);
};