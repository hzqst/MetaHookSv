#include <metahook.h>
#include <capstone.h>

#include "enginedef.h"
#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"
#include "message.h"

#include "ClientPhysicManager.h"
#include "ClientEntityManager.h"
#include "Viewport.h"

#define R_NEWMAP_SIG_COMMON    "\x55\x8B\xEC\x83\xEC\x2A\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC"
#define R_NEWMAP_SIG_BLOB      R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_NEW       R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_HL25      R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_SVENGINE "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"

#define R_RECURSIVEWORLDNODE_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE"
#define R_RECURSIVEWORLDNODE_SIG_NEW2 R_RECURSIVEWORLDNODE_SIG_BLOB
#define R_RECURSIVEWORLDNODE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x47\x04"
#define R_RECURSIVEWORLDNODE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x08\x2A\x8B\x5D\x08\x83\x3B\xFE\x0F"
#define R_RECURSIVEWORLDNODE_SIG_SVENGINE "\x83\xEC\x08\x53\x8B\x5C\x24\x10\x83\x3B\xFE"

#define R_DRAWTENTITIESONLIST_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"
#define R_DRAWTENTITIESONLIST_SIG_NEW2 R_DRAWTENTITIESONLIST_SIG_BLOB
#define R_DRAWTENTITIESONLIST_SIG_NEW "\x55\x8B\xEC\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44\x0F\x8B\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define R_DRAWTENTITIESONLIST_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xF3\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F\x2E"
#define R_DRAWTENTITIESONLIST_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\x2A\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x00\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE"

#define R_RENDERVIEW_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x14\xDF\xE0\xF6\xC4"
#define R_RENDERVIEW_SIG_NEW2 R_RENDERVIEW_SIG_BLOB
#define R_RENDERVIEW_SIG_NEW "\x55\x8B\xEC\x83\xEC\x14\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44"
#define R_RENDERVIEW_SIG_HL25 "\x55\x8B\xEC\xF3\x0F\x10\x05\x2A\x2A\x2A\x2A\x83\xEC\x2A\x0F\x57\xC9\x0F\x2E\xC1\x9F\xF6"
#define R_RENDERVIEW_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x34\x53\x56\x57\x8B\x7D\x08\x85\xFF"

#define V_RENDERVIEW_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x00\x00\x00\x2A\x2A\x33\x2A\x33\x2A\x2A\x2A\x89\x35\x2A\x2A\x2A\x2A\x89\x35"
#define V_RENDERVIEW_SIG_NEW2 V_RENDERVIEW_SIG_BLOB
#define V_RENDERVIEW_SIG_NEW "\x55\x8B\xEC\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x2A\x2A\x33\x2A\x33"
#define V_RENDERVIEW_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x33\x2A\x89\x35\x2A\x2A\x2A\x2A\x89\x35"
#define V_RENDERVIEW_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\xD9\xEE\xD9\x15"

#define R_CULLBOX_SIG_BLOB "\x53\x8B\x5C\x24\x08\x56\x57\x8B\x7C\x24\x14\xBE\x2A\x2A\x2A\x2A\x56\x57\x53\xE8"
#define R_CULLBOX_SIG_NEW2 R_CULLBOX_SIG_BLOB
#define R_CULLBOX_SIG_NEW "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x57\x8B\x7D\x0C\xBE\x2A\x2A\x2A\x2A\x56\x57\x53"
#define R_CULLBOX_SIG_HL25 "\x55\x8B\xEC\x2A\x8B\x2A\x08\x2A\x2A\x8B\x2A\x0C\xBE\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C\x83\xF8\x02"
#define R_CULLBOX_SIG_SVENGINE "\x2A\x8B\x2A\x24\x08\x2A\x2A\x8B\x2A\x24\x14\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C\x83\xF8\x02"

private_funcs_t gPrivateFuncs = {0};

studiohdr_t** pstudiohdr = NULL;
model_t** r_model = NULL;
void* g_pGameStudioRenderer = NULL;
//int* r_framecount = NULL;
//int* r_visframecount = NULL;
int* cl_parsecount = NULL;
void* cl_frames = NULL;
int size_of_frame = 0;
int* cl_viewentity = NULL;
cl_entity_t** currententity = NULL;
void* mod_known = NULL;
int* mod_numknown = NULL;
TEMPENTITY* gTempEnts = NULL;

//Sven Co-op only
int* allow_cheats = NULL;

int* g_iWaterLevel = NULL;
bool* g_bRenderingPortals_SCClient = NULL;
int* g_ViewEntityIndex_SCClient = NULL;

struct pitchdrift_t* g_pitchdrift = NULL;

int* g_iUser1 = NULL;
int* g_iUser2 = NULL;

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

static hook_t* g_phook_R_NewMap = NULL;
static hook_t* g_phook_Mod_LoadStudioModel = NULL;

static hook_t* g_phook_R_RenderView_SvEngine = NULL;
static hook_t* g_phook_R_RenderView = NULL;

void Engine_FillAddress_R_DrawTEntitiesOnList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawTEntitiesOnList)
		return;

	ULONG_PTR R_DrawTEntitiesOnList_VA = 0;
	ULONG R_DrawTEntitiesOnList_RVA = 0;

	{
		const char sigs[] = "Non-sprite set to glow";
		auto NonSprite_String = Search_Pattern_Data(sigs, DllInfo);
		if (!NonSprite_String)
			NonSprite_String = Search_Pattern_Rdata(sigs, DllInfo);

		if (NonSprite_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B";
			*(DWORD*)(pattern + 1) = (DWORD)NonSprite_String;
			auto NonSprite_Call = Search_Pattern(pattern, DllInfo);

			if (NonSprite_Call)
			{
				R_DrawTEntitiesOnList_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(NonSprite_Call, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05 &&
						Candidate[6] == 0xD8)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
					});
				gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_DrawTEntitiesOnList)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_HL25, DllInfo);
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW, DllInfo);

			if (!R_DrawTEntitiesOnList_VA)
				R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW2, DllInfo);

			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_DrawTEntitiesOnList);
}

void Engine_FillAddress_R_CullBox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_CullBox)
		return;

	PVOID R_CullBox_VA = NULL;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_CullBox_VA = Search_Pattern(R_CULLBOX_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_CullBox_VA = Search_Pattern(R_CULLBOX_SIG_HL25, DllInfo);
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_CullBox_VA = Search_Pattern(R_CULLBOX_SIG_NEW, DllInfo);

		if (!R_CullBox_VA)
			R_CullBox_VA = Search_Pattern(R_CULLBOX_SIG_NEW2, DllInfo);

		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_CullBox_VA = Search_Pattern(R_CULLBOX_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_CullBox);
}

void Engine_FillAddress_R_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RenderView_SvEngine || gPrivateFuncs.R_RenderView)
		return;

	PVOID R_RenderView_VA = 0;

	{
		const char sig[] = "R_RenderView: NULL worldmodel";
		auto R_RenderView_String = Search_Pattern_Data(sig, DllInfo);
		if (!R_RenderView_String)
			R_RenderView_String = Search_Pattern_Rdata(sig, DllInfo);
		if (R_RenderView_String)
		{
			char pattern[] = "\x75\x2A\x68\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)R_RenderView_String;
			auto R_RenderView_PushString = Search_Pattern(pattern, DllInfo);
			if (R_RenderView_PushString)
			{
				R_RenderView_VA = (decltype(R_RenderView_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_RenderView_PushString, 0x100, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					//SvEngine 10182
					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
						return TRUE;

					return FALSE;
					});
			}
		}
	}

	if (!R_RenderView_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_NEW, DllInfo);

			if (!R_RenderView_VA)
				R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_NEW2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_BLOB, DllInfo);
		}
	}

	if (R_RenderView_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))ConvertDllInfoSpace(R_RenderView_VA, DllInfo, RealDllInfo);
		}
		else
		{
			gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))ConvertDllInfoSpace(R_RenderView_VA, DllInfo, RealDllInfo);
		}
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Sig_FuncNotFound(R_RenderView_SvEngine);
	}
	else
	{
		Sig_FuncNotFound(R_RenderView);
	}
}

void Engine_FillAddress_V_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.V_RenderView)
		return;

	PVOID V_RenderView_VA = 0;

	PVOID R_RenderView_VA = ConvertDllInfoSpace(
		(gPrivateFuncs.R_RenderView_SvEngine) ? (PVOID)gPrivateFuncs.R_RenderView_SvEngine : (PVOID)gPrivateFuncs.R_RenderView,
		RealDllInfo, DllInfo);

	if (g_dwVideoMode == VIDEOMODE_SOFTWARE)
	{
		//R_RenderView: called without
		const char V_RenderView_StringPattern[] = "R_RenderView: called without enough stack";
		auto V_RenderView_String = Search_Pattern_Data(V_RenderView_StringPattern, DllInfo);
		if (!V_RenderView_String)
			V_RenderView_String = Search_Pattern_Rdata(V_RenderView_StringPattern, DllInfo);

		Sig_VarNotFound(V_RenderView_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)V_RenderView_String;
		auto V_RenderView_PushString = Search_Pattern(pattern, DllInfo);

		Sig_VarNotFound(V_RenderView_PushString);

		V_RenderView_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(V_RenderView_PushString, 0x150, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
				return TRUE;

			return FALSE;
		});

		gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);

	}
	else
	{
		const char pattern[] = "\x68\x00\x40\x00\x00\xFF";
		/*
			.text:01DCDF5C 68 00 40 00 00                                      push    4000h           ; mask
			.text:01DCDF61 FF D3                                               call    ebx ; glClear
		*/
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct V_RenderView_SearchContext_s
				{
					PVOID R_RenderView_VA;
					bool bFoundCallRenderView{};
				}V_RenderView_SearchContext;

				V_RenderView_SearchContext ctx = { R_RenderView_VA };

				g_pMetaHookAPI->DisasmRanges(pFound + 5, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (V_RenderView_SearchContext*)context;

					if (address[0] == 0xE8)
					{
						PVOID callTarget = (PVOID)pinst->detail->x86.operands[0].imm;

						if (callTarget == ctx->R_RenderView_VA)
						{
							ctx->bFoundCallRenderView = true;
							return TRUE;
						}
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);

				if (ctx.bFoundCallRenderView)
				{
					V_RenderView_VA = (decltype(V_RenderView_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x300, [](PUCHAR Candidate) {

						if (Candidate[0] == 0x81 &&
							Candidate[1] == 0xEC &&
							Candidate[4] == 0 &&
							Candidate[5] == 0)
							return TRUE;

						if (Candidate[0] == 0x55 &&
							Candidate[1] == 0x8B &&
							Candidate[2] == 0xEC)
							return TRUE;

						if (Candidate[0] == 0xA1 &&
							Candidate[5] == 0x81 &&
							Candidate[6] == 0xEC)
							return TRUE;

						return FALSE;
						});

					gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);

					break;
				}

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}

	if (!gPrivateFuncs.V_RenderView)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_HL25, DllInfo);
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_NEW, DllInfo);

			if (!V_RenderView_VA)
				V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_NEW2, DllInfo);

			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_BLOB, DllInfo);
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(V_RenderView);
}

void Engine_FillAddress_CL_ReallocateDynamicData(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_max_edicts = NULL;
		cl_entity_t *cl_entities = NULL;
	*/

	//Search "CL_Reallocate cl_entities"
	const char sigs[] = "CL_Reallocate cl_entities\n";
	auto CL_Reallocate_String = Search_Pattern_Data(sigs, DllInfo);
	if (!CL_Reallocate_String)
		CL_Reallocate_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(CL_Reallocate_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)CL_Reallocate_String;
	PVOID CL_Reallocate_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(CL_Reallocate_Call);

	PVOID CL_ReallocateDynamicData_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(CL_Reallocate_Call, 0x100, [](PUCHAR Candidate) {
		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC)
		{
			return TRUE;
		}

		if (Candidate[0] == 0x8B &&
			Candidate[1] == 0x44 &&
			Candidate[2] == 0x24)
		{
			return TRUE;
		}

		if (Candidate[0] == 0xFF &&
			Candidate[2] == 0x24)
		{
			return TRUE;
		}

		return FALSE;
		});

	Sig_VarNotFound(CL_ReallocateDynamicData_VA);

	typedef struct CL_ReallocateDynamicData_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID CL_Reallocate_Call{};
	} CL_ReallocateDynamicData_SearchContext;

	CL_ReallocateDynamicData_SearchContext ctx = { DllInfo, RealDllInfo, CL_Reallocate_Call };

	g_pMetaHookAPI->DisasmRanges(CL_ReallocateDynamicData_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (CL_ReallocateDynamicData_SearchContext*)context;

		if (!cl_max_edicts && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			// mov     eax, cl_max_edicts
			// add     esp, 4
			if (0 == memcmp(address + instLen, "\x83\xC4\x04", 3))
			{
				cl_max_edicts = (decltype(cl_max_edicts))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
		}

		if (!cl_max_edicts && pinst->id == X86_INS_IMUL &&
			pinst->detail->x86.op_count == 3 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[2].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			cl_max_edicts = (decltype(cl_max_edicts))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (!cl_entities && address > (PUCHAR)ctx->CL_Reallocate_Call && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			cl_entities = (decltype(cl_entities))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (cl_entities && cl_max_edicts)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(cl_max_edicts);
	Sig_VarNotFound(cl_entities);
}

void Engine_FillAddress_TempEntsVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		TEMPENTITY *gTempEnts = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define GTEMPENTS_SIG_SVENGINE "\x68\x00\xE0\x5F\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xA3"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GTEMPENTS_SIG_SVENGINE, DllInfo);
		Sig_AddrNotFound(gTempEnts);
		PVOID gTempEnts_VA = *(PVOID*)(addr + 8);
		gTempEnts = (decltype(gTempEnts))ConvertDllInfoSpace(gTempEnts_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define GTEMPENTS_SIG_NEW "\x68\x30\x68\x17\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GTEMPENTS_SIG_NEW, DllInfo);
		Sig_AddrNotFound(gTempEnts);
		PVOID gTempEnts_VA = *(PVOID*)(addr + 8);
		gTempEnts = (decltype(gTempEnts))ConvertDllInfoSpace(gTempEnts_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(gTempEnts);
}

void Engine_FillAddress_CL_Set_ServerExtraInfo(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
		int *allow_cheats
		*/
		auto CL_Set_ServerExtraInfo = g_pMetaHookAPI->FindCLParseFuncByName("svc_sendextrainfo");

		Sig_VarNotFound(CL_Set_ServerExtraInfo);

		PVOID CL_Set_ServerExtraInfo_VA = ConvertDllInfoSpace(CL_Set_ServerExtraInfo, RealDllInfo, DllInfo);

		typedef struct CL_Set_ServerExtraInfo_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}CL_Set_ServerExtraInfo_SearchContext;

		CL_Set_ServerExtraInfo_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_Set_ServerExtraInfo_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_Set_ServerExtraInfo_SearchContext*)context;
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX)
			{
				allow_cheats = (decltype(allow_cheats))
					ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (allow_cheats)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(allow_cheats);
	}
	else
	{
		//"int allow_cheats;" is not a thing in GoldSrc
	}
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_viewentity = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		PVOID cl_viewentity_VA = *(PVOID*)(addr + 10);
		cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_GOLDSRC);
		Sig_AddrNotFound(cl_viewentity);

		typedef struct CL_ViewEntity_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			bool found_cmp_200{};
		} CL_ViewEntity_SearchContext;

		CL_ViewEntity_SearchContext ctx = { DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_ViewEntity_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x200)
			{
				ctx->found_cmp_200 = true;
			}

			if (ctx->found_cmp_200)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		if (ctx.found_cmp_200)
		{
			PVOID cl_viewentity_VA = *(PVOID*)(addr + 1);
			cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_VarNotFound(cl_viewentity);
}

void Engine_FillAddress_R_NewMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_NewMap)
		return;

	{
		//Setting up renderer...
		const char sigs[] = "Setting up renderer...\n";
		auto SettingUpRenderer_String = Search_Pattern_Data(sigs, DllInfo);
		if (!SettingUpRenderer_String)
			SettingUpRenderer_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(SettingUpRenderer_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)SettingUpRenderer_String;
		auto SettingUpRenderer_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(SettingUpRenderer_PushString);

		typedef struct SettingUpRenderer_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}SettingUpRenderer_SearchContext;

		SettingUpRenderer_SearchContext ctx = { DllInfo , RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((PUCHAR)SettingUpRenderer_PushString + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (SettingUpRenderer_SearchContext*)context;

			if (address[0] == 0xE8)
			{
				gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);
	}

	if (!gPrivateFuncs.R_NewMap)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_HL25, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_NEW, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_NewMap);
}

void Engine_FillAddress_ModKnown(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars
		model_t *mod_known = NULL;
	*/

	const char pattern[] = "\xB8\x9D\x82\x97\x53\x81\xE9";
	ULONG_PTR addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(mod_known);

	PVOID mod_known_VA = *(PVOID*)(addr + 7);
	mod_known = (decltype(mod_known))ConvertDllInfoSpace(mod_known_VA, DllInfo, RealDllInfo);

	Sig_VarNotFound(mod_known);
}

void Engine_FillAddress_Mod_NumKnown(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *mod_numknown = NULL;
	*/

	typedef struct Mod_NumKnown_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} Mod_NumKnown_SearchContext;

	const char sigs[] = "Cached models:\n";
	auto Mod_Print_String = Search_Pattern_Data(sigs, DllInfo);
	if (!Mod_Print_String)
		Mod_Print_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(Mod_Print_String);

	char pattern[] = "\x57\x68\x2A\x2A\x2A\x2A\xE8";
	*(DWORD*)(pattern + 2) = (DWORD)Mod_Print_String;
	auto Mod_Print_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(Mod_Print_Call);

	Mod_NumKnown_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(Mod_Print_Call, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (Mod_NumKnown_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0)
		{//A1 84 5C 32 02 mov     eax, mod_numknown
			mod_numknown = (decltype(mod_numknown))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG)
		{//39 3D 44 32 90 03 cmp     mod_numknown, edi
			mod_numknown = (decltype(mod_numknown))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (mod_numknown)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(mod_numknown);
}

void Engine_FillAddress_R_DrawTEntitiesOnListVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		float* r_blend = NULL;
		void *cl_frames = NULL;
		int *cl_parsecount = NULL;

		//Global vars
		int size_of_frame = sizeof(frame_t);
	*/
	PVOID R_DrawTEntitiesOnList_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawTEntitiesOnList, RealDllInfo, DllInfo);

	if (g_dwEngineBuildnum <= 8684)
	{
		size_of_frame = 0x42B8;
	}

	typedef struct R_DrawTEntitiesOnList_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int disableFog_instcount{};
		int parsemod_instcount{};
		int getskin_instcount{};
		int r_entorigin_candidate_count{};
		int push2300_instcount{};
		int ClientDLL_DrawTransparentTriangles_candidate_instcount{};
		ULONG_PTR r_entorigin_candidateVA[3]{};
	} R_DrawTEntitiesOnList_SearchContext;

	R_DrawTEntitiesOnList_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_DrawTEntitiesOnList_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (R_DrawTEntitiesOnList_SearchContext*)context;

		if (pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].imm == 0xB60)
		{//.text:01D92330 68 60 0B 00 00 push    0B60h

			ctx->disableFog_instcount = instCount;
		}

		if (address[0] == 0x6A && address[1] == 0x00 && address[2] == 0xE8)
		{
			//6A 00 push    0
			//E8 A3 13 05 00                                      call    GL_EnableDisableFog

			auto callTarget = GetCallAddress((address + 2));

			typedef struct GL_EnableDisableFog_SearchContext_s
			{
				bool bFoundGL_FOG{};
			} GL_EnableDisableFog_SearchContext;

			GL_EnableDisableFog_SearchContext ctx2 = { };

			g_pMetaHookAPI->DisasmRanges(callTarget, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx2 = (GL_EnableDisableFog_SearchContext*)context;

				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].imm == 0xB60)
				{//.text:01D92330 68 60 0B 00 00 push    0B60h

					ctx2->bFoundGL_FOG = instCount;
				}

				return FALSE;

				}, 0, &ctx2);

			if (ctx2.bFoundGL_FOG)
			{
				ctx->disableFog_instcount = instCount;
			}
		}

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D923D9 A1 DC 72 ED 01                                      mov     eax, cl_parsemod
			//.text:01D88CBB A1 CC AF E3 01                                      mov     eax, cl_parsemod
			DWORD value = *(DWORD*)pinst->detail->x86.operands[1].mem.disp;
			if (value == 63)
			{
				ctx->parsemod_instcount = instCount;
			}
		}
		else if (!cl_parsecount && ctx->parsemod_instcount &&
			instCount < ctx->parsemod_instcount + 3 &&
			(pinst->id == X86_INS_MOV || pinst->id == X86_INS_AND) &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D923DE 23 05 AC D2 30 02                                   and     eax, cl_parsecount
			//.text:01D88CC0 8B 0D 04 AE D8 02                                   mov     ecx, cl_parsecount
			cl_parsecount = (decltype(cl_parsecount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (!cl_frames && ctx->parsemod_instcount &&
			instCount < ctx->parsemod_instcount + 20 &&
			pinst->id == X86_INS_LEA &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base != 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D923F0 8D 80 F4 D5 30 02                                   lea     eax, cl_frames[eax]
			//.text:01D88CE8 8D 84 CA 4C B1 D8 02                                lea     eax, cl_frames_1[edx+ecx*8]
			cl_frames = (decltype(cl_frames))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (ctx->parsemod_instcount &&
			instCount < ctx->parsemod_instcount + 5 &&
			pinst->id == X86_INS_IMUL &&
			pinst->detail->x86.op_count == 3 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[2].type == X86_OP_IMM &&
			pinst->detail->x86.operands[2].imm > 0x4000 &&
			pinst->detail->x86.operands[2].imm < 0xF000)
		{
			//.text:01D923E4 69 C8 D8 84 00 00                                   imul    ecx, eax, 84D8h
			size_of_frame = pinst->detail->x86.operands[2].imm;
		}
		else if (
			pinst->id == X86_INS_MOVSX &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].size == 4 &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].size == 2 &&
			pinst->detail->x86.operands[1].mem.base != 0 &&
			pinst->detail->x86.operands[1].mem.disp == 0x2E8)
		{
			//.text:01D924D9 0F BF 83 E8 02 00 00                                movsx   eax, word ptr [ebx+2E8h]
			ctx->getskin_instcount = instCount;
		}

		if (ctx->getskin_instcount &&
			instCount < ctx->getskin_instcount + 20 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
			auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			if (ctx->r_entorigin_candidate_count < 3)
			{
				bool bFound = false;
				for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
				{
					if (ctx->r_entorigin_candidateVA[k] == candidateVA)
						bFound = true;
				}
				if (!bFound)
				{
					ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
					ctx->r_entorigin_candidate_count++;
				}
			}
		}

		if (ctx->getskin_instcount &&
			instCount < ctx->getskin_instcount + 20 &&
			pinst->id == X86_INS_FST &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
			auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			if (ctx->r_entorigin_candidate_count < 3)
			{
				bool bFound = false;
				for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
				{
					if (ctx->r_entorigin_candidateVA[k] == candidateVA)
						bFound = true;
				}
				if (!bFound)
				{
					ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
					ctx->r_entorigin_candidate_count++;
				}
			}
		}

		if (ctx->getskin_instcount &&
			instCount < ctx->getskin_instcount + 20 &&
			pinst->id == X86_INS_MOVSS &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:101FA69B F3 0F 10 00                                         movss   xmm0, dword ptr[eax]
			//.text : 101FA69F F3 0F 11 05 E0 02 DC 10                             movss   r_entorigin, xmm0
			auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			if (ctx->r_entorigin_candidate_count < 3)
			{
				bool bFound = false;
				for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
				{
					if (ctx->r_entorigin_candidateVA[k] == candidateVA)
						bFound = true;
				}
				if (!bFound)
				{
					ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
					ctx->r_entorigin_candidate_count++;
				}
			}
		}


		if (cl_parsecount && cl_frames && ctx->r_entorigin_candidate_count >= 3)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(cl_frames);
	Sig_VarNotFound(cl_parsecount);
	Sig_VarNotFound(size_of_frame);
}

void Engine_FillAddress_VisEdicts(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
		int *cl_numvisedicts = NULL;
		cl_entity_t **cl_visedicts = NULL;
	*/

	PVOID cl_numvisedicts_VA = 0;
	PVOID cl_visedicts_VA = 0;

	{
		/*
			.text:01D0C7AF 8B 0D 50 F9 F0 02                                   mov     ecx, dword_2F0F950
			.text:01D0C7B5 81 F9 00 02 00 00                                   cmp     ecx, 200h
		*/
		char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x81\xF9\x00\x2A\x00\x00";
		auto ClientDLL_AddEntity_Pattern = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(ClientDLL_AddEntity_Pattern);

		cl_numvisedicts_VA = *(PVOID*)((PUCHAR)ClientDLL_AddEntity_Pattern + 2);

		typedef struct VisEdicts_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} VisEdicts_SearchContext;

		VisEdicts_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(ClientDLL_AddEntity_Pattern, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (VisEdicts_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == X86_REG_ECX &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				//.text:01D198C9 89 04 8D 00 3A 6E 02                                mov     cl_visedicts[ecx*4], eax
				//.text:01D0C7C5 89 14 8D C0 F0 D5 02                                mov     cl_visedicts[ecx*4], edx

				cl_visedicts = (decltype(cl_visedicts))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (cl_visedicts)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		cl_numvisedicts = (decltype(cl_numvisedicts))ConvertDllInfoSpace(cl_numvisedicts_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(cl_visedicts);
	Sig_VarNotFound(cl_numvisedicts);
}

void Engine_FillAddress(const mh_dll_info_t &DllInfo, const mh_dll_info_t &RealDllInfo)
{
	Engine_FillAddress_R_DrawTEntitiesOnList(DllInfo, RealDllInfo);

	Engine_FillAddress_R_CullBox(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_V_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_ReallocateDynamicData(DllInfo, RealDllInfo);

	Engine_FillAddress_TempEntsVars(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_Set_ServerExtraInfo(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_ViewEntityVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_NewMap(DllInfo, RealDllInfo);

	Engine_FillAddress_ModKnown(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_NumKnown(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawTEntitiesOnListVars(DllInfo, RealDllInfo);

	Engine_FillAddress_VisEdicts(DllInfo, RealDllInfo);
}

void Client_FillAddress_CL_IsThirdPerson(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID CL_IsThirdPerson = ConvertDllInfoSpace((void*)g_pMetaSave->pExportFuncs->CL_IsThirdPerson, RealDllInfo, DllInfo);

	if(!CL_IsThirdPerson)
	{
		if (g_pMetaHookAPI->GetClientModule())
		{
			CL_IsThirdPerson = ConvertDllInfoSpace(GetProcAddress(g_pMetaHookAPI->GetClientModule(), "CL_IsThirdPerson"), RealDllInfo, DllInfo);
		}
	}

	if (CL_IsThirdPerson)
	{
		typedef struct CL_IsThirdPerson_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			ULONG_PTR CandidateVA[16]{};
			int iNumCandidates{};
		}CL_IsThirdPerson_SearchContext;

		CL_IsThirdPerson_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_IsThirdPerson, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (CL_IsThirdPerson_SearchContext*)context;
			auto pinst = (cs_insn*)inst;

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					(
						pinst->detail->x86.operands[0].reg == X86_REG_EAX ||
						pinst->detail->x86.operands[0].reg == X86_REG_EBX ||
						pinst->detail->x86.operands[0].reg == X86_REG_ECX ||
						pinst->detail->x86.operands[0].reg == X86_REG_EDX ||
						pinst->detail->x86.operands[0].reg == X86_REG_ESI ||
						pinst->detail->x86.operands[0].reg == X86_REG_EDI
						) &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidateVA[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidateVA[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		if (ctx.iNumCandidates >= 3 && ctx.CandidateVA[ctx.iNumCandidates - 1] == ctx.CandidateVA[ctx.iNumCandidates - 2] + sizeof(int))
		{
			g_iUser1 = (decltype(g_iUser1))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[ctx.iNumCandidates - 2], DllInfo, RealDllInfo);
			g_iUser2 = (decltype(g_iUser2))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[ctx.iNumCandidates - 1], DllInfo, RealDllInfo);
		}
	}
}

void Client_FillAddress_RenderingPortals(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
.text:1004EAA8 6A 00                                               push    0
.text:1004EAAA 6A 00                                               push    0
.text:1004EAAC 6A 00                                               push    0
.text:1004EAAE 8B 01                                               mov     eax, [ecx]
.text:1004EAB0 FF 50 2C                                            call    dword ptr [eax+2Ch]
.text:1004EAB3 8B 35 A4 A1 11 10                                   mov     esi, ds:glDisable
.text:1004EAB9 33 D2                                               xor     edx, edx
.text:1004EABB C6 05 0D C8 63 10 01                                mov     g_bRenderingPortals, 1
	*/

	const char pattern[] = "\x6A\x00\x6A\x00\x6A\x00\x8B\x2A\xFF\x50\x2A";
	auto addr = Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(g_bRenderingPortals);

	typedef struct RenderingPortals_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}RenderingPortals_SearchContext;

	RenderingPortals_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(addr, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (RenderingPortals_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
			pinst->detail->x86.operands[1].imm == 1)
		{
			g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_VarNotFound(g_bRenderingPortals_SCClient);
}

void Client_FillAddress_ViewEntityIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_dwEngineBuildnum >= 10182)
	{
		const char pattern[] = "\xFF\x15\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x8B\x00\x2A\x05";
		auto addr = Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(g_ViewEntityIndex_SCClient);

		typedef struct ViewEntityIndex_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}ViewEntityIndex_SearchContext;

		ViewEntityIndex_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(addr, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (ViewEntityIndex_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				g_ViewEntityIndex_SCClient = (decltype(g_ViewEntityIndex_SCClient))ConvertDllInfoSpace((PVOID) pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		Sig_VarNotFound(g_ViewEntityIndex_SCClient);
	}
}

void Client_FillAddress_Drift(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x89";
	ULONG_PTR addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(g_pitchdrift);
	PVOID g_pitchdrift_VA = *(PVOID*)(addr + 2);

	g_pitchdrift = (decltype(g_pitchdrift))ConvertDllInfoSpace(g_pitchdrift_VA, DllInfo, RealDllInfo);
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory)
	{
		auto SCClient001 = pfnClientFactory("SCClientDLL001", 0);

		if (SCClient001)
		{
			Client_FillAddress_RenderingPortals(DllInfo, RealDllInfo);
			Client_FillAddress_ViewEntityIndex(DllInfo, RealDllInfo);
			Client_FillAddress_Drift(DllInfo, RealDllInfo);

			g_bIsSvenCoop = true;
		}
	}
}

void Client_FillAddress_PlayerExtraInfo(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	 //Global pointers that link into client dll vars.
	extra_player_info_t(*g_PlayerExtraInfo)[65] = NULL;
	extra_player_info_czds_t(*g_PlayerExtraInfo_CZDS)[65] = NULL;
	*/

	//66 85 C0 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ??
	/*
	.text:019A4575 66 85 C0                                            test    ax, ax
	.text:019A4578 66 89 99 20 F4 A2 01                                mov     word_1A2F420[ecx], bx
	.text:019A457F 66 89 A9 22 F4 A2 01                                mov     word_1A2F422[ecx], bp
	.text:019A4586 66 89 91 48 F4 A2 01                                mov     word_1A2F448[ecx], dx
	.text:019A458D 66 89 81 4A F4 A2 01                                mov     word_1A2F44A[ecx], ax
	*/
	if (1)
	{
		char pattern[] = "\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct MsgFunc_ScoreInfo_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					ULONG_PTR Candidates[4]{};
					int iNumCandidates{};
				} MsgFunc_ScoreInfo_SearchContext;

				MsgFunc_ScoreInfo_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((void*)pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto ctx = (MsgFunc_ScoreInfo_SearchContext*)context;
					auto pinst = (cs_insn*)inst;

					if (ctx->iNumCandidates < 4)
					{
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].size == 2)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].size == 2 &&
							pinst->detail->x86.operands[1].imm == 0)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
					}

					if (ctx->iNumCandidates == 4)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);

				if (ctx.iNumCandidates >= 3)
				{
					std::qsort(ctx.Candidates, ctx.iNumCandidates, sizeof(ctx.Candidates[0]), [](const void* a, const void* b) {
						return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
						});

					if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_czds_t, teamnumber) - offsetof(extra_player_info_czds_t, playerclass))
							==
							ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_czds_t, teamnumber));
							g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
					}
					else
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_t, teamnumber) - offsetof(extra_player_info_t, playerclass))
							== ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_t, teamnumber));
							g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
					}
				}

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}

	if (!g_PlayerExtraInfo)
	{
		//For HL25
		char pattern[] = "\x66\x89\x2A\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct MsgFunc_ScoreInfo_ctx_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					ULONG_PTR Candidates[4]{};
					int iNumCandidates{};
				} MsgFunc_ScoreInfo_ctx;

				MsgFunc_ScoreInfo_ctx ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((void*)pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto ctx = (MsgFunc_ScoreInfo_ctx*)context;
					auto pinst = (cs_insn*)inst;

					if (ctx->iNumCandidates < 4)
					{
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].size == 2)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].size == 2 &&
							pinst->detail->x86.operands[1].imm == 0)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
					}

					if (ctx->iNumCandidates == 4)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);

				if (ctx.iNumCandidates >= 3)
				{
					std::qsort(ctx.Candidates, ctx.iNumCandidates, sizeof(ctx.Candidates[0]), [](const void* a, const void* b) {
						return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
						});

					if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_czds_t, teamnumber) - offsetof(extra_player_info_czds_t, playerclass))
							==
							ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_czds_t, teamnumber));
							g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
					}
					else
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_t, teamnumber) - offsetof(extra_player_info_t, playerclass))
							== ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_t, teamnumber));
							g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
					}
				}

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}
	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror")) {
		Sig_VarNotFound(g_PlayerExtraInfo_CZDS);
	}
	else {
		Sig_VarNotFound(g_PlayerExtraInfo);
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	Client_FillAddress_CL_IsThirdPerson(DllInfo, RealDllInfo);

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "dod"))
	{
		g_bIsDayOfDefeat = true;
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		Client_FillAddress_PlayerExtraInfo(DllInfo, RealDllInfo);
	}
}

void Client_InstallHooks(void)
{
	//do nothing
}

TEMPENTITY *efxapi_R_TempModel(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype)
{
	auto r = gPrivateFuncs.efxapi_R_TempModel(pos, dir, angles, life, modelIndex, soundtype);

	if (r && g_bIsCreatingClCorpse && g_iCreatingClCorpsePlayerIndex > 0 && g_iCreatingClCorpsePlayerIndex <= gEngfuncs.GetMaxClients())
	{
		r->entity.curstate.iuser4 = PhyCorpseFlag;
		r->entity.curstate.owner = g_iCreatingClCorpsePlayerIndex;
	}

	return r;
}

void Engine_InstallHook(void)
{
	Install_InlineHook(R_NewMap);
	
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Install_InlineHook(R_RenderView_SvEngine);
	}
	else
	{
		Install_InlineHook(R_RenderView);
	}

}

void Engine_UninstallHook(void)
{
	Uninstall_Hook(R_NewMap);
	Uninstall_Hook(Mod_LoadStudioModel);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Uninstall_Hook(R_RenderView_SvEngine);
	}
	else
	{
		Uninstall_Hook(R_RenderView);
	}
}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
	if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
	{
		ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
		ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
		auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

		auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}
	else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
	{
		auto vf_VA = (ULONG_PTR)vftable[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}

	return vftable[index];
}