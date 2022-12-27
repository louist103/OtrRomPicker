#ifdef _WIN32
#include <Windows.h>
#include <winuser.h>
#endif

#include <stdint.h>
#include <stdlib.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_messagebox.h>

#include <array>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <limits>

enum class ButtonId : int {
	YES,
	NO,
	FIND,
};

extern "C" uint32_t CRC32C(unsigned char* data, size_t dataSize);
extern "C" void RomToBigEndian(void* rom, size_t romSize);

constexpr uint32_t OOT_NTSC_10 = 0xEC7011B7;
constexpr uint32_t OOT_NTSC_11 = 0xD43DA81F;
constexpr uint32_t OOT_NTSC_12 = 0x693BA2AE;
constexpr uint32_t OOT_PAL_10 = 0xB044B569;
constexpr uint32_t OOT_PAL_11 = 0xB2055FBD;
constexpr uint32_t OOT_NTSC_JP_GC_CE = 0xF7F52DB8;
constexpr uint32_t OOT_NTSC_JP_GC = 0xF611F4BA;
constexpr uint32_t OOT_NTSC_US_GC = 0xF3DD35BA;
constexpr uint32_t OOT_PAL_GC = 0x09465AC3;
constexpr uint32_t OOT_NTSC_JP_MQ = 0xF43B45BA;
constexpr uint32_t OOT_NTSC_US_MQ = 0xF034001A;
constexpr uint32_t OOT_PAL_MQ = 0x1D4136F3;
constexpr uint32_t OOT_PAL_GC_DBG1 = 0x871E1C92; // 03-21-2002 build
constexpr uint32_t OOT_PAL_GC_DBG2 = 0x87121EFE; // 03-13-2002 build
constexpr uint32_t OOT_PAL_GC_MQ_DBG = 0x917D18F6;
constexpr uint32_t OOT_IQUE_TW = 0x3D81FB3E;
constexpr uint32_t OOT_IQUE_CN = 0xB1E1E07B;

constexpr size_t MB_BASE = 1024 * 1024;
constexpr size_t MB32 = 32 * MB_BASE;
constexpr size_t MB54 = 54 * MB_BASE;
constexpr size_t MB64 = 64 * MB_BASE;

//Uncomment to add a supported rom. Make sure to add the CRC32C (Poly 0x1EDC6F41) of the full ROM to the array bellow
static const std::unordered_map<uint32_t, const char*> verMap {
//	{OOT_NTSC_10, "NTSC 1.0"},
//	{OOT_NTSC_11, "NTSC 1.1"},
//	{OOT_NTSC_12, "NTSC 1.2"},
//	{OOT_PAL_10, "PAL 1.0"},
//	{OOT_PAL_11, "PAL 1.1"},
//	{OOT_NTSC_JP_GC_CE, "NTSC JP Collectors Edition"},
//	{OOT_NTSC_JP_GC, "NTSC JP Gamecube"},
//	{OOT_NTSC_US_GC, "NTSC US Gamecube"},
	{OOT_PAL_GC, "Pal Gamecube"},
//	{OOT_NTSC_JP_MQ, "NTSC JP Gamecube"},
//	{OOT_NTSC_US_MQ, "NTSC US Gamecube"},
//	{OOT_PAL_MQ, "PAL MQ"},
	{OOT_PAL_GC_DBG1, "PAL Debug 1"},
	{OOT_PAL_GC_DBG2, "PAL Debug 2"},
	{OOT_PAL_GC_MQ_DBG, "PAL MQ Debug"},
//	{OOT_IQUE_TW, "IQUE TW"},
//	{OOT_IQUE_CN, "IQUE CN"},
};

static constexpr std::array<const uint32_t, 5> goodCrcs = {
	0x8652ac4c, // MQ DBG 64MB
	0x1f731ffe, // MQ DBG 54MB
	0x044b3982, // NMQ DBG 54MB
	0xEB15D7B9, // NMQ DBG 64MB
	0xDA8E61BF, // GC PAL
};

static void SetupBox(SDL_MessageBoxData* boxData, SDL_MessageBoxButtonData buttonData[], const char* text) {
	buttonData[0].buttonid = 0;
	buttonData[0].text = "Yes";
	buttonData[1].buttonid = 1;
	buttonData[1].text = "No";
	buttonData[2].buttonid = 2;
	buttonData[2].text = "Find ROM";

	boxData->flags = SDL_MESSAGEBOX_INFORMATION;
	boxData->message = text;
	boxData->title = "Rom Detected";
	boxData->window = nullptr;
	boxData->numbuttons = 3;
	boxData->buttons = buttonData;
}
#ifdef _WIN32
static void OpenFileBox(char* path) {
	OPENFILENAMEA box = { 0 };
	path[0] = 0;
	box.lStructSize = sizeof(box);
	box.lpstrFile = path;
	box.nMaxFile = sizeof(path);
	box.lpstrTitle = "Open ROM";
	box.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_PATHMUSTEXIST; /* | OFN_ENABLETEMPLATE */
	GetOpenFileNameA(&box);
}

#if 0
static size_t OpenFileReadMap(const wchar_t* name, std::tuple<HANDLE, HANDLE, void*>& mapTripple) {
	std::get<T_HANDLE>(mapTripple) = CreateFile(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	std::get<T_MAPOBJ>(mapTripple) = CreateFileMapping(std::get<T_HANDLE>(mapTripple), nullptr, PAGE_READWRITE, 0, 0, nullptr);
	std::get<T_MAPPTR>(mapTripple) = MapViewOfFile(std::get<T_MAPOBJ>(mapTripple), FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	return GetFileSize(std::get<T_HANDLE>(mapTripple), 0);
}

static void CloseMap(std::tuple<HANDLE, HANDLE, void*>& mapTripple) {
	UnmapViewOfFile(std::get<T_MAPPTR>(mapTripple));
	CloseHandle(std::get<T_MAPOBJ>(mapTripple));
	CloseHandle(std::get<T_HANDLE>(mapTripple));
	std::get<T_MAPPTR>(mapTripple) = nullptr;
	std::get<T_MAPOBJ>(mapTripple) = nullptr;
	std::get<T_HANDLE>(mapTripple) = nullptr;
}
#endif

#endif

static uint32_t GetRomVerCrc(void* rom) {
	return _byteswap_ulong(((uint32_t*)rom)[4]);
}

static auto GetFileSize(const std::filesystem::path::string_type& path) {
	return std::filesystem::file_size(path);
}

static bool ValidateAndFixRom(void* romData, size_t size) {
	// The MQ debug rom is sometimes patched to have a 'U' in the header when it shouldn't. Undo that.
	if (GetRomVerCrc(romData) == OOT_PAL_GC_MQ_DBG) {
		reinterpret_cast<uint8_t*>(romData)[0x3E] = 'P';
	}
	uint32_t actualCrc = CRC32C(static_cast<uint8_t*>(romData), size);

	return true;
}

template<typename T>
static void OpenFile(std::ifstream& fstream, const T& path) {
	if (fstream.is_open()) {
		fstream.close();
	}
	fstream.open(path, std::ios::in | std::ios::binary);
	if (!fstream.is_open()) { /*TODO: Handle errors*/ }
}


int main(void) {
	SDL_MessageBoxData boxData = { 0 };
	SDL_MessageBoxButtonData buttons[3] = { {0} };
	std::vector<std::filesystem::path::string_type> roms;
	int messageBoxRet;
	uint32_t verCrc;
	std::ifstream inFile;
	void* romData = operator new(MB64);
	

	for (const auto& file : std::filesystem::directory_iterator("R:\\")) {
		if (file.is_directory()) continue;
		if ((file.path().extension() == ".n64") || (file.path().extension() == ".z64") || (file.path().extension() == ".v64")) {
			roms.push_back((file.path()));
		}
	
	}
	char textBoxBuffer[512];

	for (const auto& rom : roms) {
		std::streamsize fileSize;

		OpenFile(inFile, rom);
		fileSize = GetFileSize(rom.c_str());

		if (fileSize != MB32 && fileSize != MB54 && fileSize != MB64) {
			inFile.close();
			textBoxBuffer[0] = 0;
			snprintf(textBoxBuffer, sizeof(textBoxBuffer), "The rom file %ls was not a valid size. Was %zu MB, expecting 32, 54, or 64MB.", rom.c_str(), fileSize / MB_BASE);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid Rom Size", textBoxBuffer, nullptr);
			continue;
		}
		inFile.read((char*)romData, fileSize);

		RomToBigEndian(romData, fileSize);
		verCrc = GetRomVerCrc(romData);

		if (!verMap.contains(verCrc)) {
			inFile.close();
			continue;
		}
		textBoxBuffer[0] = 0;
		snprintf(textBoxBuffer, sizeof(textBoxBuffer), "Rom detected: %ls, Header CRC32: %8X. It appears to be: %s. Use this rom?", rom.c_str(), verCrc, verMap.at(verCrc));
		SetupBox(&boxData, buttons, textBoxBuffer);
		SDL_ShowMessageBox(&boxData, &messageBoxRet);
		if (messageBoxRet == (int)ButtonId::YES) { 
			if (!ValidateAndFixRom(romData, fileSize)) {
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Rom CRC invalid", "Rom CRC did not match the list of known good roms. Please find another.", nullptr);
				continue;
			}
			break;
		}
		else if (messageBoxRet == (int)ButtonId::FIND) {
			char newPath[MAX_PATH];
			OpenFileBox(newPath);
			OpenFile(inFile, newPath);
			ValidateAndFixRom(romData, fileSize);
		}
		else if (messageBoxRet == (int)ButtonId::NO) {
			inFile.close();
			continue;
		}

	}
	inFile.close();
	operator delete(romData);
	return 0;
}
