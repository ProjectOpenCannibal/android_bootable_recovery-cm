int get_menu_selection(const char* const * headers, const char* const * items, int menu_only, int initial_selection, Device* device);
void wipe_data(int confirm, Device* device);
int erase_volume(const char *volume, bool force = false);