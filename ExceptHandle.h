struct EXCEPTION_REGISTRATION
{
	EXCEPTION_REGISTRATION *prev;
	FARPROC handler;
};

extern "C" int _except_handler3(PEXCEPTION_RECORD, EXCEPTION_REGISTRATION *, PCONTEXT, PEXCEPTION_RECORD);
extern "C" int _except_handler4(PEXCEPTION_RECORD, EXCEPTION_REGISTRATION *, CONTEXT *, void *);

#define SetupExceptHandler3() \
	DWORD handler = (DWORD)_except_handler3; \
\
	__asm push handler \
	__asm push FS:[0] \
	__asm mov FS:[0], ESP

#define SetupExceptHandler4() \
	DWORD handler = (DWORD)_except_handler4; \
\
	__asm push handler \
	__asm push FS:[0] \
	__asm mov FS:[0], ESP