#include <metahook.h>

#include "exportfuncs.h"

#include "command.h"

cmd_function_t *(*Cmd_GetCmdBase)(void);

cmd_function_t *Cmd_FindCmd(char *cmd_name)
{
	for (cmd_function_t *cmd = Cmd_GetCmdBase(); cmd; cmd = cmd->next)
	{
		if (!strcmp(cmd->name, cmd_name))
			return cmd;
	}

	return NULL;
}

cmd_function_t *Cmd_FindCmdPrev(char *cmd_name)
{
	cmd_function_t *cmd;

	for (cmd = Cmd_GetCmdBase()->next; cmd->next; cmd = cmd->next)
	{
		if (!strcmp(cmd_name, cmd->next->name))
			return cmd;
	}

	return NULL;
}

xcommand_t Cmd_HookCmd(char *cmd_name, xcommand_t newfuncs)
{
	cmd_function_t *cmd = Cmd_FindCmd(cmd_name);
	xcommand_t result = cmd->function;
	cmd->function = newfuncs;
	return result;
}

char *Cmd_CompleteCommand(char *partial, qboolean next)
{
	cmd_function_t *cmd;
	int len;
	char ppartial[256];
	static char lastpartial[256];
	int i;
	char *name = NULL;

	strncpy(ppartial, partial, sizeof(ppartial));
	ppartial[sizeof(ppartial) - 1] = 0;

	len = strlen(ppartial);

	if (!len)
		return NULL;

	for (i = len; partial[i] == ' '; i--)
		partial[i] = 0;

	if (!stricmp(partial, lastpartial))
	{
		cmd = Cmd_FindCmd(partial);

		if (cmd)
		{
			cmd = (next) ? cmd->next : Cmd_FindCmdPrev(cmd->name);

			if (cmd)
			{
				name = cmd->name;
				goto done;
			}
		}
	}

	for (cmd = Cmd_GetCmdBase(); cmd; cmd = cmd->next)
	{
		if (!strncmp(ppartial, cmd->name, len))
		{
			if (strlen(cmd->name) == len)
			{
				name = cmd->name;
				goto done;
			}

			if (cmd)
			{
				name = cmd->name;
				goto done;
			}
		}
	}

	if (name)
	{
done:
		strncpy(lastpartial, name, sizeof(lastpartial));
		lastpartial[sizeof(lastpartial) - 1] = 0;
		return name;
	}

	return NULL;
}