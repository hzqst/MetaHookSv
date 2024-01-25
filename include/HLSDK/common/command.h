#ifndef __HLSDK_COMMAND__
#define __HLSDK_COMMAND__

typedef void (*xcommand_t) (void);

typedef struct cmd_function_s
{
	struct cmd_function_s *next;
	char *name;
	xcommand_t function;
	int flags;
}cmd_function_t;

#if 0
extern cmd_function_t* (*Cmd_GetCmdBase)(void);
cmd_function_t *Cmd_FindCmd(const char *cmd_name);
xcommand_t Cmd_HookCmd(const char *cmd_name, xcommand_t newfuncs);
cmd_function_t *Cmd_FindCmdPrev(const char *cmd_name);
char *Cmd_CompleteCommand(const char *partial, qboolean next);
#endif

#endif