#include "../LanguageRegistry.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
constexpr const char* SteamKey = "Software\\Valve\\Steam";
const char* registryValue = "russian";
int originalCalls = 0;
const char* originalDefault = nullptr;
bool terminateOriginal = true;

void __cdecl OriginalReader(const char*, const char*, char* output, int capacity, const char* defaultValue)
{
	++originalCalls;
	originalDefault = defaultValue;
	if (!output || capacity <= 0)
		return;
	if (!terminateOriginal)
	{
		std::memset(output, 'x', static_cast<size_t>(capacity));
		return;
	}
	std::snprintf(output, static_cast<size_t>(capacity), "%s", registryValue ? registryValue : defaultValue);
}

void ExpectText(const char* expected, const char* actual)
{
	if (std::strcmp(expected, actual))
	{
		std::fprintf(stderr, "Expected '%s', got '%s'\n", expected, actual);
		std::exit(EXIT_FAILURE);
	}
}

void TestFiltering()
{
	struct Query { const char* key; const char* value; bool language; };
	const Query queries[] = {
		{SteamKey, "Language", true},
		{"sOfTwArE\\vAlVe\\sTeAm", "lAnGuAgE", true},
		{SteamKey, "Skin", false},
		{SteamKey, "LanguageSuffix", false},
		{"Software\\Valve\\Half-Life\\Settings", "User Token 2", false},
		{"Software\\Valve\\Steam\\Other", "Language", false},
		{"Other\\Software\\Valve\\Steam", "Language", false},
		{"HKEY_LOCAL_MACHINE\\Software\\Valve\\Steam", "Language", false},
		// The old five-argument reader only strips HKLM; explicit HKCU is a literal subkey.
		{"HKEY_CURRENT_USER\\Software\\Valve\\Steam", "Language", false},
		{nullptr, "Language", false},
		{SteamKey, nullptr, false},
	};
	for (const auto& query : queries)
	{
		char output[128] = "before original";
		char current[128] = "previous";
		const int before = originalCalls;
		LanguageRegistry::Read(OriginalReader, query.key, query.value, output, sizeof(output),
			"default", "schinese", current, sizeof(current));
		assert(before + 1 == originalCalls);
		ExpectText("default", originalDefault);
		ExpectText(query.language ? "schinese" : "russian", output);
		ExpectText(query.language ? "schinese" : "previous", current);
	}
}

void TestOriginalLanguageAndDefault()
{
	for (const char* value : {"russian", "english", "", static_cast<const char*>(nullptr)})
	{
		registryValue = value;
		for (const char* forced : {static_cast<const char*>(nullptr), ""})
		{
			char output[128] = {};
			char current[128] = "previous";
			LanguageRegistry::Read(OriginalReader, SteamKey, "Language", output, sizeof(output),
				"fallback", forced, current, sizeof(current));
			ExpectText(value ? value : "fallback", output);
			ExpectText(value ? value : "fallback", current);
		}
	}
	registryValue = "russian";
}

void TestCapacityAndEffectiveLanguage()
{
	char output[] = {'?', '?', '?', '?', '!'};
	char current[128] = {};
	LanguageRegistry::Read(OriginalReader, SteamKey, "Language", output, 4,
		"", "schinese", current, sizeof(current));
	ExpectText("sch", output);
	ExpectText("sch", current);
	assert('!' == output[4]);

	LanguageRegistry::Read(OriginalReader, SteamKey, "Language", output, 1,
		"", "schinese", current, sizeof(current));
	ExpectText("", output);
	ExpectText("", current);

	const std::string longLanguage(256, 'a');
	char full[128] = {};
	char smallCurrent[5] = {};
	LanguageRegistry::Read(OriginalReader, SteamKey, "Language", full, sizeof(full),
		"", longLanguage.c_str(), smallCurrent, sizeof(smallCurrent));
	assert(sizeof(full) - 1 == std::strlen(full));
	ExpectText("aaaa", smallCurrent);

	terminateOriginal = false;
	char noTerminator[3] = {};
	LanguageRegistry::Read(OriginalReader, SteamKey, "Language", noTerminator, sizeof(noTerminator),
		"", nullptr, current, sizeof(current));
	ExpectText("xxx", current);
	terminateOriginal = true;
}

void TestInvalidBuffers()
{
	for (int capacity : {0, -1})
	{
		char output[] = "unchanged";
		char current[] = "previous";
		LanguageRegistry::Read(OriginalReader, SteamKey, "Language", output, capacity,
			"", "schinese", current, sizeof(current));
		ExpectText("unchanged", output);
		ExpectText("previous", current);
	}
	char current[] = "previous";
	LanguageRegistry::Read(OriginalReader, SteamKey, "Language", nullptr, 128,
		"", "schinese", current, sizeof(current));
	ExpectText("previous", current);
	char output[128] = {};
	LanguageRegistry::Read(OriginalReader, SteamKey, "Language", output, sizeof(output),
		"", "schinese", nullptr, 0);
	ExpectText("schinese", output);
}
}

int main()
{
	TestFiltering();
	TestOriginalLanguageAndDefault();
	TestCapacityAndEffectiveLanguage();
	TestInvalidBuffers();
	std::puts("Language registry tests passed.");
}
