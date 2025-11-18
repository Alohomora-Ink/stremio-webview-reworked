#include "globals.h"

HINSTANCE g_hInst = nullptr;

std::wstring g_webuiUrl;
std::vector<std::wstring> g_webuiUrls = {
    L"https://localhost:3000/",
};

const std::vector<std::wstring> g_subtitleExtensions = {
    L".srt", L".vtt", L".ass", L".ssa"
};