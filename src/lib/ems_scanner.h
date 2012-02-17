#ifndef _EMS_SCANNER_H_
#define _EMS_SCANNER_H_

typedef enum _Ems_Scanner_State Ems_Scanner_State;

enum _Ems_Scanner_State
{
    EMS_SCANNER_STATE_IDLE,
    EMS_SCANNER_STATE_WORKING,
};

Eina_Bool ems_scanner_init(void);
void ems_scanner_shutdown(void);
void ems_scanner_start(void);
void ems_scanner_state_get(Ems_Scanner_State *state, double *percent);


#endif /* _EMS_SCANNER_H_ */
