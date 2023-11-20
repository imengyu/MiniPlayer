#pragma once
#ifdef MINI_PLAYER_LIB

#define MINI_PLAYER_EXPORT __declspec(dllexport)
#define MINI_PLAYER_EXPORT_C(ret) EXTERN_C ret __declspec(dllexport)

#endif
