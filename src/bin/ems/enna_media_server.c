#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#include <Ecore.h>
#include <Ems.h>

int main(int argc, char **argv)
{

   pid_t pid;
   pid_t sid;
   int fp;
   char str[32];

   pid = fork();
   if (pid < 0)
     return EXIT_FAILURE;

   if (pid > 0)
     return EXIT_SUCCESS;
 
   umask(027);

   sid = setsid();
   if (sid < 0)
     return EXIT_FAILURE;
        
        
   if ((chdir("/")) < 0)
     return EXIT_FAILURE;
        
   /* Close out the standard file descriptors */
   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);


   fp = open("/run/lock/enna-media-server.lock", 
             O_RDWR|O_CREAT, 0640);
   printf("Fp : %d\n", fp);
   if (fp < 0)
     return EXIT_FAILURE;

   if (lockf(fp, F_TLOCK, 0) < 0) 
     return EXIT_FAILURE;

   snprintf(str, sizeof(str), "%d\n", getpid());
   write(fp, str, strlen(str)); /* record pid to lockfile */

   if (!ems_init(NULL))
     return EXIT_FAILURE;
        
   ems_run();
   ems_shutdown();
   return EXIT_SUCCESS;

}
