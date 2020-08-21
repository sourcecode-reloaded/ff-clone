char *room_codename;

int script_openpng(lua_State *L);
int script_set_codename(lua_State *L);
void open_level(char *codename);
void open_user_level();
void end_level();
void refresh_user_level();

int script_flip_object(lua_State *L);
int script_start_active_fish(lua_State *L);
