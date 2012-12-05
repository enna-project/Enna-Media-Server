#include <Ecore.h>

#include <Ems.h>

#include "cmds.h"

static Ecore_Timer *timer = NULL;

static Eina_Bool
_timer_cb(void *data)
{
   double percent = 0.0;
   Ems_Scanner_State state;
   
   ems_scanner_state_get(&state, &percent);

   printf("State : [%d] | Progress : %3.3f\n", state, percent);

   return EINA_TRUE;
}

int
cmd_scan(int argc, char **argv)
{
   printf("Cmd Scann\n");
   
   timer = ecore_timer_add(1.0, _timer_cb, NULL);

   ems_scanner_start();
   ems_run();
   ems_shutdown();

   return EINA_TRUE;
}
