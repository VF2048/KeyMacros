#pragma once

#include <vector>
#include <Windows.h>
#include <set>
#include <thread>
#include <memory>

struct KeyAction {
	KeyAction(DWORD vkCode, bool state)
		: vkCode(vkCode), state(state) {}
	DWORD vkCode;
	bool state;
};

class KeySender : public std::enable_shared_from_this<KeySender>
{
public:
	KeySender() = default;
	~KeySender();

	void Start();
	void Stop();
	void AddKeySpam(DWORD vkCode, bool is_down);
	void RemoveKeySpam(DWORD vkCode);

private:
	void KeyInput(const std::vector<KeyAction>& keyActions);
	void SendAllKeys(const std::set<DWORD>& vkCodes);
	void Loop();

private:
	static const int INTERVAL_MS = 15;
	std::set<DWORD> ActiveKey;
	std::set<DWORD> PressedKey;

	struct {
		std::thread thread;
		std::chrono::milliseconds interval{ INTERVAL_MS };
		bool stop = false;
	} tSend;
};