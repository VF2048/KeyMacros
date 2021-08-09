#include "KeySender.h"

KeySender::~KeySender()
{
	Stop();
}

void KeySender::AddKeySpam(DWORD vkCode, bool is_down) {
	if (is_down) {
		ActiveKey.insert(vkCode);
	}
	else
		ActiveKey.erase(vkCode);
}

void KeySender::RemoveKeySpam(DWORD vkCode) {
	ActiveKey.erase(vkCode);
}

void KeySender::SendAllKeys(const std::set<DWORD>& vkCodes)
{
	std::vector<KeyAction> keyActions;
	for (const DWORD vkCode : vkCodes) {
		keyActions.push_back({ vkCode, true });
		keyActions.push_back({ vkCode, false });
	}
	KeyInput(keyActions);
}

void KeySender::Loop() {
	while (!tSend.stop) {
		SendAllKeys(ActiveKey);
		std::this_thread::sleep_for(tSend.interval);
	}
}

void KeySender::Start() {
	auto self = shared_from_this();
	tSend.thread = std::thread(&KeySender::Loop, self);
}

void KeySender::Stop() {
	tSend.stop = true;
	if (tSend.thread.joinable())
		tSend.thread.join();
}

void KeySender::KeyInput(const std::vector<KeyAction>& keyActions) {

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