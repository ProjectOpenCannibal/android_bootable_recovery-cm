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
extern ScreenRecoveryUI* screen;

String8 COTTheme::chosen_theme("default");
bool COTTheme::use_theme = false;
int COTTheme::C_HEADER[4] = { 111, 111, 111, 255 };
int COTTheme::C_TOP[4] = { 208, 208, 208, 255};
int COTTheme::C_MENU_SEL_FG[4] = { 25, 160, 210, 255 };
int COTTheme::C_MENU_SEL_BG[4] = { 60, 60, 61, 255 };
int COTTheme::C_LOG[4] = { 76, 76, 76, 255 };
int COTTheme::C_TEXT_FILL[4] = { 0, 0, 0, 255 };
int COTTheme::C_ERROR_TEXT[4] = { 255, 0, 0, 255 };
int COTTheme::C_DEFAULT[4] = { 255, 255, 255, 255 };

void COTTheme::LoadTheme(Device* device, char * themename) {
	LOGE("Loading theme %s...\n", themename);
	ensure_path_mounted("/data/media");
	ensure_path_mounted("/sdcard");
	dictionary * ini;
	if (strcmp(themename, "default")) {
		char * theme_base = "/sdcard/themes/";
		char * theme_end = "/theme.ini";
		char * full_theme_file = (char *)malloc(strlen(theme_base) + strlen(theme_end) + strlen(themename));
		strcpy(full_theme_file, "/sdcard/themes/");
		strcat(full_theme_file, themename);
		strcat(full_theme_file, "/theme.ini");
		ini = iniparser_load(full_theme_file);
		if (ini == NULL) {
			LOGE("Can't load theme %s from %s!\n", themename, full_theme_file);
			return;
		}
		LOGE("Theme %s loaded from %s!\n", themename, full_theme_file);
		COTTheme::use_theme = true;
		COTTheme::chosen_theme = themename;
	} else {
		char * ini_file = "/res/images/default_theme.ini";
		ini = iniparser_load(ini_file);
		if (ini == NULL) {
			LOGE("Can't load theme %s!\n", themename);
			return;
		}
		COTTheme::use_theme = false;
		COTTheme::chosen_theme = "default";
	}
	COTTheme::C_HEADER[0] = iniparser_getint(ini, "theme:header_r", NULL);
	COTTheme::C_HEADER[1] = iniparser_getint(ini, "theme:header_g", NULL);
	COTTheme::C_HEADER[2] = iniparser_getint(ini, "theme:header_b", NULL);
	COTTheme::C_HEADER[3] = iniparser_getint(ini, "theme:header_a", NULL);
		
	COTTheme::C_TOP[0] = iniparser_getint(ini, "theme:top_r", NULL);
	COTTheme::C_TOP[1] = iniparser_getint(ini, "theme:top_g", NULL);
	COTTheme::C_TOP[2] = iniparser_getint(ini, "theme:top_b", NULL);
	COTTheme::C_TOP[3] = iniparser_getint(ini, "theme:top_a", NULL);
		
	COTTheme::C_MENU_SEL_FG[0] = iniparser_getint(ini, "theme:menufg_r", NULL);
	COTTheme::C_MENU_SEL_FG[1] = iniparser_getint(ini, "theme:menufg_g", NULL);
	COTTheme::C_MENU_SEL_FG[2] = iniparser_getint(ini, "theme:menufg_b", NULL);
	COTTheme::C_MENU_SEL_FG[3] = iniparser_getint(ini, "theme:menufg_a", NULL);
		
	COTTheme::C_MENU_SEL_BG[0] = iniparser_getint(ini, "theme:menubg_r", NULL);
	COTTheme::C_MENU_SEL_BG[1] = iniparser_getint(ini, "theme:menubg_g", NULL);
	COTTheme::C_MENU_SEL_BG[2] = iniparser_getint(ini, "theme:menubg_b", NULL);
	COTTheme::C_MENU_SEL_BG[3] = iniparser_getint(ini, "theme:menubg_a", NULL);
		
	COTTheme::C_LOG[0] = iniparser_getint(ini, "theme:log_r", NULL);
	COTTheme::C_LOG[1] = iniparser_getint(ini, "theme:log_g", NULL);
	COTTheme::C_LOG[2] = iniparser_getint(ini, "theme:log_b", NULL);
	COTTheme::C_LOG[3] = iniparser_getint(ini, "theme:log_a", NULL);
		
	COTTheme::C_TEXT_FILL[0] = iniparser_getint(ini, "theme:textfill_r", NULL);
	COTTheme::C_TEXT_FILL[1] = iniparser_getint(ini, "theme:textfill_g", NULL);
	COTTheme::C_TEXT_FILL[2] = iniparser_getint(ini, "theme:textfill_b", NULL);
	COTTheme::C_TEXT_FILL[3] = iniparser_getint(ini, "theme:textfill_a", NULL);
		
	COTTheme::C_ERROR_TEXT[0] = iniparser_getint(ini, "theme:errortext_r", NULL);
	COTTheme::C_ERROR_TEXT[1] = iniparser_getint(ini, "theme:errortext_g", NULL);
	COTTheme::C_ERROR_TEXT[2] = iniparser_getint(ini, "theme:errortext_b", NULL);
	COTTheme::C_ERROR_TEXT[3] = iniparser_getint(ini, "theme:errortext_a", NULL);
	
	COTTheme::C_DEFAULT[0] = iniparser_getint(ini, "theme:default_r", NULL);
	COTTheme::C_DEFAULT[1] = iniparser_getint(ini, "theme:default_g", NULL);
	COTTheme::C_DEFAULT[2] = iniparser_getint(ini, "theme:default_b", NULL);
	COTTheme::C_DEFAULT[3] = iniparser_getint(ini, "theme:default_a", NULL);
}

int COTTheme::compare_string(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void COTTheme::ChooseThemeMenu(Device* device) {
	ensure_path_mounted("/data/media");
	ensure_path_mounted("/sdcard");
	static const char* headers[] = { "Choose Theme",
									"",
									NULL
	};
    DIR* d;
    struct dirent* de;
    d = opendir("/sdcard/themes");
    if (d == NULL) {
        LOGE("error opening /sdcard/themes: %s\n", strerror(errno));
        return;
    }

    int d_size = 0;
    int d_alloc = 10;
    char** dirs = (char**)malloc(d_alloc * sizeof(char*));
    int z_size = 1;
    int z_alloc = 10;
    char** zips = (char**)malloc(z_alloc * sizeof(char*));
    zips[0] = strdup("default");

    while ((de = readdir(d)) != NULL) {
        int name_len = strlen(de->d_name);

        if (de->d_type == DT_DIR) {
            // skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' &&
                de->d_name[1] == '.') continue;

            if (d_size >= d_alloc) {
                d_alloc *= 2;
                dirs = (char**)realloc(dirs, d_alloc * sizeof(char*));
            }
            dirs[d_size] = (char*)malloc(name_len + 2);
            strcpy(dirs[d_size], de->d_name);
            dirs[d_size][name_len] = '\0';
            ++d_size;
        }
    }
    closedir(d);

    qsort(dirs, d_size, sizeof(char*), compare_string);
    qsort(zips, z_size, sizeof(char*), compare_string);
    
    // append dirs to the zips list
    if (d_size + z_size + 1 > z_alloc) {
        z_alloc = d_size + z_size + 1;
        zips = (char**)realloc(zips, z_alloc * sizeof(char*));
    }
    memcpy(zips + z_size, dirs, d_size * sizeof(char*));
    free(dirs);
    z_size += d_size;
    zips[z_size] = NULL;
    
    int result;
    int chosen_item = 0;
	
	for (;;) {
		chosen_item = get_menu_selection(headers, zips, 1, chosen_item, device);
		if (chosen_item == Device::kGoBack) {
			return;
		}
		
		char* item = zips[chosen_item];
		int item_len = strlen(item);
		
		char new_path[PATH_MAX];
		strlcpy(new_path, item, PATH_MAX);
		
		LOGE("Chose %s ...\n", item);
		COTTheme::chosen_theme = item;
		COTTheme::use_theme = true;
		COTTheme::LoadTheme(device, item);
		int i;
		for (i = 0; i < z_size; ++i) free(zips[i]);
		free(zips);
		free(headers);
		ui->ResetIcons();
		return;
	}
}
