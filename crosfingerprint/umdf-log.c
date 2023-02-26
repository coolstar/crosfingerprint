#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

void DebugLog(const char* format, ...) {
	HANDLE logFile = CreateFile(L"C:\\Windows\\Temp\\crfp-log.txt",
			FILE_APPEND_DATA,
			0,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

	char* logStor = malloc(0x1000);
	if (!logStor) {
		CloseHandle(logFile);
		return;
	}
	RtlZeroMemory(logStor, 0x1000);

	va_list args;
	va_start(args, format);
	vsprintf_s(logStor, 0x1000, format, args);
	va_end(args);

	LONG bytesWritten;
	WriteFile(logFile, logStor, strlen(logStor), &bytesWritten, NULL);

	free(logStor);
	CloseHandle(logFile);
}