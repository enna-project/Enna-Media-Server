#!/bin/bash

CFLAGS="-Wall -Wextra -O0 -g -ggdb3 $(pkg-config --libs --cflags azy ecore eina)"

colorgcc $CFLAGS-o client client.c ems_rpc_Config.azy_client.c ems_rpc_Browser.azy_client.c ems_rpc_Common.c ems_rpc_Common_Azy.c $CFLAGS

