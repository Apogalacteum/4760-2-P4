/* Author: Alexander Hughey
 * CS4760 F2020
 * Project 3 - OS Shell
 * main driver for project 3
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
  int child_max = 5;
  int countdown = 20;
  char * log_file = "logfile.txt";
  pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
  //signal(SIGINT, sighandler);
  
  //processes command line arguments
  //options are -h, -c x, -l file, -t time
  while((opt = getopt(argc, argv, ":hc:l:t:")) != -1)
  {
    switch(opt)
    {
      case 'h':
        printf("Usage:\n");
        printf("./oss [-c x] [-l fielname] [-t time]\n");
        printf("\t-c x\tindicate the maximum total of child processes master will ");
        printf("\n\t\tspawn. (Default 5)\n");
        printf("\t-l x\tspecifies the name of the log file.\n");
        printf("\t-t time\tdetermines the time in seconds when the master will ");
        printf("\n\t\tterminate itself and all children (default 20).\n");
        break;
      case 'c':
        child_max = atoi(optarg);
        if(child_max >= 20)
        {
          printf("no more than 19 children may exist at once\n");
          printf("you attempted %d\n", child_max);
          child_max = 19;
          printf("value has been defaulted to %d...\n", child_max);
        }
        else
          printf("called with c option %d value\n", child_max);
        break;
      case 'l':
        log_file = optarg;
        printf("called with l option %d value\n", log_file);
        break;
      case 't':
        countdown = atoi(optarg);
        printf("called with t option %d value\n", countdown);
        break;
      case ':':
        fprintf(stderr, "option needs a value\n");
        perror("ERROR: ");
        return(-1);
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
  
  // clearing/creating log file
  FILE *fp1; 
  fp1 = fopen(log_file, "w");
  fprintf(fp1, "");
  fclose(fp1);
  
  // create shared memory for clock.
  // seconds, nanosecond, shmPID (used by child processes to indicate when they have
  // terminated.)
  //keys for shmget
  key_t sec_key; 
  key_t ns_key;
  key_t shmPID_key;
  //ids returned from shmget
  int sec_shmid;
  int ns_shmid;
  int shmPID_shmid;
  //shared mem size (same for all 3)
  int size;
  
  //inserting values
  sec_key = 909090;
  ns_key = 808080;
  shmPID_key = 707070;
  size = sizeof(int);
  
  //creating shared memory for seconds, nanosecond, PID flag
  if((sec_shmid = shmget( sec_key, size, 0666 | IPC_CREAT)) == -1)
  {
    perror("failed to create shared memory for clock seconds");
    return -1;
  }//end of if
  if((ns_shmid = shmget( ns_key, size, 0666 | IPC_CREAT)) == -1)
  {
    perror("failed to create shared memory for clock nanoseconds");
    return -1;
  }//end of if
  if((shmPID_shmid = shmget( shmPID_key, size, 0666 | IPC_CREAT)) == -1)
  {
    perror("failed to create shared memory for shmPID");
    return -1;
  }//end of if
  
  //////////////////////////////////////////////////////////////////////
  //////////////////////ATTACH//AND//DETACH//BLOCK//////////////////////
  //////////////////////////////////////////////////////////////////////
  int *shm_sec;
  int *shm_nan;
  int *shm_PID;
  
  if((shm_sec = shmat( sec_shmid, NULL, 0)) == (int *) -1)
  {
    perror("failed to attach shared memory for clock seconds");
    return -1;
  }//end of if
  if((shm_nan = shmat( ns_shmid, NULL, 0)) == (int *) -1)
  {
    perror("failed to attach shared memory for clock nanoseconds");
    return -1;
  }//end of if
  if((shm_PID = shmat( shmPID_shmid, NULL, 0)) == (int *) -1)
  {
    perror("failed to attach shared memory for shmPID");
    return -1;
  }//end of if
  
  //setting values
  *shm_sec = 0;
  *shm_nan = 0;
  *shm_PID = 0;
  
  /*debug
  printf("1. shm sec in main = %d\n", *shm_sec);
  printf("1. shm nan in main = %d\n", *shm_nan);
  printf("1. shm PID in main = %d\n", *shm_PID);*/
  
  //detaching shared memory segments
  if((shmdt(shm_sec)) == -1)
  {
    perror("failed to detach shared memory for clock seconds");
    return -1;
  }//end of if
  if((shmdt(shm_nan)) == -1)
  {
    perror("failed to detach shared memory for clock nanoseconds");
    return -1;
  }//end of if
  if((shmdt(shm_PID)) == -1)
  {
    perror("failed to detach shared memory for shmPID");
    return -1;
  }//end of if
  //////////////////////////////////////////////////////////////////////
  ////////////////////////////END//OF//BLOCK////////////////////////////
  //////////////////////////////////////////////////////////////////////
  
  //array of character arrays used as an argument for exec
  /*debug
  printf("sending values\nsec_shmid:%d\nns_shmid:%d\n", sec_shmid, ns_shmid);
  printf("PID_shmid:%d\n", shmPID_shmid);*/
  char sec_id_str[50];  
  char nan_id_str[50];
  char PID_id_str[50];
  sprintf(sec_id_str, "%d", sec_shmid);  
  sprintf(nan_id_str, "%d", ns_shmid); 
  sprintf(PID_id_str, "%d", shmPID_shmid); 
  
  char *args[]={"./user", sec_id_str, nan_id_str, PID_id_str,NULL};
  //iterator
  int it = 0;
  //launches initial children
  for( it = 1; it <= child_max ; it++ )
  {
    if(fork() == 0)//child enter
    { 
      execvp(args[0],args);
      return 0;
    }//end of if
  }//end of for loop L1
  
  
  /*debug
  if(fork() == 0)
  {
    execvp(args[0], args);
    return 0;
  }
  wait(NULL);*/
  
  //////////////////////////////////////////////////////////////////////
  //////////////////////ATTACH//AND//DETACH//BLOCK//////////////////////
  //////////////////////////////////////////////////////////////////////
  if((shm_sec = shmat( sec_shmid, NULL, 0)) == (int *) -1)
  {
    perror("failed to attach shared memory for clock seconds");
    return -1;
  }//end of if
  if((shm_nan = shmat( ns_shmid, NULL, 0)) == (int *) -1)
  {
    perror("failed to attach shared memory for clock nanoseconds");
    return -1;
  }//end of if
  if((shm_PID = shmat( shmPID_shmid, NULL, 0)) == (int *) -1)
  {
    perror("failed to attach shared memory for shmPID");
    return -1;
  }//end of if
  
  /*debug
  printf("3. shm sec in main = %d\n", *shm_sec);
  printf("3. shm nan in main = %d\n", *shm_nan);
  printf("3. shm PID in main = %d\n", *shm_PID);*/
  
  //detaching shared memory segments
  if((shmdt(shm_sec)) == -1)
  {
    perror("failed to detach shared memory for clock seconds");
    return -1;
  }//end of if
  if((shmdt(shm_nan)) == -1)
  {
    perror("failed to detach shared memory for clock nanoseconds");
    return -1;
  }//end of if
  if((shmdt(shm_PID)) == -1)
  {
    perror("failed to detach shared memory for shmPID");
    return -1;
  }//end of if
  //////////////////////////////////////////////////////////////////////
  ////////////////////////////END//OF//BLOCK////////////////////////////
  //////////////////////////////////////////////////////////////////////
  
  int child_tot = child_max; //total number of child processes
  int checkpoint = 0; //holds ns since last process termination
  int sec_temp = 0; //holds seconds from shared memory clock
  
  
  while( checkpoint < 2000000000 && child_tot < 100 && sec_temp < countdown)
  {
    //pthread_mutex_lock( &myMutex );
    //////////////////////////////////////////////////////////////////////
    //////////////////////ATTACH//AND//DETACH//BLOCK//////////////////////
    //////////////////////////////////////////////////////////////////////
    if((shm_sec = shmat( sec_shmid, NULL, 0)) == (int *) -1)
    {
      perror("failed to attach shared memory for clock seconds");
      return -1;
    }//end of if
    if((shm_nan = shmat( ns_shmid, NULL, 0)) == (int *) -1)
    {
      perror("failed to attach shared memory for clock nanoseconds");
      return -1;
    }//end of if
    if((shm_PID = shmat( shmPID_shmid, NULL, 0)) == (int *) -1)
    {
      perror("failed to attach shared memory for shmPID");
      return -1;
    }//end of if
    
    sec_temp = *shm_sec;
    *shm_nan = *shm_nan + 100;
    printf("In loop: %d sec_temp\n", sec_temp);
    if(*shm_nan >= 1000000000)
    {
      *shm_sec = *shm_sec + 1;
      *shm_nan = 0;
    }
    if(*shm_PID > 0)
    {
      FILE *fp; 
      fp = fopen(log_file, "w");
      fprintf(fp, "Child pid %d is terminating at system clock time ", *shm_PID);
      fprintf(fp, "%d:%d", *shm_sec, *shm_nan);
      fclose(fp);
      *shm_PID = 0;
      child_tot++;
      if(fork() == 0)//child enter
      { 
        execvp(args[0],args);
        return 0;
      }//end of if
    }
    
    //detaching shared memory segments
    if((shmdt(shm_sec)) == -1)
    {
      perror("failed to detach shared memory for clock seconds");
      return -1;
    }//end of if
    if((shmdt(shm_nan)) == -1)
    {
      perror("failed to detach shared memory for clock nanoseconds");
      return -1;
    }//end of if
    if((shmdt(shm_PID)) == -1)
    {
      perror("failed to detach shared memory for shmPID");
      return -1;
    }//end of if
    //////////////////////////////////////////////////////////////////////
    ////////////////////////////END//OF//BLOCK////////////////////////////
    //////////////////////////////////////////////////////////////////////
    sleep(1);
    //pthread_mutex_unlock( &myMutex );
  }//end of while loop L1
  
  wait(NULL);
  
  ////////////////destroy shm
  
  if((shmctl( sec_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for sec");
    return -1;
  }//end of if
  
  if((shmctl( ns_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for nanosec");
    return -1;
  }//end of if
  
  if((shmctl( shmPID_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for shmPID");
    return -1;
  }//end of if
  
  
  return 0;
}//end of main

/*
void sighandler(int signum) 
{
  printf("Caught signal %d, exiting...\n", signum);
  
  //////////////////detaching shared memory segments
  if((shmdt(shm_sec)) == -1)
  {
    perror("failed to detach shared memory for clock seconds");
    return;
  }//end of if
  if((shmdt(shm_nan)) == -1)
  {
    perror("failed to detach shared memory for clock nanoseconds");
    return;
  }//end of if
  if((shmdt(shm_PID)) == -1)
  {
    perror("failed to detach shared memory for shmPID");
    return;
  }//end of if
  
  //////////////////destroying shared memory segments
  if((shmctl( sec_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for sec");
    return;
  }//end of if
  if((shmctl( ns_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for nanosec");
    return;
  }//end of if
  if((shmctl( shmPID_shmid, IPC_RMID, NULL)) == -1)
  {
    perror("failed to destroy shared memory for shmPID");
    return;
  }//end of if
  
  exit(1);
}
*/