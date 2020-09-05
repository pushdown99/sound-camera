#ifndef FOURIER_H_
#define FOURIER_H_

void cdft(int, int, double *);
void rdft(int, int, double *);
void ddct(int, int, double *);
void ddst(int, int, double *);
void dfct(int, double *);
void dfst(int, double *);

#endif /* FOURIER_H_ */
