#include "../internal.h"

typedef void	(*DELIVER) (float*,  unsigned int );
//typedef void (__cdecl *DELIVER)(float *, unsigned int);

#define SSRC_API 

int* init_ssrc(unsigned long SFRQ,unsigned long DFRQ,unsigned int NCH, DELIVER encode_routine, int** ENDING, IScriptEnvironment* env);


SSRC_API void downsample(REAL *inbuf,unsigned int nsmplread);
SSRC_API deinit_ssrc(void);
SSRC_API int init_shaper(int freq,int nch,int min,int max,int dtype);
SSRC_API int do_shaping(double s,double *peak,int dtype,int ch);
SSRC_API void quit_shaper(int nch);
SSRC_API void ssrc_callback(DELIVER encode_routine);
SSRC_API void ssrc_init(int INIT);
SSRC_API void ssrc_ending(int ENDING);