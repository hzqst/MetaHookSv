#include <metahook.h>
#include "parsemsg.h"
#include "msghook.h"

typedef struct usermsg_s
{
	int index;
	int size;
	char name[16];
	struct usermsg_s *next;
	pfnUserMsgHook function;
}usermsg_t;

usermsg_t **gClientUserMsgs = NULL;

void MSG_Init(void)
{
	DWORD address = (DWORD)g_pMetaSave->pEngineFuncs->pfnHookUserMsg;

	if (*(BYTE *)(address + 0x1A) != 0xE8)
		address += 0x19;
	else
		address += 0x1A;

	address += 0x1;
	address += *(DWORD *)address + 0x4;

	if (*(BYTE *)(address + 0xC) != 0x35)
		address += 0x9;
	else
		address += 0xC;

	gClientUserMsgs = *(usermsg_t ***)(address + 0x1);
}

usermsg_t *MSG_FindUserMsgHook(char *szMsgName)
{
	for (usermsg_t *msg = *gClientUserMsgs; msg; msg = msg->next)
	{
		if (!strcmp(msg->name, szMsgName))
			return msg;
	}

	return NULL;
}

usermsg_t *MSG_FindUserMsgHookPrev(char *szMsgName)
{
	for (usermsg_t *msg = (*gClientUserMsgs)->next; msg->next; msg = msg->next)
	{
		if (!strcmp(msg->next->name, szMsgName))
			return msg;
	}

	return NULL;
}

pfnUserMsgHook MSG_HookUserMsg(char *szMsgName, pfnUserMsgHook pfn)
{
	usermsg_t *msg = MSG_FindUserMsgHook(szMsgName);

	if (msg)
	{
		pfnUserMsgHook result = msg->function;
		msg->function = pfn;
		return result;
	}

	gEngfuncs.pfnHookUserMsg(szMsgName, pfn);
	return pfn;
}

pfnUserMsgHook MSG_UnHookUserMsg(char *szMsgName)
{
	usermsg_t *msg = MSG_FindUserMsgHook(szMsgName);

	if (msg)
	{
		usermsg_t *prev = MSG_FindUserMsgHookPrev(szMsgName);

		if (prev)
		{
			prev->next = msg->next;
			return msg->function;
		}
	}

	return NULL;
}