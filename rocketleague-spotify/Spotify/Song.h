#pragma once

class Song {
public:
	std::string id;
	std::string previewUrl;
	std::wstring name;
	std::wstring artist;
	std::wstring album;
	std::string albumArtUrl;
	std::wstring path;

	Song(std::string id,
		std::string previewUrl,
		std::wstring name,
		std::wstring artist,
		std::wstring album,
		std::string albumArtUrl,
		std::wstring path) {

		this->id = id;
		this->previewUrl = previewUrl;
		this->name = name;
		this->artist = artist;
		this->album = album;
		this->albumArtUrl = albumArtUrl;
		this->path = path;
	}
};