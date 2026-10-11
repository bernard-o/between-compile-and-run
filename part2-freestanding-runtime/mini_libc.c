/* mini_libc.c - a minimal freestanding libc, just enough to run Lua with no
 * system C library. Implements the hot path for real (mem/str, snprintf with
 * %d/%s/%f, the arena allocator). Stubs the cold / never-reached path (file I/O,
 * pow, a full strtod). No constructors: everything is static (the blob is
 * injected raw, with no __libc_init to run). */

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include "ctype_table.h"

/* ---------------- syscalls ---------------- */
static long _wr(long fd, const void *b, unsigned long n){
    long r; __asm__ volatile("syscall":"=a"(r):"a"(1),"D"(fd),"S"(b),"d"(n):"rcx","r11","memory");
    return r;
}
void _exit(int code){
    __asm__ volatile("syscall"::"a"(231),"D"((long)code):"rcx","r11","memory");
    for(;;);
}
void exit(int c){ _exit(c); }
void abort(void){ _wr(2,"[abort]\n",8); _exit(134); }

/* output used by the shim (Lua's 'emit' function) */
long fs_write(long fd, const void *b, unsigned long n){ return _wr(fd,b,n); }

/* ---------------- mem/str ---------------- */
void *memcpy(void *d,const void *s,size_t n){unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
void *memmove(void *d,const void *s,size_t n){unsigned char*a=d;const unsigned char*b=s;
    if(a<b){while(n--)*a++=*b++;}else{a+=n;b+=n;while(n--)*--a=*--b;}return d;}
void *memset(void *d,int c,size_t n){unsigned char*a=d;while(n--)*a++=(unsigned char)c;return d;}
int memcmp(const void*p,const void*q,size_t n){const unsigned char*a=p,*b=q;while(n--){if(*a!=*b)return *a-*b;a++;b++;}return 0;}
void *memchr(const void*p,int c,size_t n){const unsigned char*a=p;while(n--){if(*a==(unsigned char)c)return (void*)a;a++;}return 0;}
size_t strlen(const char*s){const char*p=s;while(*p)p++;return (size_t)(p-s);}
int strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
int strncmp(const char*a,const char*b,size_t n){while(n&&*a&&*a==*b){a++;b++;n--;}return n?(unsigned char)*a-(unsigned char)*b:0;}
int strcoll(const char*a,const char*b){return strcmp(a,b);}
char *strcpy(char*d,const char*s){char*r=d;while((*d++=*s++));return r;}
char *strchr(const char*s,int c){for(;*s;s++)if(*s==(char)c)return (char*)s;return c?0:(char*)s;}
size_t strspn(const char*s,const char*set){const char*p=s;for(;*p;p++){const char*q=set;for(;*q&&*q!=*p;q++);if(!*q)break;}return (size_t)(p-s);}
char *strpbrk(const char*s,const char*set){for(;*s;s++){const char*q=set;for(;*q;q++)if(*q==*s)return (char*)s;}return 0;}
char *strstr(const char*h,const char*n){if(!*n)return (char*)h;for(;*h;h++){const char*a=h,*b=n;while(*a&&*b&&*a==*b){a++;b++;}if(!*b)return (char*)h;}return 0;}
char *strerror(int e){(void)e;return "error";}
int abs(int x){return x<0?-x:x;}

/* ---------------- ctype (real glibc tables, offset +128) ---------------- */
const unsigned short **__ctype_b_loc(void){ static const unsigned short *p=CB+128; return &p; }
const int **__ctype_tolower_loc(void){ static const int *p=CL+128; return &p; }
const int **__ctype_toupper_loc(void){ static const int *p=CU+128; return &p; }

/* ---------------- errno ---------------- */
static int _errno; int *__errno_location(void){ return &_errno; }

/* ---------------- stdio: stubs (never reached on our path) ---------------- */
static char _f_stdin,_f_stdout,_f_stderr;
void *stdin=&_f_stdin, *stdout=&_f_stdout, *stderr=&_f_stderr;
void *fopen64(const char*a,const char*b){(void)a;(void)b;return 0;}
void *freopen64(const char*a,const char*b,void*c){(void)a;(void)b;(void)c;return 0;}
int fclose(void*f){(void)f;return 0;}
int fflush(void*f){(void)f;return 0;}
unsigned long fread(void*p,unsigned long s,unsigned long n,void*f){(void)p;(void)s;(void)n;(void)f;return 0;}
int feof(void*f){(void)f;return 1;}
int ferror(void*f){(void)f;return 0;}
int getc(void*f){(void)f;return -1;}
/* fprintf: format and send to fd 2, so Lua errors are visible */
int vsnprintf(char*,size_t,const char*,va_list);
int fprintf(void*stream,const char*fmt,...){(void)stream;char b[512];va_list ap;va_start(ap,fmt);
    int n=vsnprintf(b,sizeof b,fmt,ap);va_end(ap);if(n>0)_wr(2,b,(unsigned long)(n<(int)sizeof b?n:(int)sizeof b));return n;}

/* ---------------- locale (only decimal_point matters to Lua) ---------------- */
struct lconv{char*decimal_point;char*thousands_sep;char*grouping;/* rest ignored */};
static struct lconv _lc={".","",""};
struct lconv *localeconv(void){ return &_lc; }

/* ---------------- math ---------------- */
double floor(double x){ double t=(double)(long long)x; return (t>x)?t-1.0:t; }
double frexp(double x,int*e){ int ex=0; double a=x<0?-x:x; if(a==0){*e=0;return 0;}
    while(a>=1.0){a/=2;ex++;} while(a<0.5){a*=2;ex--;} *e=ex; return x<0?-a:a; }
double ldexp(double x,int e){ while(e>0){x*=2;e--;} while(e<0){x/=2;e++;} return x; }
double fmod(double x,double y){ if(y==0)return 0; double q=(double)(long long)(x/y); return x-q*y; }
double pow(double b,double e){ /* integer exponent only (cold path); otherwise approximate */
    long long n=(long long)e; double r=1,base=b; int neg=n<0; if(neg)n=-n;
    while(n){ if(n&1)r*=base; base*=base; n>>=1; } return neg?1.0/r:r; }
static double pow10i(int n){ double r=1,b=10; int neg=n<0; if(neg)n=-n;
    while(n){ if(n&1)r*=b; b*=b; n>>=1; } return neg?1.0/r:r; }

/* ---------------- basic strtod (cold path: string->number coercion) ---------------- */
double strtod(const char*s,char**end){
    while(*s==' '||*s=='\t')s++; int sign=1; if(*s=='+')s++; else if(*s=='-'){sign=-1;s++;}
    double r=0; while(*s>='0'&&*s<='9'){r=r*10+(*s-'0');s++;}
    if(*s=='.'){s++; double f=0.1; while(*s>='0'&&*s<='9'){r+=(*s-'0')*f;f*=0.1;s++;}}
    if(*s=='e'||*s=='E'){s++; int es=1,ev=0; if(*s=='+')s++; else if(*s=='-'){es=-1;s++;}
        while(*s>='0'&&*s<='9'){ev=ev*10+(*s-'0');s++;} r*=pow10i(es*ev);}
    if(end)*end=(char*)s; return sign*r;
}
long long strtoll(const char*s,char**e,int base){(void)base;while(*s==' ')s++;int sg=1;
    if(*s=='+')s++;else if(*s=='-'){sg=-1;s++;}long long r=0;while(*s>='0'&&*s<='9'){r=r*10+(*s-'0');s++;}
    if(e)*e=(char*)s;return sg*r;}
long strtol(const char*s,char**e,int b){return (long)strtoll(s,e,b);}
unsigned long strtoul(const char*s,char**e,int b){return (unsigned long)strtoll(s,e,b);}
double atof(const char*s){return strtod(s,0);}
void *getenv(const char*n){(void)n;return 0;}
long time(long*t){ if(t)*t=0; return 0; }

/* ---------------- arena allocator (mmap via syscall, no huge .bss) ---------------- */
#define ARENA_SZ (8ul*1024ul*1024ul)
static unsigned char *_arena = 0;
static size_t _apos = 0;
static void arena_init(void){
    /* mmap(0, ARENA_SZ, PROT_READ|WRITE=3, MAP_PRIVATE|ANON=0x22, -1, 0) */
    register long r10 __asm__("r10") = 0x22;
    register long r8  __asm__("r8")  = -1;
    register long r9  __asm__("r9")  = 0;
    long ret;
    __asm__ volatile("syscall" : "=a"(ret)
        : "a"(9L), "D"(0L), "S"(ARENA_SZ), "d"(3L), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory");
    _arena = (unsigned char*)ret;
}
static void *bump(size_t n){ if(!_arena){ arena_init(); }
    n=(n+15)&~(size_t)15; if(_apos+n>ARENA_SZ)return 0; void*p=_arena+_apos; _apos+=n; return p; }
void *malloc(size_t n){ size_t*p=bump(n+16); if(!p)return 0; *p=n; return p+2; }
void free(void*p){ (void)p; }   /* arena: no reclaim (ephemeral process) */
void *realloc(void*p,size_t n){ if(!p)return malloc(n); if(n==0)return 0;
    size_t old=((size_t*)p)[-2]; void*q=malloc(n); if(!q)return 0;
    memcpy(q,p, old<n?old:n); return q; }
void *calloc(size_t a,size_t b){ size_t n=a*b; void*p=malloc(n); if(p)memset(p,0,n); return p; }

/* ---------------- vsnprintf / snprintf ---------------- */
typedef struct{ char*buf; size_t cap; size_t len; } Sink;
static void put(Sink*s,char c){ if(s->len<s->cap-1) s->buf[s->len]=c; s->len++; }
static void puts_n(Sink*s,const char*p,size_t n){ while(n--) put(s,*p++); }

static int utoa(unsigned long long v,unsigned base,int upper,char*out){
    const char*dig = upper?"0123456789ABCDEF":"0123456789abcdef"; char tmp[32]; int i=0;
    if(v==0)tmp[i++]='0'; while(v){ tmp[i++]=dig[v%base]; v/=base; }
    for(int j=0;j<i;j++)out[j]=tmp[i-1-j]; return i;
}
/* format a double as %f with 'prec' decimals; returns length in out (sign handled by caller).
 * NOTE: rounds half-up. glibc rounds half-to-even from the exact binary value, which
 * differs only at exact decimal halfway points (e.g. %.0f of 2.5 -> 2 in glibc, 3 here).
 * Reproducing that needs big-integer decimal conversion (Ryu/gdtoa); out of scope here. */
static int dtoa_f(double x,int prec,char*out){
    int n=0; if(prec<0)prec=6;
    double ip=floor(x); double frac=x-ip;
    /* round the fraction (half-up) */
    double scale=pow10i(prec); long long fi=(long long)(frac*scale+0.5);
    if(fi>=(long long)(scale+0.5)){ ip+=1; fi=0; }
    /* integer part */
    unsigned long long u=(unsigned long long)ip; n+=utoa(u,10,0,out+n);
    if(prec>0){ out[n++]='.'; /* zero-pad the fraction */
        char fb[32]; int fl=utoa((unsigned long long)fi,10,0,fb);
        for(int z=0;z<prec-fl;z++)out[n++]='0'; for(int j=0;j<fl;j++)out[n++]=fb[j]; }
    return n;
}
/* scientific notation: d.ddddde(+/-)XX, x>=0, 'prec' digits after the dot */
static int dtoa_e(double x,int prec,int upper,char*out){
    if(prec<0)prec=6;
    int e=0; double a=x;
    if(a!=0){ while(a>=10){a/=10;e++;} while(a<1){a*=10;e--;} }
    double scale=pow10i(prec); long long mi=(long long)(a*scale+0.5);
    if(mi>=(long long)(10*scale+0.5)){ mi=(long long)(scale+0.5); e++; } /* 9.99..->10 => 1.0, e+1 */
    char digs[48]; int nd=utoa((unsigned long long)mi,10,0,digs);
    int n=0; out[n++]=digs[0];
    if(prec>0){ out[n++]='.'; for(int i=1;i<=prec;i++) out[n++]=(i<nd)?digs[i]:'0'; }
    out[n++]= upper?'E':'e'; out[n++]= e<0?'-':'+';
    int ea=e<0?-e:e; if(ea<10)out[n++]='0'; n+=utoa((unsigned)ea,10,0,out+n);
    return n;
}
/* %g: %e or %f by exponent, trailing zeros stripped; 'upper' => E for %G */
static int dtoa_g(double x,int prec,int upper,char*out){
    if(prec<=0)prec=1; if(x==0){ out[0]='0'; return 1; }
    int e=0; double a=x; while(a>=10){a/=10;e++;} while(a<1){a*=10;e--;}
    int n;
    if(e< -4 || e>=prec){ /* %e form */
        char tmp[64]; int tn=dtoa_e(x,prec-1,upper,tmp);
        int ei=0; while(ei<tn && tmp[ei]!='e' && tmp[ei]!='E') ei++;
        int m=ei; while(m>1 && tmp[m-1]=='0') m--; if(m>0&&tmp[m-1]=='.') m--;
        n=0; for(int j=0;j<m;j++)out[n++]=tmp[j]; for(int j=ei;j<tn;j++)out[n++]=tmp[j];
    } else { /* %f form */
        int p=prec-1-e; if(p<0)p=0; n=dtoa_f(x,p,out);
        if(p>0){ while(n>1&&out[n-1]=='0')n--; if(out[n-1]=='.')n--; }
    }
    return n;
}

int vsnprintf(char*buf,size_t cap,const char*fmt,va_list ap){
    Sink s={buf,cap?cap:1,0};
    for(const char*p=fmt;*p;p++){
        if(*p!='%'){ put(&s,*p); continue; }
        p++;
        int left=0,zero=0,plus=0,space=0,alt=0;
        for(;;p++){ if(*p=='-')left=1; else if(*p=='0')zero=1; else if(*p=='+')plus=1;
                    else if(*p==' ')space=1; else if(*p=='#')alt=1; else break; }
        int width=0; if(*p=='*'){ width=va_arg(ap,int); p++; }
            else while(*p>='0'&&*p<='9'){ width=width*10+(*p-'0'); p++; }
        int prec=-1; if(*p=='.'){ p++; prec=0; if(*p=='*'){ prec=va_arg(ap,int); p++; }
            else while(*p>='0'&&*p<='9'){ prec=prec*10+(*p-'0'); p++; } }
        while(*p=='l'||*p=='h'||*p=='z'||*p=='j'||*p=='t'||*p=='L')p++; /* length: ignored (we read per conversion) */
        char c=*p; char tmp[400]; int tl=0; const char*body=tmp; int sign=0;
        switch(c){
            case '%': tmp[tl++]='%'; break;
            case 'c': tmp[tl++]=(char)va_arg(ap,int); break;
            case 's':{ const char*str=va_arg(ap,const char*); if(!str)str="(null)";
                       int len=0; while(str[len]&&(prec<0||len<prec))len++;
                       body=str; tl=len; break; }
            case 'd': case 'i':{ long long v=va_arg(ap,long long); if(v<0){sign='-';
                       tl=utoa((unsigned long long)(-v),10,0,tmp);} else {sign=plus?'+':(space?' ':0);
                       tl=utoa((unsigned long long)v,10,0,tmp);} break; }
            case 'u':{ unsigned long long v=va_arg(ap,unsigned long long); tl=utoa(v,10,0,tmp); break; }
            case 'x': case 'X':{ unsigned long long v=va_arg(ap,unsigned long long); tl=utoa(v,16,c=='X',tmp); break; }
            case 'o':{ unsigned long long v=va_arg(ap,unsigned long long); tl=utoa(v,8,0,tmp); break; }
            case 'p':{ void*pp=va_arg(ap,void*); tmp[tl++]='0';tmp[tl++]='x';
                       tl+=utoa((unsigned long long)(uintptr_t)pp,16,0,tmp+tl); break; }
            case 'f': case 'F':{ double d=va_arg(ap,double); if(d<0){sign='-';d=-d;} else sign=plus?'+':(space?' ':0);
                       tl=dtoa_f(d,prec,tmp); break; }
            case 'g': case 'G':{ double d=va_arg(ap,double); if(d<0){sign='-';d=-d;} else sign=plus?'+':(space?' ':0);
                       tl=dtoa_g(d,prec<0?6:prec,c=='G',tmp); break; }
            case 'e': case 'E':{ double d=va_arg(ap,double); if(d<0){sign='-';d=-d;} else sign=plus?'+':(space?' ':0);
                       tl=dtoa_e(d,prec<0?6:prec,c=='E',tmp); break; }
            default: tmp[tl++]='%'; tmp[tl++]=c; break;
        }
        int total=tl+(sign?1:0);
        int pad=width>total?width-total:0;
        if(!left && !zero) while(pad-->0) put(&s,' ');
        if(sign) put(&s,(char)sign);
        if(!left && zero) while(pad-->0) put(&s,'0');
        puts_n(&s,body,tl);
        if(left) while(pad-->0) put(&s,' ');
    }
    if(s.cap) s.buf[s.len<s.cap?s.len:s.cap-1]=0;
    return (int)s.len;
}
int snprintf(char*buf,size_t cap,const char*fmt,...){
    va_list ap; va_start(ap,fmt); int n=vsnprintf(buf,cap,fmt,ap); va_end(ap); return n;
}
