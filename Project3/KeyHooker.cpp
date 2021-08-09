#include "KeyHooker.h"

KeyHooker::KeyHandler KeyHooker::Macros;
KeyHooker* KeyHooker::g_this;
std::set<DWORD> KeyHooker:: PressedKey;

KeyHooker* KeyHooker::GetInstance(KeyHandler cbKeyHandler)
{
    if (g_this == nullptr) {
        if (cbKeyHandler) {
            g_this = new KeyHooker(cbKeyHandler);
        }
        else {
            throw std::exception("KeyHooks::GetInstance failed.");
        }
    }
    return g_this;
}

KeyHooker::KeyHooker(KeyHandler cbKeyHandler) {
	Macros = cbKeyHandler;
}

KeyHooker::~KeyHooker(){
	Unhook();
}

//enum Keys
//{
//	ShiftKey = 16,
//	Capital = 20,
//};
//
//int shift_active() {
//	return GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0;
//}
//
//int capital_active() {
//	return (GetKeyState(VK_CAPITAL) & 1) == 1;
//}
//LRESULT KeyHooker::keyboard_hook(const int nCode, const WPARAM wParam, const LPARAM lParam)
//{
//	if (nCode != HC_ACTION)
//		return CallNextHookEx(NULL, nCode, wParam, lParam);
//
//	bool bEatKey = false;
//	const PKBDLLHOOKSTRUCT& p = (PKBDLLHOOKSTRUCT)lParam;
//	KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
//	DWORD wVirtKey = kbdStruct->vkCode;
//	DWORD wScanCode = kbdStruct->scanCode;
//
//	if (!(p->flags & LLKHF_INJECTED)) {
//		if (wParam == WM_KEYDOWN) {
//			BYTE lpKeyState[256];
//			GetKeyboardState(lpKeyState);
//			lpKeyState[Keys::ShiftKey] = 0;
//			lpKeyState[Keys::Capital] = 0;
//			if (shift_active()) {
//				lpKeyState[Keys::ShiftKey] = 0x80;
//			}
//			if (capital_active()) {
//				lpKeyState[Keys::Capital] = 0x01;
//			}
//
//			char result;
//			ToAscii(wVirtKey, wScanCode, lpKeyState, (LPWORD)&result, 0);
//		}
//	}
//
//	p->flags &= ~LLKHF_INJECTED;
//
//	return CallNextHookEx(hHook, nCode, wParam, lParam);
//}

LRESULT KeyHooker::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	bool hooked = false;
	const PMSLLHOOKSTRUCT& p = (PMSLLHOOKSTRUCT)lParam;

	if (!(p->flags & LLMHF_INJECTED)) {

		bool is_down = true;
		DWORD vkCode = 0;

		switch (wParam) {
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
		switch (wParam) {
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

void KeyHooker::Hook()
{
	//hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, 0);
	//if (!hhkLowLevelKybd)
	//	throw std::exception("KeyHooks::Hook failed.");

	hhkLowLevelMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
	if (!hhkLowLevelMouse)
		throw std::exception("MouseHooks::Hook failed.");
}

void KeyHooker::Unhook(){
	//if (!UnhookWindowsHookEx(hhkLowLevelKybd))
	//	throw std::exception("KeyHooks::Unhook failed.");
	if (!UnhookWindowsHookEx(hhkLowLevelMouse))
		throw std::exception("KeyHooks::Unhook failed.");
}
