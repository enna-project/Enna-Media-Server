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
#include <Ecore_Getopt.h>
#include <Ems.h>

static const Ecore_Getopt options = 
{
        "enna-media-server",
        "%prog [options]",
        "1.0.0",
        "(C) 2012 Nicolas Aguirre",
        "GPLv2",
        "Enna Media Server",
        EINA_TRUE,
        {
                ECORE_GETOPT_STORE_DEF_BOOL
                        ('n', "nodaemon", "Don't daemonize (stay in foreground)", 0),
                ECORE_GETOPT_VERSION
                        ('v', "version"),
                ECORE_GETOPT_COPYRIGHT
                        ('c', "copyright"),
                ECORE_GETOPT_LICENSE
                        ('l', "license"),
                ECORE_GETOPT_HELP
                        ('h', "help"),
                ECORE_GETOPT_SENTINEL
        }
};

int main(int argc, char **argv)
{
   int args;
   pid_t pid;
   pid_t sid;
   int fp;
   char str[32];
   Eina_Bool nodaemon_opt = EINA_FALSE;
   Eina_Bool quit_opt = EINA_FALSE;

   Ecore_Getopt_Value values[] = 
        {
                ECORE_GETOPT_VALUE_BOOL(nodaemon_opt),
                ECORE_GETOPT_VALUE_BOOL(quit_opt),
                ECORE_GETOPT_VALUE_BOOL(quit_opt),
                ECORE_GETOPT_VALUE_BOOL(quit_opt),
                ECORE_GETOPT_VALUE_BOOL(quit_opt),
                ECORE_GETOPT_VALUE_NONE
        };

   eina_init();
   ecore_init();

   ecore_app_args_set(argc, (const char **) argv);
   args = ecore_getopt_parse(&options, values, argc, argv);

   if (args < 0)
     {
        printf("ERROR: could not parse options.\n");
        return EXIT_FAILURE;
     }
   
   if (quit_opt)
     return EXIT_SUCCESS;

   if (!nodaemon_opt)
     {

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

       printf("%s is launched in background.\n", argv[0]);
        
       /* Close out the standard file descriptors */
       close(STDIN_FILENO);
       close(STDOUT_FILENO);
       close(STDERR_FILENO);


       fp = open("/tmp/enna-media-server.lock", 
		 O_RDWR|O_CREAT, 0640);
       printf("Fp : %d\n", fp);
       if (fp < 0)
	 return EXIT_FAILURE;

       if (lockf(fp, F_TLOCK, 0) < 0) 
	 return EXIT_FAILURE;

       snprintf(str, sizeof(str), "%d\n", getpid());
       write(fp, str, strlen(str)); /* record pid to lockfile */
     }
   
   if (!ems_init(NULL, EINA_TRUE))
     return EXIT_FAILURE;

   ems_run();
   ems_shutdown();
   return EXIT_SUCCESS;

}
