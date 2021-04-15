#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"


class RocketLeagueSpotify : public BakkesMod::Plugin::BakkesModPlugin {
public:
	RocketLeagueSpotify();
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper);
	void FadeIn(int);
	void FadeOut(int);
	void HandleStatEvent(ServerWrapper, void*);
	void ReplayStart(std::string);
	void ReplayEnd(std::string);
	void CVarMasterVolume(std::string, CVarWrapper);
	void CVarGoalSong(std::string, CVarWrapper);
	void Tick();

	std::shared_ptr<int> fadeInTimeCVar;
	std::shared_ptr<int> fadeOutTimeCVar;

	LinearColor textColor;

	AudioManager audioManager;
	SpotifyManager spotifyManager;
	bool bInMenu;
	UniqueIDWrapper* playerID;
	std::string playerIDString;
	std::wstring modDir;

	void FadeMasterVolume(int, int);

	clock_t lastFadeTime = 0;
	double timeSinceFade = 0.f;
	double fadeDuration = 0.f;
	double fadeTarget = 0.f;
	double deltaTime = 0.f;
	std::vector<HSTREAM> replaySounds;

	std::string goalSongId;
	std::wstring goalSongFilePath;

	std::vector<std::wstring> songPaths;

	std::vector<std::string> randomSongs = {
		"7tBa1miuCyMBRAT3n3MEU2",
		"0YXZX07Iv3xqkqW9hQaMaG",
		"0m9Td3Cuc0frjFJWxvYpon",
		"7faFNpxAE54UaUTlpg1RTF",
		"6I7FFkcXS58luQoYLzPHi3",
		"6cthJIumjsMpJBeDJ2w6Dn",
		"1hGRe4d3LJCg1VszAU8Cy1",
		"20ztml2STRF7Sq1UaBB6ox",
		"0iGPqIcglmqPUTMv7X2VEb",
		"2q1Wbi1ZN9loz58YF8S10e",
		"5CZNuQZszsDSa0VX7kpFsV",
		"4kwk7NNS5oieONL98Dt5cV",
		"6tlZPoCiTZUZmVypVnybzd",
		"1wvwpqp33lJ90fMOZ9odG3",
		"3vfOHusoUfUQEhGxLdM8ek",
		"3ARnfNFPZ9DYviuqC9TK3M",
		"4n0sVfRnd0UJsqcbPj7GqN",
		"6W9p8YvgzEMJT8ENSaw32S",
		"0dPcCco9gfUVAb0JVds2NB",
		"3OaunNUlXXs5e2PXtNAzzG",
		"5yeXDo6VhuDgvnKcaPKZjh",
		"5QYTgU6q3X6Tbskus5wqlp",
		"4kfeRwpq5KUaqTkgi4TbDF",
		"527k23H0A4Q0UJN3vGs0Da",
		"2n5tZGBgPZ4qeQH7UcTJgN",
		"3bcF82sBcGX88lntacdjkN",
		"6MO2bfLHKykUgCChFdw91H",
		"74fV8TuLZKVzSIOOGu8wwI",
		"5yY9lUy8nbvjM1Uyo1Uqoc",
		"7dsImih2L25fa6qjNphBI7",
		"2QeQNF182V61Im0QpjdVta"
	};
};