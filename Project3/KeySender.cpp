#include "KeySender.h"

KeySender::~KeySender(){
	Stop();
}

void KeySender::AddKeySpam(DWORD vkCode, bool is_add) {
	std::lock_guard<std::mutex> lk(ActiveKey.lock);
	if (is_add) {
		ActiveKey.data.insert(vkCode);
	}
	else
		ActiveKey.data.erase(vkCode);
}

void KeySender::AddKeySpam(DWORD vkCode, bool is_add, std::chrono::milliseconds tSleep) {
	std::lock_guard<std::mutex> lk(stDelayKey.lock);
	stDelayKey.DellayKeys.insert({ tSleep, { vkCode, is_add } });
}

void KeySender::tAddKeySpam() {

	while (!tsSend.stop) {

		std::multimap< std::chrono::milliseconds, DelayKey>::iterator bDKey;
		//std::multimap< std::chrono::milliseconds, DelayKey>::const_iterator CIteratorKey;

		std::lock_guard<std::mutex> lk(stDelayKey.lock);
		if (stDelayKey.DellayKeys.empty()) {
			continue;
		}
		bDKey = stDelayKey.DellayKeys.begin();
		std::this_thread::sleep_for(bDKey->first);
		AddKeySpam(bDKey->second.vkCode, bDKey->second.is_add);
		stDelayKey.DellayKeys.erase(bDKey);
		//if(bDKey->second.is_add)
		//	ActiveKey.data.insert(bDKey->second.vkCode);
		//else
		//	ActiveKey.data.erase(bDKey->second.vkCode);
		//CIteratorKey = stDelayKey.DellayKeys.begin();
	};
}

void KeySender::RemoveKeySpam(DWORD vkCode) {
	std::lock_guard<std::mutex> lk(ActiveKey.lock);
	ActiveKey.data.erase(vkCode);
}

void KeySender::SendAllKeys(const std::set<DWORD>& vkCodes){
	std::vector<KeyAction> keyActions;
	for (const DWORD vkCode : vkCodes) {
		keyActions.push_back({ vkCode, true });
		keyActions.push_back({ vkCode, false });
	}
	KeyInput(keyActions);
}

void KeySender::Loop() {
	while (!tSend.stop) {
		SendAllKeys(ActiveKey.data);
		std::this_thread::sleep_for(tSend.interval);
	}
}

void KeySender::Start() {
	if (tSend.thread.joinable())
		tSend.thread.join();
	if (tsSend.thread.joinable())
		tsSend.thread.join();
	auto self = shared_from_this();
	tSend.thread = std::thread(&KeySender::Loop, self);
	tsSend.thread = std::thread(&KeySender::tAddKeySpam, self);
}

void KeySender::Stop() {
	tSend.stop = true;
	tsSend.stop = true;
	if (tSend.thread.joinable())
		tSend.thread.join();
	if (tsSend.thread.joinable())
		tsSend.thread.join();
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