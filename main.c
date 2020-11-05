/* Author: Alexander Hughey
 * CS4760 F2020
 * Project 4 - Process Scheduling
 * main driver for project 4
 * invoked as "oss"
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/msg.h> 
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>

//void sighandler(int);

int main(int argc, char* argv[])
{
  //variable declarations for command line options and defaults
  int opt;
  srand(time(0));
  const int maxTimeBetweenNewProcsNS = (rand()%2);
  const int maxTimeBetweenNewProcsSecs = (rand()%1000000);
  printf("rand NS=%d rand S=%d\n",maxTimeBetweenNewProcsNS,maxTimeBetweenNewProcsSecs);
  char * log_file = "logfile.txt";
  
  //////////BEGIN COMMAND LINE PARSING//////////
  //options are -h, -c x, -l file, -t time
  while((opt = getopt(argc, argv, ":hc:l:t:")) != -1)
  {
    switch(opt)
    {
      case 'h':
        printf("Usage:\n");
        printf("./oss\n");
        break;
      case'?':
        printf("unknown option\n");
        perror("ERROR: ");
        return(-1);
        break;
    }//end of switch
  }//end of while loop
  
  //catches extra arguments
  for(; optind < argc; optind++)
  { 
    char * test_str_file = "bb.bb";
    test_str_file = argv[optind];
    printf("extra argument: %s\n", test_str_file);  
  }//end of for loop
  //////////END COMMAND LINE PARSING//////////
  
  // clearing/creating log file
  FILE *fp1; 
  fp1 = fopen(log_file, "w");
  fprintf(fp1, "");
  fclose(fp1);
  
  //creating structure for process control block
  struct pcb
  {
    int time_cpu;
    int time_tot;
    int time_last;
    int sim_PID;
    int priority;
  };

  struct pcb test1;
  test1.time_cpu = 8000000;
  test1.time_tot = 9000;
  test1.time_last = 9000;
  test1.sim_PID = 1;
  test1.priority = 2;
  
  struct pcb pct[18];
  pct[0] = test1;
  
  printf("test1 time_cpu = %d\n", pct[0].time_cpu);
  
  // create shared memory for clock.
  // seconds, nanosecond, pcb
  //keys for shmget
  key_t sec_key; 
  key_t ns_key;
  key_t pct_key;
  //ids returned from shmget
  int sec_shmid;
  int ns_shmid;
  int pct_shmid;
  //shared mem size (same for all 3)
  int size_clock;
  int size_table;
  
  //inserting values
  sec_key = 909090;
  ns_key = 808080;
  pct_key = 707070;
  size_clock = sizeof(int);
  size_table = (sizeof(struct pcb))*20;
  
  //creating shared memory for seconds, nanosecond, process control table
  if((sec_shmid = shmget( sec_key, size_clock, 0666 | IPC_CREAT)) == -1)
  {
    perror("failed to create shared memory for clock seconds");
    return -1;
  }//end of if
  if((ns_shmid = shmget( ns_key, size_clock, 0666 | IPC_CREAT)) == -1)
  {
    perror("failed to create shared memory for clock nanoseconds");
    //destroying previously allocated shared memory segments
    if((shmctl( sec_shmid, IPC_RMID, NULL)) == -1)
    {
      perror("failed to destroy shared memory for sec");
      return -1;
    }//end of if
    return -1;
  }//end of if
  if((pct_shmid = shmget( pct_key, size_table, 0666 | IPC_CREAT)) == -1)
  {
    perror("failed to create shared memory for proces control table");
    //destroying previously allocated shared memory segments
    if((shmctl( sec_shmid, IPC_RMID, NULL)) == -1)
    {
      perror("failed to destroy shared memory for sec inside pct creation");
      return -1;
    }//end of if
    if((shmctl( ns_shmid, IPC_RMID, NULL)) == -1)
    {
      perror("failed to destroy shared memory for nanosec");
      return -1;
    }//end of if
    return -1;
  }//end of if
  
  //destroying shared memory segments
  if((shmctl( sec_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for sec at end");
    return -1;
  }//end of if
  if((shmctl( ns_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for nanosec");
    return -1;
  }//end of if
  if((shmctl( pct_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for shmPID");
    return -1;
  }//end of if
  
  return 0;
}//end of main