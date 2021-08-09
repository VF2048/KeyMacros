#pragma once

#include <Windows.h>
#include <functional>
#include <set>

#include <exception>
#include <string>

class KeyHooker
{
public:
	typedef std::function<bool(DWORD, DWORD, bool)> KeyHandler;

public:
	static KeyHooker* GetInstance(KeyHandler cbKeyHandler = nullptr);

	void Hook();
	void Unhook();

private:
	KeyHooker(KeyHandler cbKeyHandler);
	~KeyHooker();
	//static LRESULT CALLBACK keyboard_hook(const int nCode, const WPARAM wParam, const LPARAM lParam);
	static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
	HHOOK hhkLowLevelKybd;
	HHOOK hhkLowLevelMouse;
	static KeyHooker* g_this;
	static KeyHandler Macros;
	static std::set<DWORD> PressedKey;

};

