#pragma once

void Cache_Free(cache_user_t* c);
void* Cache_Check(cache_user_t* c);
void* Cache_Alloc(cache_user_t* c, int size, const char* name);
void* Hunk_AllocName(int size, const char* name);