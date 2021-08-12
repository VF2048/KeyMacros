//#include <stdio.h>
//#include <tchar.h>
//#include <iostream>
#include <Windows.h>

#include "KeyHooker.h"
#include "KeySender.h"

KeyHooker* hooker;
std::shared_ptr <KeySender> sender;

bool Macros(DWORD vkCode, DWORD time, bool is_down) {

	static bool xbActive = false;
	std::chrono::milliseconds interval{ 500 };

	switch (vkCode)
	{
	case VK_XBUTTON2:
		xbActive = is_down;
		sender->AddKeySpam('E', is_down);
		return true;
	default:
		if (xbActive) {
			sender->RemoveKeySpam('E');
			switch (vkCode) {
			case VK_RBUTTON:
				//RemoveKeySpam('E');
				sender->AddKeySpam(VK_RBUTTON, is_down);
				return true;
			case VK_LBUTTON:
				sender->AddKeySpam(VK_LBUTTON, is_down);
				return true;
			}
		}
		else {
			switch (vkCode) {
			case VK_XBUTTON1:
				sender->AddKeySpam('V', is_down);
				return true;
			case VK_MBUTTON:
				sender->AddKeySpam('F', is_down, interval);
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

void Await() {
	MSG msg;
	while (!GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

int main() {
	//struct raii {
	//	raii() {}
	//	~raii() { Unhook(); }
	//}name;
	//int exit = 0;

	hooker = KeyHooker::GetInstance(Macros);
	sender = std::make_shared<KeySender>();

	hooker->Hook();

	sender->Start();

	Await();

	return 0;
}