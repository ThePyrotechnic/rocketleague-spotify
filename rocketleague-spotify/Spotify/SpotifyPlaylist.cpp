#include "stdafx.h"

#include "Spotify/SpotifyPlaylist.h"

size_t SpotifyPlaylist::Size() { return songs.size(); }

void SpotifyPlaylist::Shuffle(std::mt19937 &rng) {
	std::shuffle(songs.begin(), songs.end(), rng);
}

Song SpotifyPlaylist::RandomSong() {
	int index = Helpers::RandomNumber() % Size();
	return songs[index];
}