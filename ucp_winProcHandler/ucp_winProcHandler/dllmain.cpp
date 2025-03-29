
#include "pch.h"

// lua
#include "lua.hpp"

#include "winProcHandler.h"

#include <map>

static WNDPROC realWinProc{ nullptr };
static std::map<int, WinProcHeader::FuncCallNextProc> procMap{};

/* helper */

static __declspec(naked) LRESULT __stdcall CallRealProcFunc(int, HWND, UINT, WPARAM, LPARAM)
{
  __asm
  {
    pop     ecx   // remove return address
    pop     eax   // removes index, since we are going to main func
    push    ecx   // add return address again

    jmp     realWinProc   // jump to actual main window proc
  }
}

static WinProcHeader::FuncCallNextProc __stdcall GetProcFuncAddress(int &indexInStack)
{
  auto nextFunc{ procMap.upper_bound(indexInStack) };

  while (nextFunc != procMap.end())
  {
    if (nextFunc->second != nullptr)
    {
      indexInStack = nextFunc->first; // set index in stack
      return nextFunc->second;
    }

    nextFunc = procMap.erase(nextFunc);
  }
  return CallRealProcFunc;  // index remains unchanged, since it is removed anyway
}


/* Export functions, CallNextProc */

extern "C" __declspec(dllexport, naked) LRESULT __stdcall CallNextProc(int, HWND, UINT, WPARAM, LPARAM)
{
  __asm
  {
    mov     eax, esp            // stack ptr in eax
    add     eax, 4              // ptr to prio index
    push    eax                 // push prio index ref
    call    GetProcFuncAddress  // get new func address in eax

    jmp     eax     // jump to next window proc
  }
}


/* Called window function */

static __declspec(naked) LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM)
{
  __asm
  {
    pop     ecx                                       // remove return address
    push    dword ptr WinProcHeader::NO_VALID_PRIO    // adds start prio
    push    ecx                                       // add return address again

    jmp     CallNextProc   // jump to actual main window proc
  }
}


/* Other export functions */

extern "C" __declspec(dllexport) WNDPROC __stdcall GetMainProc()
{
  return WindowProc;
}

extern "C" __declspec(dllexport) int __stdcall RegisterProc(WinProcHeader::FuncCallNextProc callNextProcFunc, int prio)
{
  while (prio != WinProcHeader::NO_VALID_PRIO)
  {
    auto res{ procMap.try_emplace(prio, callNextProcFunc) };
    if (std::get<1>(res)) // was it emplaced?
    {
      break;
    }
    ++prio;
  }
  return prio;
}



// lua module load
extern "C" __declspec(dllexport) int __cdecl luaopen_winProcHandler(lua_State * L)
{

  lua_newtable(L); // push a new table on the stack

  // need to write window callback func to the returned address
  lua_pushinteger(L, (DWORD)&realWinProc);
  lua_setfield(L, -2, "address_FillWithWindowProcCallback");

  // simple replace
  lua_pushinteger(L, (DWORD)WindowProc);
  lua_setfield(L, -2, "funcAddress_WindowProc");

  // Add extern function pointers for ease of use from lua
  lua_pushinteger(L, (DWORD)RegisterProc);
  lua_setfield(L, -2, "funcAddress_RegisterProc");

  lua_pushinteger(L, (DWORD)GetMainProc);
  lua_setfield(L, -2, "funcAddress_GetMainProc");

  lua_pushinteger(L, (DWORD)CallNextProc);
  lua_setfield(L, -2, "funcAddress_CallNextProc");
  
  return 1;
}