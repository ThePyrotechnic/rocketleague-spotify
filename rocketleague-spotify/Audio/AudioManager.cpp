#include "stdafx.h"

#include "AudioManager.h"


AudioManager::AudioManager() {
	long double loadedVersion = HIWORD(BASS_GetVersion());
	if (loadedVersion != BASSVERSION) {
		throw std::runtime_error("Incorrect version of BASS.DLL was loaded (expected " + std::to_string(BASSVERSION) + ", got " + std::to_string(loadedVersion));
	}

	bool init = BASS_Init(-1, 44100, 0, 0, NULL);
	if (BASS_ErrorGetCode() == BASS_ERROR_ALREADY) {
		BASS_Free();
		init = BASS_Init(-1, 44100, 0, 0, NULL);
	}
	if (!init) throw std::runtime_error("Unable to initialize BASS" + std::to_string(BASS_ErrorGetCode()));

	BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, 1);
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, 10000); // global stream volume (0-10000)
}

HSTREAM AudioManager::PlaySoundFromFile(std::wstring file) {

	HSTREAM stream = BASS_StreamCreateFile(FALSE, file.c_str(), 0, 0, 0);
	int code = BASS_ChannelPlay(stream, true);
	if (code != 0) { lastError = code; }
	return stream;
}

void AudioManager::PlaySoundFromURL(std::wstring url) {
}

int AudioManager::StopSound(HSTREAM s) {

	BASS_ChannelStop(s);
	int code = BASS_ErrorGetCode();
	if (code != 0) { lastError = code; return code; }
	BASS_StreamFree(s);
	code = BASS_ErrorGetCode();
	if (code != 0) { lastError = code; }
	return code;
}

float AudioManager::GetMasterVolume() {
	return volume;
}

void AudioManager::SetMasterVolume(float volume) {
	this->volume = volume;  // Preserve float value
	int code = BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, volume * 100.0f);
	if (code != 0) { lastError = code; }
}