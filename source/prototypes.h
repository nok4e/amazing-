
namespace prototype {
	
	using void_t = void(__cdecl*)();
	using wnd_proc_t = HRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM);
	using init_game_instance_t = HWND(__cdecl*)(HINSTANCE);
	using set_world_time_t = void(__thiscall*)(void*, char, char);
	using force_weather_t = void(__cdecl*)(short);
	using blend_animation_t = void* (__cdecl*)(void*, int, int, float);
}