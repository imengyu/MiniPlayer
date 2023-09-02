// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <mpg123.h>

HINSTANCE hInst;

BOOL APIENTRY DllMain(HMODULE hModule,
  DWORD  ul_reason_for_call,
  LPVOID lpReserved
)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    if (mpg123_init() != MPG123_OK) {
      MessageBox(NULL, L"Initialization error", L"Failed init mpg123!", MB_ICONERROR);
      return FALSE;
    }
    hInst = hModule;
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    mpg123_exit();
    break;
  }
  return TRUE;
}

