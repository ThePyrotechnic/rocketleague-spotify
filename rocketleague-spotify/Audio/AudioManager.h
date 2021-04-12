#pragma once

class AudioManager {
public:
	AudioManager();
	int error = 999;
	void PlaySoundFromURL(std::wstring);
	void PlaySoundFromFile(std::wstring);
	void SetMasterVolume(int);
};