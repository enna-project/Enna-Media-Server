#!/bin/bash

CFLAGS="-Wall -Wextra -O0 -g -ggdb3 $(pkg-config --libs --cflags azy ecore eina)"

gcc $CFLAGS-o client client.c EMS_Config.azy_client.c EMS_Common.c EMS_Common_Azy.c $CFLAGS

