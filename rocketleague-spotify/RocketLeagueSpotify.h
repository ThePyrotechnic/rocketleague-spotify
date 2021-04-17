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
	std::shared_ptr <std::string> goalPlaylistCVar;

	LinearColor textColor;

	AudioManager audioManager;
	SpotifyManager spotifyManager;
	bool bInMenu;
	UniqueIDWrapper* playerID;
	std::string playerIDString;
	std::wstring modDir;
	std::wstring audioDir;
	void FadeMasterVolume(int, int);
	static std::wstring StrToWStr(std::string);
	clock_t lastFadeTime = 0;
	double timeSinceFade = 0.f;
	double fadeDuration = 0.f;
	double fadeTarget = 0.f;
	double deltaTime = 0.f;
	std::vector<HSTREAM> replaySounds;

	std::string goalSongId;
	std::wstring goalSongFilePath;

	std::unordered_map<std::string, std::vector<std::wstring>> songPaths;

	std::vector<std::string> funnySongs = {
		"29qFlNOssruDfoEN8vN2Uu",
		"1sqoFAF7asvG3Y7ELGsT5M",
		"01b5BkzCdW9eLvbiFIN6oY",
		"3uYDO9dPLTVrgfwg7EYXSf",
		"4kPvYCqzHg48ZHCiBXnmRI",
		"0iGPqIcglmqPUTMv7X2VEb",
		"5cqKPQnDAGgAh6d9x0X7iD",
		"6M14BiCN00nOsba4JaYsHW",
		"6CRtIYDga4VKW5sV5rfAL3",
		"4qDHt2ClApBBzDAvhNGWFd",
		"1PmXm1881bonBI1AlG5uaH",
		"13ZsIACMHaWi8F1va20QZW",
		"3PqShDqDiQljnuLeGCmXjB",
		"56KyV36puztkiJ62ca3D1t",
		"6L33GgWMpd53M1vldLstrW",
		"2zF7IDI6UsLXpwl3FjukPS",
		"3cfOd4CMv2snFaKAnMdnvK",
		"1nbzG1yqmbg8b7EByobVRW",
		"2LELFaNglE9B5xlcmd4qtQ",
		"5e0O7MjhNHq9G67qDFM8nR",
		"7Gts6qFgl7b4ANALcbC1cc",
		"1B75hgRqe7A4fwee3g3Wmu",
		"24CXuh2WNpgeSYUOvz14jk",
		"1mNSomylino1Hoaw3MzCC8",
		"79VupVk4BWNqYcfXe5oVOh",
		"0trTpNYmHAK3EihaZBd2h6",
		"1xOufOKaxfBlxG6l7ytHZ0",
		"1FL7eUG80aeUeyMO2N4btN",
		"6A3pJMpqcz3maI3H0yBLC9",
		"131yybV7A3TmC34a0qE8u8",
		"4C64ZXG24vRJ4lwkxCA24G",
		"1iOHLlEsMbgJfYBFWy4TjG",
		"3JOVTQ5h8HGFnDdp4VT3MP",
		"3aPkMnaq49WpMxY0KAKwBY",
		"1Gk5fwOwrZs379XdVzQ1gq",
		"4jDmJ51x1o9NZB5Nxxc7gY",
		"6FLwmdmW77N1Pxb1aWsZmO",
		"3IDyCfYrxmQMOH0aXpbm91",
		"4cOdK2wGLETKBW3PvgPWqT",
		"1Vchex0xowRj9k59RLvRfo",
		"3KWpkpZRx6mJ3W5jGCLXnY",
		"2WfaOiMkCvy7F5fcp2zZ8L",
		"0ouSkB2t2fGeW60MPcvmXl",
		"0YveezON7jpiaHA8fnUHxN",
		"5dulYd9AKIprMoF837OTDb",
		"5qQ2Bnai4C9NQ1vWBl7yeX",
		"5sRQNFKwE9kgNleZgnCpgw",
		"4nnHlGaBwJHb1rBetqj0Yl",
		"1V2MT0QZbqNUUr2fmSxSFE",
		"7MwwPyZJ7UKFROj2oVnH6R",
		"4ySncWxaMZr4UpebQEPZ2m",
		"33LC84JgLvK2KuW43MfaNq",
		"2IHaGyfxNoFPLJnaEg4GTs",
		"5E3n459RNgTgWjuNDivIvC",
		"4NN5xMoyYbcRsc7ZS8THcK",
		"66TRwr5uJwPt15mfFkzhbi",
		"2Mik4RyMTMGXscX9QGiDoX",
		"4LwU4Vp6od3Sb08CsP99GC",
		"7w87IxuO7BDcJ3YUqCyMTT",
		"1jWiqP0Rm8r3UyfeeN2vRT",
		"2yAVzRiEQooPEJ9SYx11L3",
		"2Fs18NaCDuluPG1DHGw1XG",
		"0KFkqcntA7fnGuLo2mRAvh",
		"4mn9xkejyNn8EBKhrOf3aW",
		"3sJ1HwrSpTVosPhJHFbRLK",
		"7EpoeKjtxThtWkkrpVj4fH",
		"2cF3dqa4vwazzaUlhRXA1I",
		"7KphWfHFIvAWEV1opc5PMT",
		"33t80N4dlcMZhTCBJHoAhQ",
		"4yiGLfzCc2jdByeQ2PIHXA",
		"4kwk7NNS5oieONL98Dt5cV",
		"6MCU7EJ155Ukx2Q6gbScNN",
		"70Vbl5WZallZei6vuwVQeX",
		"15RSD04RKmM1fjrBONNtqK"
	};

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