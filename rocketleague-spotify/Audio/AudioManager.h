#pragma once

class AudioManager {
private:
	float volume;
	std::vector<HSTREAM> activeStreams;
public:
	AudioManager();
	int error = 999;
	float GetMasterVolume();
	void PlaySoundFromURL(std::wstring);
	HSTREAM PlaySoundFromFile(std::wstring);
	void SetMasterVolume(float);
	int StopSound(HSTREAM s);
};