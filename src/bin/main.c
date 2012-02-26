#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>
#include <Ems.h>


int main(int argc, char **argv)
{
   if (!ems_init(NULL))
     return EXIT_FAILURE;

   ems_scanner_start();
   ems_avahi_start();

   ems_run();

   ems_shutdown();

   return EXIT_SUCCESS;
}
