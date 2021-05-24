#include "stdafx.h"

#include "Helpers.h"


int Helpers::RandomNumber() {
	std::default_random_engine generator(std::random_device{}());
	std::uniform_int_distribution<int> distribution{ 1 };

	return distribution(generator);
}

std::string Helpers::RandomString(int length) {  // Lifted from https://stackoverflow.com/questions/47977829/generate-a-random-string-in-c11
	std::default_random_engine generator{ std::random_device{}() };
	std::uniform_int_distribution<int> distribution{ '0', 'Z' };  // uses ASCII range
	std::string rand_str(length, '\0');
	for (auto& dis : rand_str)
		dis = distribution(generator);

	return rand_str;
}

std::wstring Helpers::StrToWStr(std::string str) {
	std::wstring wstr = std::wstring(str.length(), L' ');
	std::copy(str.begin(), str.end(), wstr.begin());

	return wstr;
}
