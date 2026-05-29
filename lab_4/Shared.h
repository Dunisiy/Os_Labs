#pragma once
#include <windows.h>

#define NUM_PAGES 9
#define MAX_CONCURRENT_READERS 50
#define MAX_WRITES_PER_PAGE 10

struct SharedMeta {
	LONG readerCount[NUM_PAGES];
	BOOL hasData[NUM_PAGES];
	LONG writeCount[NUM_PAGES];
	LONG readCount[NUM_PAGES];
	BOOL writersFinished;
};