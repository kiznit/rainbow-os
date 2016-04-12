// Note: no header guards, this is intentional so that <assert.h> can be included multiple times.

#ifdef __cplusplus
extern "C" {
#endif


#undef assert

#ifdef NDEBUG
#define assert(x) ((void)0)
#else
#define assert(x) if (x) {} else _assert(#x, __FILE__, __LINE__, __FUNCTION__);
#endif


void _assert(const char* expression, const char* file, int line, const char* function);


#ifdef __cplusplus
}
#endif
