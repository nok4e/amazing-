#pragma once

#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <regex>

#include "samp.h"
#include "prototypes.h"
#include "xorstr.hpp"

#include <kthook/kthook.hpp>
#include <RakHook/rakhook.hpp>
#include <RakNet/PacketEnumerations.h>


class c_plugin {

public:

	bool inited;
	bool running = false;

	uint8_t cef_app_set_mode;
	uint8_t strange_byte;

	c_plugin();
	~c_plugin();

	kthook::kthook_simple<prototype::void_t> update_hook { reinterpret_cast<void*>(0x561B10) };
	void update(const decltype(update_hook)& hook);
	
	kthook::kthook_simple<prototype::wnd_proc_t> wnd_proc_hook { };

	HRESULT wnd_proc_handler(const decltype(wnd_proc_hook)& hook, HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

	kthook::kthook_simple<prototype::set_world_time_t> set_world_time_hook { };

	void set_world_time(const decltype(set_world_time_hook)& hook, void* game, char h, char m);
	
	bool on_receive_rpc(unsigned char& _id, RakNet::BitStream* bs);
	bool on_send_rpc(int& _id, RakNet::BitStream* bs, PacketPriority& priority, PacketReliability& reliability, char& ord_channel, bool& sh_timestamp);
	
	bool on_send_packet(RakNet::BitStream* bs, PacketPriority& priority, PacketReliability& reliability, char& ord_channel);


	template <typename T>
	std::string read_with_size(RakNet::BitStream* bs) {
		T size;
		if (!bs->Read(size))
			return {};
		std::string str(size, '\0');
		bs->Read(str.data(), size);
		return str;
	}
	template <typename T>
	void write_with_size(RakNet::BitStream* bs, std::string_view str) {
		T size = static_cast<T>(str.size());
		bs->Write(size);
		bs->Write(str.data(), size);
	}
};