#include <iostream>
#include <stdio.h>

bool writeLog = false;
#ifdef _DEBUG
bool temporaryLog = true;
#else
bool temporaryLog = false;
#endif

void _ogldbglog(const char *txt)
{
	if (!writeLog)
		return;

	FILE *FPlog;

	static bool once = true;
	const char *_logFileName = "\\debug.log";
	static char logFileName[1024];
	if (once)
	{
		once = false;
		HMODULE hm;

		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCSTR)&_ogldbglog, &hm))
		{
			GetModuleFileNameA(hm, logFileName, sizeof(logFileName));
			char *p = strrchr(logFileName, '\\');
			strcpy_s(p, sizeof(logFileName) - (p - logFileName), _logFileName);
		}
	}

	fopen_s(&FPlog, logFileName, "a");
	if (FPlog != NULL) {
		fwrite(txt, strlen(txt), 1, FPlog);
		fwrite("\n", 2, 1, FPlog);
		fclose(FPlog);
	}
}


void dbglog(const char *fmt, ...)
{
	if (!writeLog)
		return;

	static char txt[10000];
	va_list	ap;

	if (fmt == NULL)
		return;

	va_start(ap, fmt);
	vsprintf_s(txt, fmt, ap);
	va_end(ap);
	_ogldbglog(txt);
}

// ============================================================================

class HRTimer {
private:
	LARGE_INTEGER start;
	LARGE_INTEGER stop;
	double ticTackTime;

public:
	HRTimer(void) : 
		start(), 
		stop()		
	{
		LARGE_INTEGER proc_freq;
		if (!QueryPerformanceFrequency(&proc_freq))
			throw TEXT("QueryPerformanceFrequency() failed");

		ticTackTime = (double)proc_freq.QuadPart;
		ticTackTime = 1000.0 / ticTackTime;
		startTimer();
	}

	void startTimer(void)
	{
		QueryPerformanceCounter(&start);
	}

	double checkTimer(void)
	{
		QueryPerformanceCounter(&stop);

		return ((stop.QuadPart - start.QuadPart) * ticTackTime);
	}

	double restartTimer(void)
	{
		QueryPerformanceCounter(&stop);

		double ret = ((stop.QuadPart - start.QuadPart) * ticTackTime);
		start = stop;

		return ret;
	}
};

// ============================================================================

char tmpLogBuf[1024];
HRTimer heartRate;

void clearTmpLog()
{
	tmpLogBuf[0] = 0;
	if (!temporaryLog)
		return;
	heartRate.startTimer();
}

void _tmpLog(const char *txt)
{
	if (!temporaryLog)
		return;

	static char buf[1024];
	sprintf_s(buf, "%.3f: ", heartRate.restartTimer());
	strncat_s(tmpLogBuf, buf, strlen(buf));
	strncat_s(tmpLogBuf, txt, strlen(txt));
}

void tmpLog(const char *fmt, ...)
{
	if (!temporaryLog)
		return;

	va_list	ap;

	if (fmt == NULL)
		return;

	static char txt[sizeof(tmpLogBuf) - 1];

	va_start(ap, fmt);
	vsprintf_s(txt, fmt, ap);
	va_end(ap);
	_tmpLog(txt);
}

