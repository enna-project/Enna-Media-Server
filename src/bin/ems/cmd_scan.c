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
   
   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL);

   return EINA_TRUE;
}
