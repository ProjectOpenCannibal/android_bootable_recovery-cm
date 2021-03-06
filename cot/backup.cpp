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
#include <sys/types.h>
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

#define YES 0
#define NO 1

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
    String8 mBackupDir("/");
    mBackupDir += get_primary_storage_path();
    mBackupDir += "/0/cot/backup";
    COTStorage::EnsureDirectoryExists(mBackupDir.string());
    sprintf(backup_path, "%s/0/cot/backup/%s-%s", get_primary_storage_path(), ANDROID_VERSION, timestamp);
}

int COTBackup::MakeBackup(int system, int data, int cache, int boot, int recovery, Device* device) {
    char backup_path[1024];
    GenerateBackupPath(backup_path);
    
    String8 bPath(backup_path);
    
    LOGI("Backup path: %s\n", bPath.string());
    bPath += ".ab";
    
    char tmp[1024];
    int fd = open(bPath.string(), O_CREAT|O_WRONLY|O_EXCL, 0777);
    LOGI("Got fd: %d\n", fd);
    if (fd == -1) {
        LOGE("Failed to open file: %s\n", strerror(errno));
    }else{
        fd = open(bPath.string(), O_WRONLY);
        sprintf(tmp, "bu %d backup boot system data cache", fd);
        __system(tmp);
    }
    close(fd);
    return 0;
}

int COTBackup::RestoreBackup(String8 backup_path, Device* device) {
    char tmp[1024];
    int fd = open(backup_path.string(), O_RDWR);
    LOGI("Got restore fd: %d\n", fd);
    if (fd == -1) {
        LOGE("Failed to open file: %s\n", strerror(errno));
    }else{
        sprintf(tmp, "bu %d restore", fd);
        __system(tmp);
    }
    close(fd);
    return 0;
}

int COTBackup::DeleteBackup(String8 backup_path, Device* device) {
    char tmp[1024];
    sprintf(tmp, "rm %s", backup_path.string());
    __system(tmp);
    return 0;
}

void COTBackup::ShowBackupMenu(Device* device) {
    static const char* BackupNowHeaders[] = { "Backup Now",
            "",
            NULL
    };

    static const char* BackupNowItems[] = { "Yes - backup now",
            "No - don't backup",
            NULL
    };

    for (;;) {
        int BackupNowSelection = get_menu_selection(BackupNowHeaders, BackupNowItems, 0, 0, device);
        switch (BackupNowSelection) {
            case YES:
                MakeBackup(1, 1, 1, 1, 0, device);
                return;
            case NO:
                return;
            case Device::kGoBack:
                return;
        }
    }
}

void COTBackup::ShowRestoreMenu(Device* device) {
    String8 mRestorePath(get_primary_storage_path());
    mRestorePath += "/0/cot/backup/";
    char *cRestoreBackupFile = COTStorage::ChooseFileMenu(mRestorePath.string(), ".ab", NULL, device);

    if (cRestoreBackupFile == NULL)
        return;

    static const char* RestoreNowHeaders[] = { "Restore Now",
        "",
        NULL
    };
    
    static const char* RestoreNowItems[] = { "Yes - restore now",
        "No - don't restore",
        NULL
    };
    
    for (;;) {
        int RestoreNowSelection = get_menu_selection(RestoreNowHeaders, RestoreNowItems, 0, 0, device);
        switch (RestoreNowSelection) {
            case YES:
            {
                String8 mRestoreBackupFile(cRestoreBackupFile);
                LOGI("File to restore: %s\n", mRestoreBackupFile.string());
                RestoreBackup(mRestoreBackupFile, device);
                return;
            }
            case NO:
                return;
            case Device::kGoBack:
                return;
        }
    }
}

void COTBackup::ShowDeleteMenu(Device* device) {
    String8 mDeletePath(get_primary_storage_path());
    mDeletePath += "/0/cot/backup/";
    char* file = COTStorage::ChooseFileMenu(mDeletePath.string(), ".ab", NULL, device);
    
    if (file == NULL)
        return;

    static const char* DeleteNowHeaders[] = { "Delete Now",
            "",
            NULL
    };

    static const char* DeleteNowItems[] = { "Yes - delete backup",
            "No - don't delete",
            NULL
    };

    for (;;) {
        int RestoreNowSelection = get_menu_selection(DeleteNowHeaders, DeleteNowItems, 0, 0, device);
        switch (RestoreNowSelection) {
            case YES:
            {
                String8 mDeleteBackupFile(file);
                LOGI("File to delete: %s\n", mDeleteBackupFile.string());
                ui->DialogShowInfo("Deleting backup...");
                int ret = DeleteBackup(mDeleteBackupFile, device);
                sleep(1);
                ui->DialogDismiss();
                return;
            }
            case NO:
                return;
            case Device::kGoBack:
                return;
        }
    }
}

void COTBackup::ShowMainMenu(Device* device) {
    static const char* MainMenuHeaders[] = { "Backup and Restore",
        "",
        NULL
    };
    
    static const char* MainMenuItems[] = { "Backup Now",
        "Restore a backup",
        "Delete backup files",
        NULL
    };
    
#define BACKUP_NOW 0
#define RESTORE_BACKUP 1
#define DELETE_BACKUP 2
    
    for (;;) {
        int MainMenuSelection = get_menu_selection(MainMenuHeaders, MainMenuItems, 0, 0, device);
        switch (MainMenuSelection) {
            case BACKUP_NOW:
                ShowBackupMenu(device);
                break;
            case RESTORE_BACKUP:
                ShowRestoreMenu(device);
                break;
            case DELETE_BACKUP:
                ShowDeleteMenu(device);
                break;
            case Device::kGoBack:
                return;
        }
    }
}