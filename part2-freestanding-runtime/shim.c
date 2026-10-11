/* shim.c - freestanding entry point. Redirected printf calls land in instr_printf,
 * which decodes the varargs and hands them to a Lua function that does the format. */
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdarg.h>
#include "render_bc.h"

long fs_write(long fd, const void *b, unsigned long n);   /* mini_libc.c */
extern void *realloc(void*, unsigned long);
extern void  free(void*);

static void *lalloc(void *ud, void *ptr, size_t osize, size_t nsize){
    (void)ud; (void)osize;
    if (nsize == 0){ free(ptr); return 0; }
    return realloc(ptr, nsize);
}
static int l_emit(lua_State *L){
    size_t n; const char *s = luaL_checklstring(L, 1, &n);
    fs_write(1, s, (unsigned long)n);
    return 0;
}
static int in_set(const char *set, char c){ for(;*set;set++) if(*set==c) return 1; return 0; }
static void norm_fmt(const char *fmt, char *out, unsigned long cap){
    unsigned long o=0; const char *CONV="diouxXeEfFgGaAcsp", *LEN="hlLzjtq";
    for(const char *p=fmt;*p;p++){
        if(o+2>=cap) break;
        if(*p!='%'){ out[o++]=*p; continue; }
        out[o++]='%'; p++;
        if(*p=='%'){ out[o++]='%'; continue; }
        while(*p && !in_set(CONV,*p)){ if(!in_set(LEN,*p)&&o+1<cap) out[o++]=*p; p++; }
        if(*p && o+1<cap) out[o++]=*p;
    }
    out[o<cap?o:cap-1]=0;
}

static lua_State *L = 0;
static int ref = LUA_NOREF;
static void init(void){
    L = lua_newstate(lalloc, 0);
    if(!L){ fs_write(2,"no state\n",9); return; }
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L,1);
    lua_pushcfunction(L, l_emit); lua_setglobal(L, "emit");
    if(luaL_loadbufferx(L,(const char*)RENDER_BC,RENDER_BC_LEN,"render","b")!=LUA_OK
       || lua_pcall(L,0,1,0)!=LUA_OK){ fs_write(2,"load fail\n",10); return; }
    ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

int instr_printf(const char *fmt, ...){
    if(!L) init();
    if(ref==LUA_NOREF) return -1;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    char nfmt[1024]; norm_fmt(fmt, nfmt, sizeof nfmt);
    lua_pushstring(L, nfmt);
    int nargs=1;
    va_list ap; va_start(ap,fmt);
    for(const char *p=fmt;*p;p++){
        if(*p!='%') continue; p++; if(*p=='%') continue;
        int lng=0;
        while(*p && !in_set("diouxXeEfFgGaAcsp",*p)){
            if(*p=='l') lng++; else if(*p=='j'||*p=='z'||*p=='t') lng=2; p++; }
        if(!*p) break;
        char c=*p;
        if(c=='d'||c=='i'||c=='o'||c=='u'||c=='x'||c=='X'){
            long long v=(lng>=2)?va_arg(ap,long long):(lng==1)?(long long)va_arg(ap,long):(long long)va_arg(ap,int);
            lua_pushinteger(L,(lua_Integer)v); nargs++;
        } else if(c=='c'){ lua_pushinteger(L, va_arg(ap,int)); nargs++; }
        else if(c=='e'||c=='E'||c=='f'||c=='F'||c=='g'||c=='G'||c=='a'||c=='A'){ lua_pushnumber(L, va_arg(ap,double)); nargs++; }
        else if(c=='s'){ lua_pushstring(L, va_arg(ap,const char*)); nargs++; }
        else if(c=='p'){ lua_pushlightuserdata(L, va_arg(ap,void*)); nargs++; }
    }
    va_end(ap);
    if(lua_pcall(L,nargs,0,0)!=LUA_OK){
        const char *e=lua_tostring(L,-1);
        fs_write(2,"[instr_printf] ",15); if(e) fs_write(2,e,(unsigned long)__builtin_strlen(e));
        fs_write(2,"\n",1); lua_pop(L,1); return -1;
    }
    return 0;
}
