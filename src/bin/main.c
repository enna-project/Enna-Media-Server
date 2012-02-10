#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>
#include <Ems.h>


int main(int argc, char **argv)
{
  if (!ems_init())
    return EXIT_FAILURE;

   ecore_main_loop_begin();

   ems_shutdown();

   return EXIT_SUCCESS;
}
