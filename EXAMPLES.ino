/*
 * 
  from https://stackoverflow.com/questions/5141960/get-the-current-time-in-c
  //localtime example in C
#include <stdio.h>
#include <time.h>

int main ()
  {
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  printf ( "Current local time and date: %s", asctime (timeinfo) );

  return 0;
}
 */
