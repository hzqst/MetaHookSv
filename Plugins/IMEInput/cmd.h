typedef struct cmd_function_s
{
	struct cmd_function_s *next;
	char *name;
	void (*function)(void);
	int flags;
}
cmd_function_t;