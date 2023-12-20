#include <metahook.h>
#include "plugins.h"
#include "gl_local.h"

void* Hunk_AllocName(int size, const char* name)
{
	return gRefFuncs.Hunk_AllocName(size, name);
}

void* Cache_Alloc(cache_user_t* c, int size, const char* name)
{
	return gRefFuncs.Cache_Alloc(c, size, name);
}

void Cache_UnlinkLRU(cache_system_t* cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		g_pMetaHookAPI->SysError("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

void Cache_Free(cache_user_t* c)
{
	cache_system_t* cs;

	if (!c->data)
		g_pMetaHookAPI->SysError("Cache_Free: not allocated");

	cs = ((cache_system_t*)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU(cs);
}

void Cache_MakeLRU(cache_system_t* cs)
{
	if (cs->lru_next || cs->lru_prev)
		g_pMetaHookAPI->SysError("Cache_MakeLRU: active link");

	(*cache_head).lru_next->lru_prev = cs;
	cs->lru_next = (*cache_head).lru_next;
	cs->lru_prev = &(*cache_head);
	(*cache_head).lru_next = cs;
}

void* Cache_Check(cache_user_t* c)
{
	cache_system_t* cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t*)c->data) - 1;

	Cache_UnlinkLRU(cs);
	Cache_MakeLRU(cs);

	return c->data;
}
