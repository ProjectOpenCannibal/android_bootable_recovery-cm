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
#include <dirent.h>

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

void COTStorage::MountInternalStorage() {
    ensure_path_mounted("/data/media");
    ensure_path_mounted("/sdcard");
    ensure_path_mounted("/storage/sdcard0");
}

void COTStorage::UnmountInternalStorage() {
    ensure_path_unmounted("/sdcard");
    ensure_path_unmounted("/storage/sdcard0");
    ensure_path_unmounted("/data/media");
}

void COTStorage::EnsureDirectoryExists(const char* dir) {
    char tmp[1024];
    sprintf(tmp, "mkdir -p %s ; chmod 777 %s", dir, dir);
    __system(tmp);
}

char** COTStorage::gather_files(const char* directory, const char* fileExtensionOrDirectory, int* numFiles)
{
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files = NULL;
    int pass;
    *numFiles = 0;
    int dirLen = strlen(directory);

    dir = opendir(directory);
    if (dir == NULL) {
        ui_print("Couldn't open directory.\n");
        return NULL;
    }

    int extension_length = 0;
    if (fileExtensionOrDirectory != NULL)
        extension_length = strlen(fileExtensionOrDirectory);

    int isCounting = 1;
    i = 0;
    for (pass = 0; pass < 2; pass++) {
        while ((de=readdir(dir)) != NULL) {
            // skip hidden files
            if (de->d_name[0] == '.')
                continue;

            // NULL means that we are gathering directories, so skip this
            if (fileExtensionOrDirectory != NULL)
            {
                // make sure that we can have the desired extension (prevent seg fault)
                if (strlen(de->d_name) < extension_length)
                    continue;
                // compare the extension
                if (strcmp(de->d_name + strlen(de->d_name) - extension_length, fileExtensionOrDirectory) != 0)
                    continue;
            }
            else
            {
                struct stat info;
                char fullFileName[PATH_MAX];
                strcpy(fullFileName, directory);
                strcat(fullFileName, de->d_name);
                stat(fullFileName, &info);
                // make sure it is a directory
                if (!(S_ISDIR(info.st_mode)))
                    continue;
            }

            if (pass == 0)
            {
                total++;
                continue;
            }

            files[i] = (char*) malloc(dirLen + strlen(de->d_name) + 2);
            strcpy(files[i], directory);
            strcat(files[i], de->d_name);
            if (fileExtensionOrDirectory == NULL)
                strcat(files[i], "/");
            i++;
        }
        if (pass == 1)
            break;
        if (total == 0)
            break;
        rewinddir(dir);
        *numFiles = total;
        files = (char**) malloc((total+1)*sizeof(char*));
        files[total]=NULL;
    }

    if(closedir(dir) < 0) {
        LOGE("Failed to close directory.");
    }

    if (total==0) {
        return NULL;
    }

    // sort the result
    if (files != NULL) {
        for (i = 0; i < total; i++) {
            int curMax = -1;
            int j;
            for (j = 0; j < total - i; j++) {
                if (curMax == -1 || strcmp(files[curMax], files[j]) < 0)
                    curMax = j;
            }
            char* temp = files[curMax];
            files[curMax] = files[total - i - 1];
            files[total - i - 1] = temp;
        }
    }

    return files;
}

void COTStorage::FreeStringArray(char** array)
{
    if (array == NULL)
        return;
    char* cursor = array[0];
    int i = 0;
    while (cursor != NULL)
    {
        free(cursor);
        cursor = array[++i];
    }
    free(array);
}


// pass in NULL for fileExtensionOrDirectory and you will get a directory chooser
char* COTStorage::ChooseFileMenu(const char* directory, const char* fileExtensionOrDirectory, const char* headers[], Device* device) {
    char path[] = "";
    DIR *dir;
    struct dirent de;
    int numFiles = 0;
    int numDirs = 0;
    int i;
    char* return_value = NULL;
    int dir_len = strlen(directory);
    
    static const char* fixed_headers[] = { "Choose Backup to restore",
        "",
        NULL
    };
    
    char** files = gather_files(directory, fileExtensionOrDirectory, &numFiles);
    char** dirs = NULL;
    if (fileExtensionOrDirectory != NULL)
        dirs = gather_files(directory, NULL, &numDirs);
    int total = numDirs + numFiles;
    if (total == 0)
    {
        LOGI("No files found.\n");
    }
    else
    {
        char** list = (char**) malloc((total + 1) * sizeof(char*));
        list[total] = NULL;
        
        
        for (i = 0 ; i < numDirs; i++)
        {
            list[i] = strdup(dirs[i] + dir_len);
        }
        
        for (i = 0 ; i < numFiles; i++)
        {
            list[numDirs + i] = strdup(files[i] + dir_len);
        }
        
        for (;;)
        {
            int chosen_item = get_menu_selection(fixed_headers, list, 0, 0, device);
            if (chosen_item == Device::kGoBack)
                break;
            static char ret[PATH_MAX];
            if (chosen_item < numDirs)
            {
                char* subret = ChooseFileMenu(dirs[chosen_item], fileExtensionOrDirectory, headers, device);
                if (subret != NULL)
                {
                    strcpy(ret, subret);
                    return_value = ret;
                    break;
                }
                continue;
            }
            strcpy(ret, files[chosen_item - numDirs]);
            return_value = ret;
            break;
        }
        FreeStringArray(list);
    }
    
    FreeStringArray(files);
    FreeStringArray(dirs);
    //free(fixed_headers);
    return return_value;
}
