/*
 * misc.h
 *
 *  Created on: Sep 2, 2020
 *      Author: hjlee
 */

#ifndef MISC_H_
#define MISC_H_

long  getTimestamp (void);
int   getUnixtime (void);
char* getTime();
char* dumpUInt32(unsigned int *list, int len, char* out);

#endif /* MISC_H_ */
