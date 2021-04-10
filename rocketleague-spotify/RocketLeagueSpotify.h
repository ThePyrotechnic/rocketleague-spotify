#pragma once
#pragma comment( lib, "bakkesmod.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<bool> bEnabled;
	std::string lastEventName;

public:
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper);

	void PlaySound(std::string);
};