#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <winuser.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif

#include <sys/stat.h>

#if __has_include(<byteswap.h>)
#include "byteswap.h"
#define _byteswap_ulong(x) bswap_32(x)
#endif

// Non Win32 doesn't have this defined. Define it to something I know exists and works.
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#if defined(_MSC_VER)
#define ASSUME(x) __assume(x)
#else
#define ASSUME(x)
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
#include <unordered_map>
#include <memory>

extern "C" uint32_t CRC32C(unsigned char* data, size_t dataSize);
extern "C" void RomToBigEndian(void* rom, size_t romSize);

using FsPath = std::filesystem::path::string_type;

static constexpr uint32_t OOT_NTSC_10 = 0xEC7011B7;
static constexpr uint32_t OOT_NTSC_11 = 0xD43DA81F;
static constexpr uint32_t OOT_NTSC_12 = 0x693BA2AE;
static constexpr uint32_t OOT_PAL_10 = 0xB044B569;
static constexpr uint32_t OOT_PAL_11 = 0xB2055FBD;
static constexpr uint32_t OOT_NTSC_JP_GC_CE = 0xF7F52DB8;
static constexpr uint32_t OOT_NTSC_JP_GC = 0xF611F4BA;
static constexpr uint32_t OOT_NTSC_US_GC = 0xF3DD35BA;
static constexpr uint32_t OOT_PAL_GC = 0x09465AC3;
static constexpr uint32_t OOT_NTSC_JP_MQ = 0xF43B45BA;
static constexpr uint32_t OOT_NTSC_US_MQ = 0xF034001A;
static constexpr uint32_t OOT_PAL_MQ = 0x1D4136F3;
static constexpr uint32_t OOT_PAL_GC_DBG1 = 0x871E1C92; // 03-21-2002 build
static constexpr uint32_t OOT_PAL_GC_DBG2 = 0x87121EFE; // 03-13-2002 build
static constexpr uint32_t OOT_PAL_GC_MQ_DBG = 0x917D18F6;
static constexpr uint32_t OOT_IQUE_TW = 0x3D81FB3E;
static constexpr uint32_t OOT_IQUE_CN = 0xB1E1E07B;

static constexpr size_t MB_BASE = 1024 * 1024;
static constexpr size_t MB32 = 32 * MB_BASE;
static constexpr size_t MB54 = 54 * MB_BASE;
static constexpr size_t MB64 = 64 * MB_BASE;

static const std::unordered_map<uint32_t, const char*> verMap = {
    //	{OOT_NTSC_10, "NTSC 1.0"},
    //	{OOT_NTSC_11, "NTSC 1.1"},
    //	{OOT_NTSC_12, "NTSC 1.2"},
    //	{OOT_PAL_10, "PAL 1.0"},
    //	{OOT_PAL_11, "PAL 1.1"},
    //	{OOT_NTSC_JP_GC_CE, "NTSC JP Collectors Edition"},
    //	{OOT_NTSC_JP_GC, "NTSC JP Gamecube"},
    //	{OOT_NTSC_US_GC, "NTSC US Gamecube"},
    { OOT_PAL_GC, "Pal Gamecube" },
    //	{OOT_NTSC_JP_MQ, "NTSC JP Gamecube"},
    //	{OOT_NTSC_US_MQ, "NTSC US Gamecube"},
    //	{OOT_PAL_MQ, "PAL MQ"},
    { OOT_PAL_GC_DBG1, "PAL Debug 1" },
    { OOT_PAL_GC_DBG2, "PAL Debug 2" },
    { OOT_PAL_GC_MQ_DBG, "PAL MQ Debug" },
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

enum class ButtonId : int {
    YES,
    NO,
    FIND,
};

enum class BoxReadResult : int {
    SUCCESS,
    FAIL_NO_FILE,
    FAIL_BAD_DATA,
};

enum class MsgBoxType : int {
    YESNO,
    ROM,
};

class Extractor {
    std::unique_ptr<unsigned char[]> mRomData = std::make_unique<unsigned char[]>(MB64);
    FsPath mCurrentRomPath;
    size_t mCurRomSize = 0;

#ifdef _WIN32
    bool GetRomPathFromBox();
#endif

    uint32_t GetRomVerCrc();
    size_t GetCurRomSize();
    bool ValidateAndFixRom();
    bool ValidateRomSize();

    bool ValidateRom(bool skipCrcBox = false);
    const char* GetZapdVerStr();
    bool IsMasterQuest();
    void CallZapd();

    int CreateYesNoBox();
    void SetRomInfo(const FsPath& path);

    void GetRoms(std::vector<FsPath>& roms);
    void ShowSizeErrorBox();
    void ShowCrcErrorBox();
    int ShowRomPickBox(uint32_t verCrc);

  public:
    bool Run();
    const char* GetZapdStr();
};

void Extractor::ShowSizeErrorBox() {
    char boxBuffer[MAX_PATH + 100];
#ifdef _WIN32
    snprintf(boxBuffer, MAX_PATH, "The rom file %ls was not a valid size. Was %zu MB, expecting 32, 54, or 64MB.",
             mCurrentRomPath.c_str(), mCurRomSize / MB_BASE);
    MessageBoxA(nullptr, boxBuffer, "Invalid Rom Size", MB_ICONERROR | MB_OK);
#else
    snprintf(boxBuffer, MAX_PATH, "The rom file %s was not a valid size. Was %zu MB, expecting 32, 54, or 64MB.",
             mCurrentRomPath.c_str(), mCurRomSize / MB_BASE);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid Rom Size", boxBuffer, nullptr);
#endif
}

void Extractor::ShowCrcErrorBox() {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Rom CRC invalid",
                             "Rom CRC did not match the list of known good roms. Please find another.", nullptr);
}

int Extractor::ShowRomPickBox(uint32_t verCrc) {
    char buffer[MAX_PATH + 100];
    SDL_MessageBoxData boxData = { 0 };
    SDL_MessageBoxButtonData buttons[3] = { { 0 } };
    int ret;

    buttons[0].buttonid = 0;
    buttons[0].text = "Yes";
    buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    buttons[1].buttonid = 1;
    buttons[1].text = "No";
    buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    boxData.numbuttons = 2;
// TODO: File box on other platforms.
#ifdef _WIN32
    buttons[2].buttonid = 2;
    buttons[2].text = "Find ROM";
    boxData.numbuttons = 3;
#endif
    boxData.flags = SDL_MESSAGEBOX_INFORMATION;
    boxData.message = buffer;
    boxData.title = "Rom Detected";
    boxData.window = nullptr;

    boxData.buttons = buttons;

    snprintf(buffer, sizeof(buffer), "Rom detected: %ls, Header CRC32: %8X. It appears to be: %s. Use this rom?",
             mCurrentRomPath.c_str(), verCrc, verMap.at(verCrc));
    SDL_ShowMessageBox(&boxData, &ret);
    return ret;
}

void Extractor::SetRomInfo(const FsPath& path) {
    mCurrentRomPath = path;
    mCurRomSize = GetCurRomSize();
}

void Extractor::GetRoms(std::vector<FsPath>& roms) {
#ifdef _WIN32
    WIN32_FIND_DATA ffd;
    HANDLE h = FindFirstFile(L"\\*", &ffd);

    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            wchar_t* ext = PathFindExtension(ffd.cFileName);
            if ((wcscmp(ext, L".z64") == 0) || (wcscmp(ext, L".v64") == 0) || (wcscmp(ext, L".n64") == 0))
                roms.push_back(ffd.cFileName);
        }
    } while (FindNextFile(h, &ffd) != 0);
#else
    for (const auto& file : std::filesystem::directory_iterator("./")) {
        if (file.is_directory())
            continue;
        if ((file.path().extension() == ".n64") || (file.path().extension() == ".z64") ||
            (file.path().extension() == ".v64")) {
            roms.push_back((file.path()));
        }
    }
#endif
}

#ifdef _WIN32
bool Extractor::GetRomPathFromBox() {
    OPENFILENAME box = { 0 };
    wchar_t nameBuffer[512];
    nameBuffer[0] = 0;

    box.lStructSize = sizeof(box);
    box.lpstrFile = nameBuffer;
    box.nMaxFile = sizeof(nameBuffer) / sizeof(nameBuffer[0]);
    box.lpstrTitle = L"Open Rom";
    box.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    box.lpstrFilter = L"N64 Roms\0*.z64;*.v64;*.n64\0\0";
    if (!GetOpenFileName(&box)) {
        DWORD err = CommDlgExtendedError();
        // GetOpenFileName will return 0 but no error is set if the user just closes the box.
        if (err != 0) {
            const char* errStr = nullptr;
            switch (err) {
                case FNERR_BUFFERTOOSMALL:
                    errStr = "Path buffer too small. Move file closer to root of your drive";
                    break;
                case FNERR_INVALIDFILENAME:
                    errStr = "File name for rom provided is invalid.";
                    break;
                case FNERR_SUBCLASSFAILURE:
                    errStr = "Failed to open a filebox because there is not enough RAM to do so.";
                    break;
            }
            MessageBoxA(nullptr, "Box Error", errStr, MB_OK | MB_ICONERROR);
            return false;
        }
    }
    // The box was closed without something being selected.
    if (nameBuffer[0] == 0) {
        return false;
    }
    mCurrentRomPath = nameBuffer;
    mCurRomSize = GetCurRomSize();
    return true;
}
#endif
uint32_t Extractor::GetRomVerCrc() {
    return _byteswap_ulong(((uint32_t*)mRomData.get())[4]);
}

size_t Extractor::GetCurRomSize() {
    return std::filesystem::file_size(mCurrentRomPath);
}

bool Extractor::ValidateAndFixRom() {
    if (GetRomVerCrc() == OOT_PAL_GC_MQ_DBG) {
        mRomData[0x3E] = 'P';
    }
    const uint32_t actualCrc = CRC32C(mRomData.get(), mCurRomSize);

    for (const uint32_t crc : goodCrcs) {
        if (actualCrc == crc) {
            return true;
        }
    }
    return false;
}

bool Extractor::ValidateRomSize() {
    if (mCurRomSize != MB32 && mCurRomSize != MB54 && mCurRomSize != MB64) {
        return false;
    }
    return true;
}

bool Extractor::ValidateRom(bool skipCrcTextBox) {
    if (!ValidateRomSize()) {
        ShowSizeErrorBox();
        return false;
    }
    if (!ValidateAndFixRom()) {
        if (!skipCrcTextBox) {
            ShowCrcErrorBox();
        }
        return false;
    }
    return true;
}

bool Extractor::Run() {
    std::vector<FsPath> roms;
    std::ifstream inFile;
    uint32_t verCrc;

    GetRoms(roms);

    if (roms.empty()) {
#ifdef _WIN32
        int ret = MessageBoxA(nullptr, "No roms found in this folder. Search for one somewhere else?", "No roms found",
                              MB_YESNO | MB_ICONERROR);
        switch (ret) {
            case IDYES:
                if (!GetRomPathFromBox()) {
                    MessageBoxA(nullptr, "No rom selected. Exiting", "No rom selected", MB_OK | MB_ICONERROR);
                    return false;
                }
                inFile.open(mCurrentRomPath, std::ios::in | std::ios::binary);
                if (!inFile.is_open()) {
                    return false; // TODO Handle error
                }
                inFile.read((char*)mRomData.get(), mCurRomSize);
                if (!ValidateRom()) {
                    return false;
                }
                break;
            case IDNO:
                return false;
            default:
                ASSUME(0);
        }
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "No roms found",
                                 "No roms found in this folder. Please move one here.", nullptr);
#endif
    }

    for (const auto& rom : roms) {
        int option;
        SetRomInfo(rom);
        if (inFile.is_open()) {
            inFile.close();
        }
        inFile.open(rom, std::ios::in | std::ios::binary);
        if (!ValidateRomSize()) {
            ShowSizeErrorBox();
            continue;
        }
        inFile.read((char*)mRomData.get(), mCurRomSize);
        RomToBigEndian(mRomData.get(), mCurRomSize);
        verCrc = GetRomVerCrc();

        // Rom doesn't claim to be valid
        if (!verMap.contains(verCrc)) {
            continue;
        }

        option = ShowRomPickBox(verCrc);
        if (option == (int)ButtonId::YES) {
            if (!ValidateRom(true)) {
                if (rom == roms.back()) {
                    ShowCrcErrorBox();
                } else {
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_ERROR, "Rom CRC invalid",
                        "Rom CRC did not match the list of known good roms. Trying the next one...", nullptr);
                }
                continue;
            }
            break;
        } 
#ifdef _WIN32
else if (option == (int)ButtonId::FIND) {
            if (!GetRomPathFromBox()) {
                MessageBoxA(nullptr, "No rom selected. Exiting", "No rom selected", MB_OK | MB_ICONERROR);
                return false;
            }
            inFile.open(mCurrentRomPath, std::ios::in | std::ios::binary);
            if (!inFile.is_open()) {
                return false; // TODO Handle error
            }
            inFile.read((char*)mRomData.get(), mCurRomSize);
            if (!ValidateRom()) {
                return false;
            }
            break;
}
#endif
         else if (option == (int)ButtonId::NO) {
            inFile.close();
            if (rom == roms.back()) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "No rom provided", "No rom provided. Exiting", nullptr);
                return false;
            }
            continue;
        }
        break;
    }
    return true;
}

bool Extractor::IsMasterQuest() {
    switch (GetRomVerCrc()) {
        case OOT_PAL_GC_MQ_DBG:
            return true;
        case OOT_PAL_GC:
        case OOT_PAL_GC_DBG1:
            return false;
    }
}

const char* Extractor::GetZapdVerStr() {
    switch (GetRomVerCrc()) {
        case OOT_PAL_GC:
            return "GC_NMQ_PAL_F";
        case OOT_PAL_GC_DBG1:
            return "GC_NMQ_D";
        case OOT_PAL_GC_MQ_DBG:
            return "GC_MQ_D";
        default:
            // We should never be in a state where this path happens.
            ASSUME(0);
    }
}

const char* Extractor::GetZapdStr() {
    constexpr size_t ZAPD_STR_SIZE = 1024;
    char* zapdCall = new char[ZAPD_STR_SIZE];
    const char* verStr = GetZapdVerStr();

    // TODO anything would be better than this
    snprintf(
        zapdCall, ZAPD_STR_SIZE,
        "ed -i assets/extractor/xmls/%s -b %ls -fl assets/extractor/filelists -o placeholder -osf placeholder -gsf "
        "1 -rconf assets/extractor/Config_%s.xml -se OTR --otrfile %s",
        verStr, mCurrentRomPath.c_str(), verStr, IsMasterQuest() ? "oot-mq.otr" : "oot.otr");

    return zapdCall;
}

int main() {
    Extractor e;
    bool valid = e.Run();
    const char* zapd = e.GetZapdStr();
    printf("ZAPD: %s", zapd);
}
