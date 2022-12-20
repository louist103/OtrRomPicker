#ifdef _WIN32
#include <Windows.h>
#include <fileapi.h>
#include <memoryapi.h>
#include <winuser.h>
constexpr auto T_HANDLE = 0;
constexpr auto T_MAPOBJ = 1;
constexpr auto T_MAPPTR = 2;
#endif

#include <stdint.h>
#include <stdlib.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL.h>
#include <SDL_messagebox.h>

#include <filesystem>
#include <algorithm>
#include <iostream>

enum class ButtonId : int {
	YES,
	NO,
	FIND,
};


extern "C" uint32_t CRC32C(unsigned char* data, size_t dataSize);

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

static size_t OpenFileReadMap(const wchar_t* name, std::tuple<HANDLE, HANDLE, void*>& mapTripple) {
	std::get<T_HANDLE>(mapTripple) = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	std::get<T_MAPOBJ>(mapTripple) = CreateFileMapping(std::get<T_HANDLE>(mapTripple), nullptr, PAGE_READONLY, 0, 0, nullptr);
	std::get<T_MAPPTR>(mapTripple) = MapViewOfFile(std::get<T_MAPOBJ>(mapTripple), FILE_MAP_READ, 0, 0, 0);
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

uint32_t GetRomVerCrc(void* rom) {
	uint8_t firstByte = ((uint8_t*)rom)[0];
	uint32_t verCrc;
	switch (firstByte) {
		case 0x80: //Big Endian
			verCrc = _byteswap_ulong(((uint32_t*)rom)[4]);
			break;
		case 0x40: //Little Endian
			verCrc = ((uint32_t*)rom)[4];
			break;
		case 0x37: //ByteSwapped
			verCrc = (((uint16_t*)rom)[10]) | ((((uint16_t*)rom)[8]) << 16);
			break;
	}
	return verCrc;
}

int main(void) {
	SDL_MessageBoxData boxData = { 0 };
	SDL_MessageBoxButtonData buttons[3] = {0};
	std::vector<std::filesystem::path::string_type> roms;
	int messageBoxRet;

#ifdef _WIN32
	std::tuple<HANDLE, HANDLE, void*> fileMapTripple;
#endif

	for (const auto& file : std::filesystem::directory_iterator("./")) {
		if (file.is_directory()) continue;
		if ((file.path().extension() == ".n64") || (file.path().extension() == ".z64") || (file.path().extension() == ".v64")) {
			roms.push_back(std::move(file.path().c_str()));
		}
	
	}
	char message[128];

	for (const auto& rom : roms) {
	    std::wstring rom = L"R:\\ZELOOTMA.v64";
		size_t fileSize = OpenFileReadMap(rom.c_str(), fileMapTripple);
		if (fileSize != MB32 && fileSize != MB54 && fileSize != MB64) {
			CloseMap(fileMapTripple);
			char errMessage[256];
			snprintf(errMessage, sizeof(errMessage), "The rom file %ls was not a valid size. Was %zu MB, expecting 32, 54, or 64MB.", rom.c_str(), fileSize / MB_BASE);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid Rom Size", errMessage, nullptr);
			continue;
		}
		(void)GetRomVerCrc(std::get<T_MAPPTR>(fileMapTripple));
		//snprintf(message, sizeof(message), "Rom detected: %ls, CRC32: %8X. Use this rom?", rom.c_str(), crc);
		SetupBox(&boxData, buttons, message);
		SDL_ShowMessageBox(&boxData, &messageBoxRet);
		if (messageBoxRet == (int)ButtonId::YES) { break; }
		else if (messageBoxRet == (int)ButtonId::FIND) {
			CloseMap(fileMapTripple);
			//OpenFileBox();
		}
	}
	return 0;
}
