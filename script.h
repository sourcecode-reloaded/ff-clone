#include <lualib.h>
#include <lauxlib.h>

lua_State *luastat;
void initlua();
void *my_luaL_checkludata(lua_State *L, int narg);
void setluastring(lua_State *L, char *name, char *str);
void endlua();
