#pragma once

class AudioManager {
public:
	int lastError = 0;
	float volume;
	std::vector<HSTREAM> activeStreams;
	AudioManager();
	int error = 999;
	float GetMasterVolume();
	void PlaySoundFromURL(std::wstring);
	HSTREAM PlaySoundFromFile(std::wstring);
	void SetMasterVolume(float);
	int StopSound(HSTREAM s);
};