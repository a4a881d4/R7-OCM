#include "gr_complex.h"
#define BURST_SIZE 148
extern "C"
{

static int dump(float *metrics,int *tr)
{
   int i;
   float a=1e30;
   int k = 0;
   for(i=0;i<4;i++)
   {
      if(metrics[i]<a){
         a = metrics[i];
         k = i;
      }
   }
   for(i=0;i<4;i++)
   {
      if (i==k) printf("%d*",i>>1);
      printf("%d:%4.2f \t",i,metrics[i]);
   }
  for(i=0;i<4;i++)
    printf("%d",tr[i]);
   printf("\n");
   return k>>1;
}

static void build_t(gr_complex restore[2][8], gr_complex *rhh)
{
  int i;
  restore[0][0] =    gr_complex(0,-1)*(rhh[0]+rhh[2])-rhh[1]; // -j,-1,-j
  restore[0][1] =    gr_complex(0,1)*(rhh[2]-rhh[0])-rhh[1]; // -j,-1,+j
  restore[0][2] =    gr_complex(0,-1)*(rhh[0]+rhh[2])+rhh[1]; // -j,+1,-j
  restore[0][3] =    gr_complex(0,1)*(rhh[2]-rhh[0])+rhh[1]; // -j,+1,+j
  restore[1][0] = (-rhh[0]-rhh[2])-gr_complex(0,1)*rhh[1]; // -1,-j,-1
  restore[1][1] = (+rhh[2]-rhh[1])-gr_complex(0,1)*rhh[1]; // -1,-j,+1
  restore[1][2] = (-rhh[0]-rhh[2])+gr_complex(0,1)*rhh[1]; // -1,+j,-1
  restore[1][3] = (+rhh[2]-rhh[1])+gr_complex(0,1)*rhh[1]; // -1,+j,+1
  for(i=4;i<8;i++){
    restore[0][i] = -restore[0][i^0x7];
    restore[1][i] = -restore[1][i^0x7];
  }
  for(i=0;i<8;i++)
    printf("%2.1f+j%2.1f ",restore[0][i].real(),restore[0][i].imag());
  printf("\n");
  for(i=0;i<8;i++)
    printf("%2.1f+j%2.1f ",restore[1][i].real(),restore[1][i].imag());
  printf("\n");
}
static inline float norm2(gr_complex a,gr_complex b) 
{
  gr_complex x=a-b;
  return x.real()*x.real()+x.imag()*x.imag();
}
void viterbi_detector(const gr_complex * input
	, unsigned int samples_num
	, gr_complex * rhh
	, unsigned int start_state
	, const unsigned int * stop_states
	, unsigned int stops_num
	, int * output
  , int * hard )
{
	gr_complex restore[2][8];
	float M0[4],M1[4];
	float *n,*o,*tmp;
	int i,j;
	int r_i;
	int sample_nr;
	int tracback[BURST_SIZE][4];
	float pm_candidate0,pm_candidate1;
	gr_complex x;
	for(i=0;i<4;i++){
		M0[i]=1e30;
	}
	M0[start_state]=0.;
	build_t(restore,rhh);
  
  sample_nr=0;
    o=M0;
    n=M1;
    r_i = 1;
    while(sample_nr<samples_num) {
#define update(s0,s1,p0,p1,su) \
    	pm_candidate0 = o[s0] + norm(input[sample_nr]-restore[r_i][p0]); \
    	pm_candidate1 = o[s1] + norm(input[sample_nr]-restore[r_i][p1]); \
    	if(pm_candidate0 < pm_candidate1){ \
        	n[su] = pm_candidate0; \
        	tracback[sample_nr][su] = 0; \
      	} \
      	else{ \
        	n[su] = pm_candidate1; \
        	tracback[sample_nr][su] = 1; \
      	} \

      	update(0,2,0,4,0);
      	update(0,2,1,5,1);
     	  update(1,3,2,6,2);
      	update(1,3,3,7,3);
        printf("\t%d:%2.1f+j%2.1f \t",sample_nr,input[sample_nr].real(),input[sample_nr].imag());
      	hard[sample_nr]=dump(n,tracback[sample_nr]);
        r_i = !r_i;
      	tmp = o;
      	o = n;
      	n = tmp;
      	sample_nr++;
    }
   	unsigned int best_stop_state;
    float stop_state_metric, min_stop_state_metric;
    best_stop_state = stop_states[0];
    min_stop_state_metric = o[best_stop_state];
    for(i=1; i< stops_num; i++){
      stop_state_metric = o[stop_states[i]];
      if(stop_state_metric < min_stop_state_metric){
         min_stop_state_metric = stop_state_metric;
         best_stop_state = stop_states[i];
      }
   }
   sample_nr=samples_num;
   unsigned int state_nr=best_stop_state;
   unsigned int decision;
   
   while(sample_nr>0){
      sample_nr--;
      decision = tracback[sample_nr][state_nr];
      output[sample_nr]=decision;
      state_nr += 4*decision;
      state_nr >>= 1;
      //printf("%d ",state_nr);
   }
}
void viterbi_restore(int * input
  , unsigned int samples_num
  , gr_complex * rhh
  , unsigned int start_state
  , int r_i
  , gr_complex * output
  )
{
  gr_complex restore[2][8];
  build_t(restore,rhh);
  int sample_nr;
  int state = start_state;
  sample_nr=0;
  while(sample_nr<samples_num){
    state *= 2;
    state += input[sample_nr];
    state &= 0x7;
    output[sample_nr] = restore[r_i][state];
    sample_nr++;
    r_i = !r_i;
  }
}
} //extern "C"