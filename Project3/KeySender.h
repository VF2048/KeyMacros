#pragma once

#include <vector>
#include <Windows.h>
#include <set>
#include <thread>
//#include <memory>
#include <map>
#include <mutex>

#include <functional>

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
	void AddKeySpam(DWORD vkCode, bool is_add);
	void AddKeySpam(DWORD vkCode, bool is_add, std::chrono::milliseconds tSleep);
	void tAddKeySpam();
	void RemoveKeySpam(DWORD vkCode);

private:
	void KeyInput(const std::vector<KeyAction>& keyActions);
	void SendAllKeys(const std::set<DWORD>& vkCodes);
	void Loop();

private:
	static const int INTERVAL_MS = 15;

	struct {
		std::set<DWORD> data;
		std::mutex lock;
	} ActiveKey;

	struct {
		std::thread thread;
		std::chrono::milliseconds interval{ INTERVAL_MS };
		bool stop = false;
	} tSend;

	struct {
		std::thread thread;
		bool stop = false;
	} tsSend;

	struct DelayKey {
		DWORD vkCode;
		bool is_add;
	};

	struct {
		std::multimap< std::chrono::milliseconds, DelayKey> DellayKeys;
		std::mutex lock;
	}stDelayKey;
};