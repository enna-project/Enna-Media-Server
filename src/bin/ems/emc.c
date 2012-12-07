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

#include "cmds.h"

#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))

typedef struct _Commands
{
   const char *cmd;
   int (*fn)(int, const char **);
}Commands;

const char ems_usage_string[] = 
  "ems [--version] [--help]\n"
  "    <command>\n";

static int
_print_usage(int argc, const char **argv)
{
   printf("%s", ems_usage_string);
   return EINA_TRUE;
}

static Commands commands[] = {
  { "--help", _print_usage },
  { "scan", cmd_scan },
  { "list", cmd_list },
  { "ip", cmd_ip },
  { "port", cmd_port },
  { "medias", cmd_medias },
};

static int
_handle_options(int argc, char **argv)
{
   int i;

   while(argc > 0)
     {
        const char *cmd = argv[0];

        for (i = 0; i < ARRAY_SIZE(commands); i++) {
           Commands *c = commands + i;
           if (strcmp(c->cmd, cmd))
             continue;

           if (c->fn)
             c->fn(argc, argv);
	}

        argv++;
        argc--;
     }



   return 0;
}

int main(int argc, char **argv)
{
   if (!ems_init(NULL, EINA_FALSE))
     return EXIT_FAILURE;

   argv++;
   argc--;

   _handle_options(argc, argv);

   ems_run();
   ems_shutdown();

   return EXIT_SUCCESS;
}
