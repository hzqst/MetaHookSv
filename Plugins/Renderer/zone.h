void Z_Free(void *ptr);
void *Z_Malloc(int size);
void *Z_TagMalloc(int size, int tag);
void Z_CheckHeap(void);

void *Hunk_AllocName(int size, char *name);
void *Hunk_Alloc(int size);
void *Hunk_HighAllocName(int size, char *name);
int	Hunk_LowMark(void);
void Hunk_FreeToLowMark(int mark);
int	Hunk_HighMark(void);
void Hunk_FreeToHighMark(int mark);
void *Hunk_TempAlloc(int size);

#if !defined(CACHE_USER)
#define CACHE_USER
typedef struct cache_user_s
{
	void *data;
}
cache_user_t;
#endif

void Cache_FreeLow(int new_low_hunk);
void Cache_FreeHigh(int new_high_hunk);
void *Cache_Alloc(cache_user_t *c, int size, char *name);
void Cache_Force_Flush(void);
void Cache_Flush(void);
void Cache_Print(void);
void *Cache_Check(cache_user_t *c);
void Cache_Free(cache_user_t *c);
void Cache_Report(void);
void Cache_Compact(void);
void Cache_Init(void);
int Cache_TotalUsed(void);

void Memory_Init(void);