#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <ctime>
#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <map>
using std::min;
using std::max;

#ifdef ORIGINAL_GUI_TEST_INPUT
short OriginalGuiReadKeyState(int key);
#else
inline short OriginalGuiReadKeyState(int key) { return GetAsyncKeyState(key); }
#endif
