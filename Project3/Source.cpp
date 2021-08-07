#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <iostream>
#include <set>
#include <thread>
#include <vector>

HHOOK hHook{ NULL };

HHOOK hhkLowLevelKybd;
HHOOK hhkLowLevelMouse;

std::set<DWORD> ActiveKey;
std::set<DWORD> PressedKey;

const int INTERVAL_MS = 15;

struct KeyAction {
	KeyAction(DWORD vkCode, bool state)
		: vkCode(vkCode), state(state) {}
	DWORD vkCode;
	bool state;
};

struct {
	std::thread thread;
	std::chrono::milliseconds interval{ INTERVAL_MS };
	bool stop = false;
} tSend;

enum Keys
{
	ShiftKey = 16,
	Capital = 20,
};

int shift_active() {
	return GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0;
}

int capital_active() {
	return (GetKeyState(VK_CAPITAL) & 1) == 1;
}

void AddKeySpam(DWORD vkCode, bool is_down) {
	if (is_down) {
		ActiveKey.insert(vkCode);
	}
	else
		ActiveKey.erase(vkCode);
}

void RemoveKeySpam(DWORD vkCode) {
	ActiveKey.erase(vkCode);
}

void KeyInput(const std::vector<KeyAction>& keyActions) {

	const int inSize = (UINT)keyActions.size();
	std::vector<INPUT> inputs(inSize);

	for (UINT i = 0; i < inSize; i++) {

		DWORD vkCode = keyActions[i].vkCode;
		bool IsDown = keyActions[i].state;
		DWORD dwFlags = 0;

		switch (vkCode)
		{
		case VK_LBUTTON:
			dwFlags = IsDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
			break;
		case VK_RBUTTON:
			dwFlags = IsDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
			break;
		case VK_MBUTTON:
			dwFlags = IsDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
			break;
		default:
			inputs[i].type = INPUT_KEYBOARD;
			inputs[i].ki.wVk = (WORD)vkCode;
			inputs[i].ki.wScan = (WORD)MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
			inputs[i].ki.dwFlags = IsDown ? 0 : KEYEVENTF_KEYUP;
			break;
		}
		if (dwFlags != 0) {
			inputs[i].type = INPUT_MOUSE;
			inputs[i].mi.dwFlags = dwFlags;
		}
	}
	UINT uSent = SendInput(inSize, inputs.data(), sizeof(INPUT));
	if (uSent != inSize)
		throw std::exception("SendMouseKey failed.");
}

void SendAllKeys(const std::set<DWORD>& vkCodes)
{
	std::vector<KeyAction> keyActions;
	for (const DWORD vkCode : vkCodes) {
		keyActions.push_back({ vkCode, true });
		keyActions.push_back({ vkCode, false });
	}
	KeyInput(keyActions);
}

bool Macros(DWORD vkCode, DWORD time, bool is_down) {

	static bool xbActive = false;

	switch (vkCode)
	{
	case VK_XBUTTON2:
		xbActive = is_down;
		AddKeySpam('E', is_down);
		return true;
	default:
		if (xbActive) {
			RemoveKeySpam('E');
			switch (vkCode) {
			case VK_RBUTTON:
				//RemoveKeySpam('E');
				AddKeySpam(VK_RBUTTON, is_down);
				return true;
			case VK_LBUTTON:
				AddKeySpam(VK_LBUTTON, is_down);
				return true;
			}
		}
		else {
			switch (vkCode) {
			case VK_XBUTTON1:
				AddKeySpam('V', is_down);
				return true;
			
			//case VK_RBUTTON:
			//	AddKeySpam(VK_RBUTTON, is_down );
			//	break;
			//case VK_LBUTTON:
			//	AddKeySpam(VK_LBUTTON, is_down);
			//	break;
			}
		}
		break;
	}
	return false;
}

LRESULT CALLBACK keyboard_hook(const int nCode, const WPARAM wParam, const LPARAM lParam)
{
	if (nCode != HC_ACTION)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	bool bEatKey = false;
	const PKBDLLHOOKSTRUCT& p = (PKBDLLHOOKSTRUCT)lParam;
	KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
	DWORD wVirtKey = kbdStruct->vkCode;
	DWORD wScanCode = kbdStruct->scanCode;

	if (!(p->flags & LLKHF_INJECTED)) {
		if (wParam == WM_KEYDOWN) {
			BYTE lpKeyState[256];
			GetKeyboardState(lpKeyState);
			lpKeyState[Keys::ShiftKey] = 0;
			lpKeyState[Keys::Capital] = 0;
			if (shift_active()) {
				lpKeyState[Keys::ShiftKey] = 0x80;
			}
			if (capital_active()) {
				lpKeyState[Keys::Capital] = 0x01;
			}

			char result;
			ToAscii(wVirtKey, wScanCode, lpKeyState, (LPWORD)&result, 0);
		}
	}

	p->flags &= ~LLKHF_INJECTED;

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	bool hooked = false;
	const PMSLLHOOKSTRUCT& p = (PMSLLHOOKSTRUCT)lParam;

	if (!(p->flags & LLMHF_INJECTED)){

		bool is_down = true;
		DWORD vkCode = 0;

		switch (wParam){
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
			vkCode = VK_LBUTTON;
			break;
		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
			vkCode = VK_RBUTTON;
			break;
		case WM_MBUTTONUP:
		case WM_MBUTTONDOWN:
			vkCode = VK_MBUTTON;
			break;
		case WM_XBUTTONUP:
		case WM_XBUTTONDOWN:
			switch (p->mouseData >> 16) {
			case XBUTTON1:
				vkCode = VK_XBUTTON1;
				break;
			case XBUTTON2:
				vkCode = VK_XBUTTON2;
				break;
			}
			break;
		}
		switch (wParam){
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			is_down = false;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			if (is_down) {
				if (!PressedKey.count(vkCode))
					hooked = Macros(vkCode, p->time, is_down);
				PressedKey.insert(vkCode);
			}
			else {
				PressedKey.erase(vkCode);
				hooked = Macros(vkCode, p->time, is_down);
			}

			break;
		}
	}

	p->flags &= ~LLMHF_INJECTED;

	return hooked ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Hook()
{
	//hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, 0);
	//if (!hhkLowLevelKybd)
	//	throw std::exception("KeyHooks::Hook failed.");

	hhkLowLevelMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
	if (!hhkLowLevelMouse)
		throw std::exception("MouseHooks::Hook failed.");
}

void Unhook()
{
	tSend.stop = true;
	//if (!UnhookWindowsHookEx(hhkLowLevelKybd))
	//	throw std::exception("KeyHooks::Unhook failed.");
	if (!UnhookWindowsHookEx(hhkLowLevelMouse))
		throw std::exception("KeyHooks::Unhook failed.");
}


void Await() {
	MSG msg;
	while (!GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Loop() {
	while (!tSend.stop) {
		SendAllKeys(ActiveKey);
		std::this_thread::sleep_for(tSend.interval);
	}
}

void Start() {
	tSend.thread = std::thread(Loop);
}

int main() {
	struct raii {
		raii() {}
		~raii() { Unhook(); }
	}name;
	int exit = 0;

	Hook();

	Start();

	Await();

	return 0;
}