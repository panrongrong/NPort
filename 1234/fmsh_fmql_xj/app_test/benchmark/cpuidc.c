/*
;  nasm -f elf cpuida.asm   for cpuida.o
;  gcc cpuidc.c -c          for cpuidc.o
;  gcc test.c cpuidc.o cpuida.o -lrt -lc -lm -o test
;  ./test
*/

  #include <stdio.h>
  #include "cpuidh.h"
  #include <stdlib.h>     
  #include <string.h> 
  #include <time.h>
  #include <math.h>
  #include <sys/time.h>
  #include <sys/resource.h>

  char    configdata[2][1024];
  char    timeday[30];
  char    idString1[100] = " ";
  char    idString2[100] = " ";
  double  theseSecs = 0.0;
  double  startSecs = 0.0;
  double  secs;
  struct  timespec tp1;

  double  ramGB;

  FILE * info;

  int CPUconf;
  int CPUavail;
 

  unsigned int millisecs  = 0;
  

/*  #include <sys/utsname.h> */


  void local_time()
  {
     time_t t;

     t = time(NULL);
     sprintf(timeday, "%s", asctime(localtime(&t)));
     return;
  }

  void getSecs()
  {
     clock_gettime(CLOCK_REALTIME, &tp1);

     theseSecs =  tp1.tv_sec + tp1.tv_nsec / 1e9;               
     return;
  }

  void start_time()
  {
      getSecs();
      startSecs = theseSecs;
      return;
  }
  

  void end_time()
  {
      getSecs();
      secs = theseSecs - startSecs;
      millisecs = (int)(1000.0 * secs);
      return;
  }   
  
  double start_time2()
  {
      getSecs();
      /*startSecs = theseSecs;*/
      return theseSecs;
  }
  
  double end_time2()
  {
      getSecs();
      /*secs = theseSecs - startSecs;*/
      /*millisecs = (int)(1000.0 *secs);*/
      return theseSecs;
  }   

  int getDetails()
  {
     size_t bytes;

     return 0; 
  }

