struct ff_image *generwall(int width, int height, const char *data);
struct ff_image *genersteel(int width, int height, const char *data);
struct ff_image *genernormal(int width, int height, const char *data);
int script_generwall(lua_State *L);
int script_genersteel(lua_State *L);
int script_genernormal(lua_State *L);
