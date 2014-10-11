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
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

#include "cutils/properties.h"

#include "../voldclient/voldclient.h"

#include "includes.h"
#include "external.h"

extern RecoveryUI* ui;

int COTBackup::MakeBackup(int system, int data, int cache, int boot, int recovery, Device* device) {
    fstab_rec* vol = NULL;
    char* path = NULL;

    if (system == 1) {
        path = (char*)malloc(1+strlen("system")+1);
        sprintf(path, "/%s", "system");
        vol = volume_for_path(path);
        String8 blkdevice(vol->blk_device);
        LOGI("Block device for /system is: %s\n", blkdevice.string());
    }
    if (data == 1) {
    }
    if (cache == 1) {
    }
    if (boot == 1) {
    }
    if (recovery == 1) {
    }
    return 0;
}

int COTBackup::RestoreBackup(String8 backup_path, Device* device) { return 0; }

void COTBackup::ShowBackupMenu(Device* device) {
    static const char* BackupNowHeaders[] = { "Backup Now",
        "",
        NULL
    };
    
    static const char* BackupNowItems[] = { "Yes - backup now",
        "No - don't backup",
        NULL
    };
    
#define YES_BACKUP 0
#define NO_BACKUP 1
    
    for (;;) {
        int BackupNowSelection = get_menu_selection(BackupNowHeaders, BackupNowItems, 0, 0, device);
        switch (BackupNowSelection) {
            case YES_BACKUP:
            {
                int result = MakeBackup(1, 1, 1, 1, 0, device);
                break;
            }
            case NO_BACKUP:
                return;
            case Device::kGoBack:
                return;
        }
    }
}

void COTBackup::ShowRestoreMenu(Device* device) { }

void COTBackup::ShowMainMenu(Device* device) {
    static const char* MainMenuHeaders[] = { "Backup and Restore",
        "",
        NULL
      };

    static const char* MainMenuItems[] = { "Backup Now",
        "Restore a backup",
        NULL
      };

#define BACKUP_NOW 0
#define RESTORE 1

    for (;;) {
        int MainMenuSelection = get_menu_selection(MainMenuHeaders, MainMenuItems, 0, 0, device);
        switch (MainMenuSelection) {
            case BACKUP_NOW:
                ShowBackupMenu(device);
                break;
            case RESTORE:
                ShowRestoreMenu(device);
                break;
            case Device::kGoBack:
                return;
        }
    }
}