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
#include <time.h>

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
#include "../flashutils/flashutils.h"

#include "cutils/properties.h"

#include "../voldclient/voldclient.h"

#include "includes.h"
#include "external.h"

extern RecoveryUI* ui;

char* COTBackup::GetAndroidVersion() {
    char* result;
    char* ANDROID_VERSION;
    ensure_path_mounted("/system");
    FILE * vers = fopen("/system/build.prop", "r");
    if (vers == NULL)
    {
        return NULL;
    }
    char line[512];
    while(fgets(line, sizeof(line), vers) != NULL) //read a line
    {
        if (strstr(line, "ro.build.display.id") != NULL)
        {
            char* strptr = strstr(line, "=") + 1;
            result = (char*)calloc(strlen(strptr) + 1, sizeof(char));
            strcpy(result, strptr);
            break; //leave the loop, we found what we're after
        }
    }
    fclose(vers);
    ensure_path_unmounted("/system");
    //strip only the android version from it
    LOGI("RAW VERSION: %s\n", result);
    int length = strlen(result);
    LOGI("LENGTH: %d\n", length);
    int i, k, found;
    for (i=0; i<length; i++) {
        k = i;
        //Android versions follow this scheme: AAADD, A=A-Z, D=0-9, ICS has an extra digit at the end
        if (isalpha(result[k]) && isalpha(result[k++]) && isalpha(result[k++]) && isdigit(result[k++]) && isdigit(result[k++])) {
            LOGI("Version number found at positions %d through %d:\n", i, k);
            found = 1;
            break;
        }
    }
    if (found) {
        ANDROID_VERSION = (char*)calloc(strlen(result) + 1, sizeof(char));
        int n = 0;
        for (i-=1; i<=k; i++) {
            ANDROID_VERSION[n] = result[i];
            n++;
        }
        ANDROID_VERSION[n] = '\0';
    } else {
        return NULL;
    }
    LOGI("Memory Address: %d\n", &ANDROID_VERSION);
    LOGI("ANDROID VERSION: %s\n", ANDROID_VERSION);
    return ANDROID_VERSION;
}

void COTBackup::GenerateBackupPath(char* backup_path) {
    char fmt[64], timestamp[64];
    struct timeval tv;
    struct tm *tm;
    gettimeofday(&tv, NULL);
    if ((tm = localtime(&tv.tv_sec)) != NULL) {
        LOGI("Generating timestamp...\n");
        strftime(fmt, sizeof fmt, "%Y-%m-%d_%H%M", tm);
        snprintf(timestamp, sizeof timestamp, fmt, tv.tv_usec);
    }
    char* ANDROID_VERSION = GetAndroidVersion();
    if (ANDROID_VERSION) {
        int vers_length = strlen(ANDROID_VERSION);
        int i;
        for (i=0; i<vers_length; i++) {
            if (ANDROID_VERSION[i] == '\n') ANDROID_VERSION[i] = NULL; // no newline please!
        }
    }
    sprintf(backup_path, "%s/0/cot/backup/%s-%s", get_primary_storage_path(), ANDROID_VERSION, timestamp);
}

int COTBackup::BackupPartition(const char* mPartition, const char* backup_path) {
    String8 backupString("Backing up ");
    backupString += mPartition;
    backupString += "...";
    ui->DialogShowInfo(backupString.string());
    int ret;
    fstab_rec* vol = NULL;
    char* path = NULL;
    path = (char*)malloc(1+strlen(mPartition)+1);
    sprintf(path, "/%s", mPartition);
    vol = volume_for_path(path);
    String8 blkdevice(vol->blk_device);
    LOGI("Block device for /%s is: %s\n", mPartition, blkdevice.string());
    String8 tmp(backup_path);
    COTStorage::EnsureDirectoryExists(tmp.string());
    tmp += "/";
    tmp += mPartition;
    tmp += ".img";
    LOGI("Backing up %s to %s\n", mPartition, tmp.string());
    if (0 != (ret = backup_raw_partition(vol->fs_type, vol->blk_device, tmp.string()))) {
        LOGE("Error while backing up %s image!\n", mPartition);
        ui->DialogDismiss();
        return -1;
    }
    ui->DialogDismiss();
    return 0;
}

int COTBackup::MakeBackup(int system, int data, int cache, int boot, int recovery, Device* device) {
    char backup_path[1024];
    GenerateBackupPath(backup_path);
    
    String8 bPath(backup_path);
    LOGI("Backup path: %s\n", bPath.string());
    
    if (boot == 1) {
        if (0 != BackupPartition("boot", backup_path)) { return -1; }
    }
    if (system == 1) {
        if (0 != BackupPartition("system", backup_path)) { return -1; }
    }
    if (data == 1) {
        ui->DialogShowInfo("Backing up data...");
        fstab_rec* vol = NULL;
        char* path = NULL;
        path = (char*)malloc(1+strlen("data")+1);
        sprintf(path, "/%s", "data");
        vol = volume_for_path(path);
        String8 blkdevice(vol->blk_device);
        LOGI("Block device for /data is: %s\n", blkdevice.string());
        ui->DialogDismiss();
    }
    if (cache == 1) {
        if (0 != BackupPartition("cache", backup_path)) { return -1; }
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
                MakeBackup(1, 1, 1, 1, 0, device);
                break;
            case NO_BACKUP:
                return;
            case Device::kGoBack:
                return;
        }
    }
}

void COTBackup::ShowRestoreMenu(Device* device) { return; }

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
#define RESTORE_BACKUP 1
    
    for (;;) {
        int MainMenuSelection = get_menu_selection(MainMenuHeaders, MainMenuItems, 0, 0, device);
        switch (MainMenuSelection) {
            case BACKUP_NOW:
                ShowBackupMenu(device);
                break;
            case RESTORE_BACKUP:
                ShowRestoreMenu(device);
                break;
            case Device::kGoBack:
                return;
        }
    }
}