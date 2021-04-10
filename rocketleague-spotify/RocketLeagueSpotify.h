#pragma once
#pragma comment( lib, "bakkesmod.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<bool> bEnabled;

	LinearColor textColor;

	std::string lastEventName;
	std::string lastStatName;
	std::string lastStatPlayer;
	std::string lastStatVictim;
	bool bInMenu;

public:
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper);

	void HandleStatEvent(ServerWrapper, void*);
	void ReplayStart(std::string);
	void ReplayEnd(std::string);
};