#pragma once
#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <fstream>
#include <math.h>
#include <set>
#include <deque>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <unordered_map>
#include <thread>
#include <random>

#pragma comment( lib, "bass.lib" )
#include "bass.h"

#pragma comment( lib, "libcrypto.lib" )
#pragma comment( lib, "zlib.lib" )
#pragma comment( lib, "libssl.lib" )
#pragma comment( lib, "libcurl.lib" )
#pragma comment( lib, "cpr.lib" )
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"

#include "imgui/imgui.h"