#pragma once

class Song {
public:
	std::string id;
	std::string previewUrl;
	std::wstring name;
	std::wstring artist;
	std::wstring album;
	std::string albumArtUrl;
	std::wstring audioPath;
	std::wstring imagePath;

	Song(){

	}

	Song(std::string id,
		std::string previewUrl,
		std::wstring name,
		std::wstring artist,
		std::wstring album,
		std::string albumArtUrl,
		std::wstring audioPath,
		std::wstring imagePath) {

		this->id = id;
		this->previewUrl = previewUrl;
		this->name = name;
		this->artist = artist;
		this->album = album;
		this->albumArtUrl = albumArtUrl;
		this->audioPath = audioPath;
		this->imagePath = imagePath;
	}
};