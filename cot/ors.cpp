/* Copyright (C) 2014 Project Open Cannibal
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../bootloader.h"
#include "../common.h"
#include "cutils/properties.h"
#include "cutils/android_reboot.h"
#include "../install.h"
#include "../minui/minui.h"
#include "../minzip/DirUtil.h"
#include "../roots.h"
#include "../ui.h"
#include "../screen_ui.h"
#include "../device.h"
#include "../voldclient/voldclient.h"

#include "includes.h"
#include "external.h"

#define SCRIPT_COMMAND_SIZE 512

extern RecoveryUI* ui;

static const char *SCRIPT_FILE_CACHE = "/cache/recovery/openrecoveryscript";
static const char *SCRIPT_FILE_TMP = "/tmp/openrecoveryscript";

void ORS::delayed_reboot() {
  int i;
  for (i = 3; i > 0; i--) {
    ui->Print("Rebooting the system in (%d)\n", i);
    sleep(1);
  }
  android_reboot(ANDROID_RB_RESTART, 0, 0);
}

int ORS::check_for_script_file(void) {
  FILE *fp = fopen(SCRIPT_FILE_CACHE, "r");
  int ret_val = 0;
  char exec[512];
  
  if (fp != NULL) {
    ret_val = 1;
    LOGI("Script file found: '%s'\n", SCRIPT_FILE_CACHE);
    fclose(fp);
    // Copy script file to /tmp
    strcpy(exec, "cp ");
    strcat(exec, SCRIPT_FILE_CACHE);
    strcat(exec, " ");
    strcat(exec, SCRIPT_FILE_TMP);
    __system(exec);
    // Delete the file from /cache
    strcpy(exec, "rm ");
    strcat(exec, SCRIPT_FILE_CACHE);
    __system(exec);
  }
  return ret_val;
}

int ORS::run_ors_script_file(void) {
  FILE *fp = fopen(SCRIPT_FILE_TMP, "r");
  struct stat st;
  int ret_val = 0, cindex, line_len, i, remove_nl;
  char script_line[SCRIPT_COMMAND_SIZE], command[SCRIPT_COMMAND_SIZE],
  value[SCRIPT_COMMAND_SIZE], mount[SCRIPT_COMMAND_SIZE],
  value1[SCRIPT_COMMAND_SIZE], value2[SCRIPT_COMMAND_SIZE];
  char *val_start, *tok;
  int ors_system = 0;
  int ors_data = 0;
  int ors_cache = 0;
  int ors_recovery = 0;
  int ors_boot = 0;
  int ors_andsec = 0;
  int ors_sdext = 0;
  int ors_no_confirm = 0;
  
  if (fp != NULL) {
    for (i = 20; i > 0; i--) {
      ui->Print("Waiting for storage to mount (%ds)\n", i);
      if (ensure_path_mounted("/sdcard") ==0) {
	ui->Print("Storage Mounted...\nContinuing...\n");
	break;
      }
      sleep(1);
    }
    while (fgets(script_line, SCRIPT_COMMAND_SIZE, fp) != NULL && ret_val == 0) {
      cindex = 0;
      line_len = strlen(script_line);
      for (i=0; i<line_len; i++) {
	if ((int)script_line[i] == 32) {
	  cindex = i;
	  i = line_len;
	}
      }
      memset(command, 0, sizeof(command));
      memset(value, 0, sizeof(value));
      if ((int)script_line[line_len - 1] == 10)
	remove_nl = 2;
      else
	remove_nl = 1;
      if (cindex != 0) {
	strncpy(command, script_line, cindex);
	LOGI("command is: '%s' and ", command);
	val_start = script_line;
	val_start += cindex + 1;
	strncpy(value, val_start, line_len - cindex - remove_nl);
	LOGI("value is: '%s'\n", value);
      } else {
	strncpy(command, script_line, line_len - remove_nl + 1);
	ui->Print("command is: '%s' and there is no value\n", command);
      }
      if (strcmp(command, "install") == 0) {
	// Install zip -- ToDo : Need to clean this shit up, it's redundant and I know it can be written better
	ensure_path_mounted("/sdcard");
	ui->Print("Installing zip file '%s'\n", value);
	//ret_val = install_zip(value);
	if (ret_val != INSTALL_SUCCESS) {
	  LOGE("Error installing zip file '%s'\n", value);
	  ret_val = 1;
	}
      } else if (strcmp(command, "wipe") == 0) {
	// Wipe -- ToDo: Make this use the same wipe functionality as normal wipes
	if (strcmp(value, "cache") == 0 || strcmp(value, "/cache") == 0) {
	  erase_volume("cache");
	} else if (strcmp(value, "dalvik") == 0 || strcmp(value, "dalvick") == 0 || strcmp(value, "dalvikcache") == 0 || strcmp(value, "dalvickcache") == 0) {
	  ui->Print("Erasing dalvik-cache not available at this time!\n");
	} else if (strcmp(value, "data") == 0 || strcmp(value, "/data") == 0 || strcmp(value, "factory") == 0 || strcmp(value, "factoryreset") == 0) {
	  ui->Print("-- Wiping Data Partition...\n");
	  erase_volume("/data");
	  ui->Print("-- Data Partition Wipe Complete!\n");
	} else {
	  LOGE("Error with wipe command value: '%s'\n", value);
	  ret_val = 1;
	}
      } else if (strcmp(command, "backup") == 0) {
	ui->Print("Nandroid is not available at this time!");
	ret_val = 1;
      } else if (strcmp(command, "restore") == 0) {
	// Restore
	ui->Print("Nandroid is not available at this time!");
	ret_val = 1;
      } else if (strcmp(command, "mount") == 0) {
	// Mount
	if (value[0] != '/') {
	  strcpy(mount, "/");
	  strcat(mount, value);
	} else {
	  strcpy(mount, value);
	}	
	ensure_path_mounted(mount);
	ui->Print("Mounted '%s'\n", mount);
      } else if (strcmp(command, "unmount") == 0 || strcmp(command, "umount") == 0) {
	// Unmount
	if (value[0] != '/') {
	  strcpy(mount, "/");
	  strcat(mount, value);
	} else {
	  strcpy(mount, value);
	}
	ensure_path_unmounted(mount);
	ui->Print("Unmounted '%s'\n", mount);
      } else if (strcmp(command, "mkdir") == 0) {
	// Make directory (recursive)
	ensure_directory_exists(value); // Untested from ORS
      } else if (strcmp(command, "reboot") == 0) {
	// Reboot
	ui->Print("Reboot command found...\n");
	fclose(fp);
	if(is_path_mounted("sdcard/")) {
	  ensure_path_unmounted("sdcard/");
	}
	delayed_reboot();
      } else if (strcmp(command, "cmd") == 0) {
	if (cindex != 0) {
	  __system(value);
	} else {
	  LOGE("No value given for cmd\n");
	}
      } else if (strcmp(command, "print") == 0) {
	ui->Print(value);
      } else {
	LOGE("Unrecognized script command: '%s'\n", command);
	ret_val = 1;
      }
    }
    fclose(fp);
    ui->Print("Done processing script file\n");
    if(ret_val != 1) {
      if(is_path_mounted("sdcard/"))
	ensure_path_unmounted("sdcard/");
      delayed_reboot();
    }
  } else {
    LOGE("Error opening script file '%s'\n");
    return 1;
  }
  return ret_val;
}
