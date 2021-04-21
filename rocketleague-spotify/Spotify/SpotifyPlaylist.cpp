#include "stdafx.h"

#include "Spotify/SpotifyPlaylist.h"

size_t SpotifyPlaylist::Size() { return songs.size(); }

void SpotifyPlaylist::Shuffle(std::default_random_engine &rng) {
	std::shuffle(songs.begin(), songs.end(), rng);
}