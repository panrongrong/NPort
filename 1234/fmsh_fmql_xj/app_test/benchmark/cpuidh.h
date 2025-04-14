#ifdef __cplusplus
extern "C" {
#endif

extern char    configdata[2][1024];
extern char    timeday[30];
extern double  theseSecs;
extern double  startSecs;
extern double  secs;
extern  double ramGB;
extern  int    megaHz;
extern int     CPUconf;
extern int     CPUavail;


 extern void local_time();
 extern void getSecs();
 extern void start_time();
 extern void end_time();
 extern int getDetails();

#ifdef __cplusplus
};
#endif


