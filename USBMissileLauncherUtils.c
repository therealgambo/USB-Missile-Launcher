/**
 * @filename USBMissileLauncherUtils.c
 *
 * @brief USB Missile Launcher and USB Circus Cannon Utilities
 *
 * @copyright Copyright (C) 2006-2008 Luke Cole
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 * 
 * 
 * @author  Luke Cole
 *          luke@coletek.org
 *
 * @version
 *   $Id$
 */

#ifndef DEPEND
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <linux/input.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<ctype.h>
#endif
#include "InputEvent.h"
#include "USBMissileLauncher.h"

#define USB_TIMEOUT 1000 /* milliseconds */
#define PORT 20000   //The port on which to listen for incoming data

int debug_level = 0;
missile_usb *control;

//=============================================================================

int HandleKeyboardEvent(struct input_event *ev, int events, int device_type);

//=============================================================================

void usage(char *filename) {
  fprintf(stderr, "\n"
      "=================================================================\n"
	  "=       M&S USB Missile Launcher - Created by Luke Cole         =\n"
	  "=          Network Support added by Kris Gambirazzi             =\n"
	  "=================================================================\n"
	  "   Usage: %s [option]\n\n"

	  "   Options:\n"
	  "   -h            Help\n"
	  "   -n            UDP Server. Port: 20000\n"
	  "   -F            Fire Missile\n"
	  "   -L            Rotate Left\n"
	  "   -R            Rotate Right\n"
	  "   -U            Tilt Up\n"
	  "   -D            Tilt Down\n"
	  "   -S n          Stop after n milliseconds (> 100)\n",
	  
	  filename);
}

//=============================================================================

void UDPServer(){
   int sockfd;
   int device_type = 1;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char buf[512];
   char *up = "U";
   char *down = "D";
   char *left = "L";
   char *right = "R";
   char *fire = "F";
   char *stop = "S";
   char msg = 0x00;

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(20000);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
   
   fprintf(stderr, "Waiting for connections.... (%s:%d)\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
   
   for (;;)
   {
      len = sizeof(cliaddr);
      recvfrom(sockfd,buf,512,0,(struct sockaddr *)&cliaddr,&len);

	  fprintf(stderr, "Received command %s from %s\n", buf, inet_ntoa(cliaddr.sin_addr));         
		
		if (strncmp(buf,left,1) == 0)
		  msg = MISSILE_LAUNCHER_CMD_LEFT;
    
		else if (strncmp(buf,right,1) == 0)
		  msg = MISSILE_LAUNCHER_CMD_RIGHT;
		
		else if (strncmp(buf,up,1) == 0)
		  msg = MISSILE_LAUNCHER_CMD_UP;
		
		else if (strncmp(buf,down,1) == 0)
		  msg = MISSILE_LAUNCHER_CMD_DOWN;
		
		else if (strncmp(buf,fire,1) == 0)
		  msg = MISSILE_LAUNCHER_CMD_FIRE;
		  
		else if (strncmp(buf,stop,1) == 0)
		  msg = MISSILE_LAUNCHER_CMD_STOP;

		
		missile_do(control, msg, device_type);
		usleep(300 * 1000);
		missile_do(control, MISSILE_LAUNCHER_CMD_STOP, device_type);
   }

	close(sockfd);
}

//=============================================================================

int main(int argc, char **argv) {
  const char *optstring = "hnFLRUDS:";

  int o;
  int delay = 0;

  int device_type = 1;

  unsigned int set_fire = 0, set_left = 0, set_right = 0;
  unsigned int set_up = 0, set_down = 0, set_stop = 0, set_net = 0;

  //---------------------------------------------------------------------------
  // Get args

  o = getopt(argc, argv, optstring);

  while (o != -1) {
    switch (o) {

    case 'h':
      usage(argv[0]);
      return 0;

    case 'n':
      set_net = 1;
      break;

    case 'F':
      set_fire = 1;
      break;

    case 'U':
      set_up = 1;
      break;

    case 'D':
      set_down = 1;
      break;

    case 'L':
      set_left = 1;
      break;

    case 'R':
      set_right = 1;
      break;

    case 'S':
      set_stop = 1;
      delay = atoi(optarg);
      break;

    }
    o = getopt(argc, argv, optstring);
  }

  //---------------------------------------------------------------------------

  if (missile_usb_initialise() != 0) {
    fprintf(stderr, "missile_usb_initalise failed: %s\n", strerror(errno));
    return -1;
  }
  
  control = missile_usb_create(debug_level, USB_TIMEOUT);
  if (control == NULL) {
    fprintf(stderr, "missile_usb_create() failed\n");
    return -1;
  }
  
  if (missile_usb_finddevice(control, 0, device_type) != 0) {
    fprintf(stderr, "USBMissileLauncher device not found\n");
    return -1;
  }

  //---------------------------------------------------------------------------

  char msg = 0x00;

  switch (device_type) {
    
  case DEVICE_TYPE_MISSILE_LAUNCHER:
  
    if (set_left)
      msg |= MISSILE_LAUNCHER_CMD_LEFT;
    
    if (set_right)
      msg |= MISSILE_LAUNCHER_CMD_RIGHT;
    
    if (set_up)
      msg |= MISSILE_LAUNCHER_CMD_UP;
    
    if (set_down)
      msg |= MISSILE_LAUNCHER_CMD_DOWN;
    
    if (set_fire)
      msg |= MISSILE_LAUNCHER_CMD_FIRE;

    missile_do(control, msg, device_type);
    
    if (set_stop) {
      usleep(delay * 1000);
      missile_do(control, MISSILE_LAUNCHER_CMD_STOP, device_type);
    }

    break;
    
  default:
    printf("Device Type (%d) not implemented, please do it!\n",
	   device_type);
    return -1;
    
  }
  
  if(set_net){
	UDPServer();
  }
  

  missile_usb_destroy(control);  

  //---------------------------------------------------------------------------

  return 0;
}

//=============================================================================

/*
 * Local Variables:
 * mode: C
 * End:
 */
