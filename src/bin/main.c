#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>
#include <ems.h>


int main(int argc, char **argv)
{

   ems_init();

   ecore_main_loop_begin();

   ems_shutdown();

   return EXIT_SUCCESS;
}
