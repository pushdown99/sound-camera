#include "soundcam.h"

long getTimestamp (void)
{
	struct timespec time_s;

	clock_gettime(CLOCK_MONOTONIC, &time_s);
	return (time_s.tv_sec*1000 + time_s.tv_nsec/1000000);
}

int getUnixtime (void)
{
	struct timespec time_s;

	clock_gettime(CLOCK_MONOTONIC, &time_s);
	return (time_s.tv_sec);
}

char* getTime() {
	time_t t = time(0);
	return ctime(&t);
}

char* dumpUInt32(unsigned int *list, int len, char* out)
{
	out[0] = 0;
	for(int i = 0; i < len; i++) {
		char buf[32];
		sprintf(buf, "%s[%d,%d]", ((i==0)? "":","), i, list[i]);
		strcat(out, buf);
	}
	return out;
}

