# UCP WinProc Handler

This repository contains a module for the "Unofficial Crusader Patch Version 3" (UCP3), a modification for Stronghold Crusader.
The module allows to register a modified WindowProc function via other module code to chain functions between the incoming messages and Crusaders real WindowProc.

### Usage

The API is provided via exported C functions and can be accessed via the `ucp.dll`-API, or from lua `modules.winProcHandler.cinterface()`. 
To make using the module easier, the header [winProcHelper.h](ucp_winProcHandler/ucp_winProcHandler/winProcHandler.h) can be copied into your project.  
It is used by calling the function *initModuleFunctions()* in your module once after it was loaded, for example in the lua require-call. It tries to receive the provided functions and returns *true* if successful. For this to work, winProcHandler needs to be a dependency.
The provided functions are the following:

* *GetMainProc()*  
  Simply returns a function pointer to the entry function that Windows calls, which will be followed by all here registered listeners. Uses the normal WindowProc structure. Can, for example, be used to send fake messages.
* *RegisterProc(FuncCallNextProc funcToCall, int priority)*  
  Called with the pointer to a *CallNextProc* function and a priority integer. Tries to place the function in the pipeline. Lower priority values mean earlier. If the value is taken, the next free after that is used. The return value is the actual priority. A return of NO_VALID_PRIO indicates, that the function could not be placed.
* *CallNextProc(int reservedCurrentPrio, HWND, UINT, WPARAM, LPARAM)*  
  Value 2 to 5 are equal to the real WindowProc, the first, however, is more or less reserved. It will contain the priority of the function and needs to be send to the next *CallNextProc* unchanged.

In general, a user of this module would need to create a function of the type *CallNextProc* and register it. Every arriving message that should not be devoured by the using module needs to call *CallNextProc* to send the messages further. Every value can be changed. The first should be send further without changes. A simple lookup should send all received values further. Note, that a LRESULT needs to be returned. If there is no specific reason why not, this should be the value returned by the call to *CallNextProc*.

### Special Thanks

To all of the UCP Team, the [Ghidra project](https://github.com/NationalSecurityAgency/ghidra) and
of course to [Firefly Studios](https://fireflyworlds.com/), the creators of Stronghold Crusader.