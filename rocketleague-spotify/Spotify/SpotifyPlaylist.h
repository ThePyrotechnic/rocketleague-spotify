#pragma once

#include "Helpers.h"
#include "Spotify/Song.h"

class RocketLeagyeSpotify;

class SpotifyPlaylist {
public:
	std::deque<Song> songs;
	std::wstring name;
	std::string id;
	unsigned seed;
	int index = -1;

	size_t Size();
	void Shuffle(std::default_random_engine &rng);
	Song RandomSong();
};