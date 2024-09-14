#include "plugin.h"

using namespace std::placeholders;

bool afk = false;
char time_value = -1;
char weather_value = -1;

#define PREFIX "{c43d5c}amazing++ "


kthook::kthook_signal<prototype::init_game_instance_t> hook_game_instance { 0x745560 };


c_plugin::c_plugin() : inited(false) {

    update_hook.set_cb(std::bind(&c_plugin::update, this, _1));
    update_hook.install();
}


HWND game_window_handle = []() {

    HWND* window_handle = *reinterpret_cast<HWND**>(0xC17054);

    if (window_handle != nullptr)
        return *window_handle;
    
    hook_game_instance.after += [](const auto& hook, HWND& return_hwnd, HINSTANCE instance) { game_window_handle = return_hwnd; };

    return HWND(0);
}();


bool c_plugin::on_receive_rpc(unsigned char& _id, RakNet::BitStream* bs) {

    if (_id == 252) {

        uint8_t cef_packet_type;

        bs->Read(cef_packet_type);
        bs->Read(cef_app_set_mode);

        bs->Read(strange_byte);

        bs->IgnoreBits(16); // 0x0 / 0x0
    }

    return true;
}


bool c_plugin::on_send_rpc(int& _id, RakNet::BitStream* bs, PacketPriority& priority, PacketReliability& reliability, char& ord_channel, bool& sh_timestamp) {

    if (_id == 115) {

        bool give_take;
        uint16_t player_id;
        float damage_amount;

        bs->Read(give_take);
        bs->Read(player_id);
        bs->Read(damage_amount);

        uint32_t weapon_id;
        bs->Read(weapon_id);
        
        if (weapon_id == 54)
            return false;
    }

    if (_id == 252) {

        uint8_t cef_packet_type;

        bs->Read(cef_packet_type);
        bs->Read(cef_app_set_mode);

        bs->Read(strange_byte);

        bs->IgnoreBits(16); // 0x0 / 0x0

        std::string packet_name = read_with_size<uint16_t>(bs);

        if (packet_name != xorstr_("cef_rpc_auth"))
            return true;

        auto packet_data_offset = bs->GetReadOffset();

        std::string packet_data = read_with_size<uint16_t>(bs);

        if (!packet_data.ends_with(0xC))
            return true;
        
        std::regex pattern(xorstr_("(data)(.*?)(ўid)"));
        std::string promocode(xorstr_("©teslochan"));
        
        auto new_packet_data = std::regex_replace(packet_data, pattern, "$1" + promocode + "$3");

        bs->SetWriteOffset(packet_data_offset);

        write_with_size<uint16_t>(bs, new_packet_data);
    }

    return true;
}


bool c_plugin::on_send_packet(RakNet::BitStream* bs, PacketPriority& priority, PacketReliability& reliability, char& ord_channel) {

    bs->ResetReadPointer();

    uint8_t _id;
    bs->Read(_id);

    if (_id == 223) {

        uint8_t type;
        bs->Read(type);

        if (afk && (type == 80 || type == 12))
            return false;

        if (type == 24 || type == 95)
            return false;
    }

    return true;
}


void c_plugin::set_world_time(const decltype(set_world_time_hook)& hook, void* game, char h, char m) {
    
    if (time_value < 0)
        return hook.get_trampoline()(game, h, m);
    else if (time_value != h)
        return hook.get_trampoline()(game, time_value, m);

    return hook.get_trampoline()(game, h, m);
}


void c_plugin::update(const decltype(update_hook)& hook) {
    
    if (!inited && c_chat::get()->ref() != nullptr && rakhook::initialize()) {

        c_chat::get()->ref()->add_message(-1, PREFIX "by{ffffff} waparabka");

        c_input::get()->ref()->add_command("st", [](const char* p) {

            auto h = atoi(p);

            if (h < 0) {

                c_chat::get()->ref()->add_message(-1, PREFIX "{ffffff}Возвращено серверное время");

                time_value = -1;

                return;
            }
                
            if (h > 24) {

                c_chat::get()->ref()->add_message(-1, PREFIX "{ffffff}Установить время можно от 0 и до 24 (-1 что бы вернуть серверное)");

                return;
            }

            time_value = h;
            c_game::get()->ref()->set_world_time(time_value, 0);
            
            c_chat::get()->ref()->add_message(-1, std::string(PREFIX "{ffffff}Установлено новое время: " + std::to_string(time_value)).c_str());
        });

        c_input::get()->ref()->add_command("sw", [](const char* p) {

            auto w = atoi(p);

            if (w < 0) {

                c_chat::get()->ref()->add_message(-1, PREFIX "{ffffff}Возвращена серверная погода");

                weather_value = -1;

                return;
            }

            if (w > 255) {
                
                c_chat::get()->ref()->add_message(-1, PREFIX "{ffffff}Установить погоду можно от 0 и до 255 (-1 что бы вернуть серверное)");

                return;
            }
            
            weather_value = w;
            c_game::get()->ref()->force_weather_now(weather_value);

            c_chat::get()->ref()->add_message(-1, std::string(PREFIX "{ffffff}Установлена новая погода: " + std::to_string(weather_value)).c_str());
        });

        c_input::get()->ref()->add_command("fd", [](const char* p) {

            auto d = atoi(p);

            if (d > 3600 || d < 0)
                return;
            
            *reinterpret_cast<float*>(0xB7C4F0) = d;
        });


        auto latest_wndproc_ptr = GetWindowLongPtrA(game_window_handle, GWLP_WNDPROC);

        wnd_proc_hook.set_dest(latest_wndproc_ptr);
        wnd_proc_hook.set_cb(std::bind(&c_plugin::wnd_proc_handler, this, _1, _2, _3, _4, _5));

        wnd_proc_hook.install();
        
        rakhook::on_receive_rpc += std::bind(&c_plugin::on_receive_rpc, this, _1, _2);
        rakhook::on_send_rpc += std::bind(&c_plugin::on_send_rpc, this, _1, _2, _3, _4, _5, _6);

        rakhook::on_send_packet += std::bind(&c_plugin::on_send_packet, this, _1, _2, _3, _4);
        
        set_world_time_hook.set_dest(reinterpret_cast<void*>( rakhook::samp_addr(0xA03A0) ));
        set_world_time_hook.set_cb(std::bind(&c_plugin::set_world_time, this, _1, _2, _3, _4));

        set_world_time_hook.install();

        inited = true;

    }

    return hook.get_trampoline()();
}


HRESULT c_plugin::wnd_proc_handler(const decltype(wnd_proc_hook)& hook, HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {

    switch (u_msg) {

        case WM_KEYDOWN: {

            if (w_param == 0x31) {

                c_input::get()->ref()->send("/anim 69");
            }
            
            if (w_param == 0x32)
                c_input::get()->ref()->send("/drugs");

            if (w_param == 0x33) {

                RakNet::BitStream* bs = new RakNet::BitStream();

                std::string packet_data { };

                packet_data.push_back(0x0E);

                packet_data.push_back(cef_app_set_mode);
                packet_data.push_back(strange_byte);

                packet_data.push_back(0x0);
                packet_data.push_back(0x0);
                
                packet_data.push_back(strlen("cef_inv_item_action"));
                packet_data.push_back(0x0);
                packet_data.append("cef_inv_item_action");

                std::vector<uint8_t> bytes { 0x28, 0x00, 0x85, 0xA6, 0x61, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x01, 0xA4, 0x61, 0x72, 0x65, 0x61,
                                             0x01, 0xA5, 0x69, 0x6E, 0x64, 0x65, 0x78, 0xC0, 0xA8, 0x72, 0x65, 0x73, 0x70, 0x6F, 0x6E, 0x73,
                                             0x65, 0x00, 0xA6, 0x73, 0x6C, 0x6F, 0x74, 0x49, 0x64, 0x06 };

                packet_data.insert(packet_data.end(), std::begin(bytes), std::end(bytes));

                for (const auto p : packet_data)
                    bs->Write(p);
                
                rakhook::send_rpc(252, bs, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, (char)0, false);

                delete bs;
            }

            if (w_param == 0x34) {

                bool* vehlods = reinterpret_cast<bool*>(0x52C9EE);
                
                *vehlods ^= true;
                
                c_chat::get()->ref()->add_message(-1, std::string(PREFIX "{ffffff}Сжатие машин " + std::string((*vehlods ? "включено" : "выключено"))).c_str());
            }
            
            if (w_param == 0x35) {
                
                afk ^= true;

                Beep(afk ? 1000 : 500, 100);
            }

            break;
        }
    }

    return hook.get_trampoline()(hwnd, u_msg, w_param, l_param);
}


c_plugin::~c_plugin() {

    update_hook.remove();
    set_world_time_hook.remove();
    hook_game_instance.remove();
    
    c_input::get()->ref()->remove_command("st");
    c_input::get()->ref()->remove_command("sw");
    c_input::get()->ref()->remove_command("fd");
    
    rakhook::destroy();
}

#undef PREFIX