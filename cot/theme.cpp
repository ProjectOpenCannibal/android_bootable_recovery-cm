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
String8 COTTheme::BatteryIndicator("false");
String8 COTTheme::BatteryLevel("bat_100");
int COTTheme::battery_x = 0;
int COTTheme::battery_y = 0;
int COTTheme::center_text = 0;

char* COTTheme::GetThemeName(const char* themepath) {
    dictionary * ini;
    String8 theme_file(get_primary_storage_path());
    theme_file += "/0/cot/themes/";
    theme_file += themepath;
    theme_file += "/theme.ini";
    ini = iniparser_load(theme_file.string());
    if (ini == NULL) {
        return NULL;
    }
    return iniparser_getstring(ini, "theme:name", NULL);
}

void COTTheme::LoadTheme(char * themename) {
    LOGI("Loading theme %s...\n", themename);
    
    // Make sure internal storage is mounted
    COTStorage::MountInternalStorage();
    
    dictionary * ini;
    if (strcmp(themename, "default")) {
        
        String8 theme_file(get_primary_storage_path());
        theme_file += "/0/cot/themes/";
        theme_file += themename;
        theme_file += "/theme.ini";
        ini = iniparser_load(theme_file.string());
        if (ini == NULL) {
            LOGI("Can't load theme %s from %s, switching to default!\n", themename, theme_file.string());
            COTTheme::LoadTheme("default");
            return;
        }
        LOGI("Theme %s loaded from %s!\n", themename, theme_file.string());
        COTTheme::use_theme = true;
        COTTheme::chosen_theme = themename;
    } else {
        char * ini_file = "/res/images/default_theme.ini";
        ini = iniparser_load(ini_file);
        if (ini == NULL) {
            LOGI("Can't load theme %s!\n", themename);
            return;
        }
        COTTheme::use_theme = false;
        COTTheme::chosen_theme = "default";
    }
    COTTheme::C_HEADER[0] = iniparser_getint(ini, "theme:header_r", 111);
    COTTheme::C_HEADER[1] = iniparser_getint(ini, "theme:header_g", 111);
    COTTheme::C_HEADER[2] = iniparser_getint(ini, "theme:header_b", 111);
    COTTheme::C_HEADER[3] = iniparser_getint(ini, "theme:header_a", 255);
    
    COTTheme::C_TOP[0] = iniparser_getint(ini, "theme:top_r", 208);
    COTTheme::C_TOP[1] = iniparser_getint(ini, "theme:top_g", 208);
    COTTheme::C_TOP[2] = iniparser_getint(ini, "theme:top_b", 208);
    COTTheme::C_TOP[3] = iniparser_getint(ini, "theme:top_a", 255);
    
    COTTheme::C_MENU_SEL_FG[0] = iniparser_getint(ini, "theme:menufg_r", 25);
    COTTheme::C_MENU_SEL_FG[1] = iniparser_getint(ini, "theme:menufg_g", 160);
    COTTheme::C_MENU_SEL_FG[2] = iniparser_getint(ini, "theme:menufg_b", 210);
    COTTheme::C_MENU_SEL_FG[3] = iniparser_getint(ini, "theme:menufg_a", 255);
    
    COTTheme::C_MENU_SEL_BG[0] = iniparser_getint(ini, "theme:menubg_r", 60);
    COTTheme::C_MENU_SEL_BG[1] = iniparser_getint(ini, "theme:menubg_g", 60);
    COTTheme::C_MENU_SEL_BG[2] = iniparser_getint(ini, "theme:menubg_b", 61);
    COTTheme::C_MENU_SEL_BG[3] = iniparser_getint(ini, "theme:menubg_a", 255);
    
    COTTheme::C_LOG[0] = iniparser_getint(ini, "theme:log_r", 76);
    COTTheme::C_LOG[1] = iniparser_getint(ini, "theme:log_g", 76);
    COTTheme::C_LOG[2] = iniparser_getint(ini, "theme:log_b", 76);
    COTTheme::C_LOG[3] = iniparser_getint(ini, "theme:log_a", 255);
    
    COTTheme::C_TEXT_FILL[0] = iniparser_getint(ini, "theme:textfill_r", 0);
    COTTheme::C_TEXT_FILL[1] = iniparser_getint(ini, "theme:textfill_g", 0);
    COTTheme::C_TEXT_FILL[2] = iniparser_getint(ini, "theme:textfill_b", 0);
    COTTheme::C_TEXT_FILL[3] = iniparser_getint(ini, "theme:textfill_a", 255);
    
    COTTheme::C_ERROR_TEXT[0] = iniparser_getint(ini, "theme:errortext_r", 255);
    COTTheme::C_ERROR_TEXT[1] = iniparser_getint(ini, "theme:errortext_g", 0);
    COTTheme::C_ERROR_TEXT[2] = iniparser_getint(ini, "theme:errortext_b", 0);
    COTTheme::C_ERROR_TEXT[3] = iniparser_getint(ini, "theme:errortext_a", 255);
    
    COTTheme::C_DEFAULT[0] = iniparser_getint(ini, "theme:default_r", 255);
    COTTheme::C_DEFAULT[1] = iniparser_getint(ini, "theme:default_g", 255);
    COTTheme::C_DEFAULT[2] = iniparser_getint(ini, "theme:default_b", 255);
    COTTheme::C_DEFAULT[3] = iniparser_getint(ini, "theme:default_a", 255);
    
    BatteryIndicator = iniparser_getstring(ini, "theme:batteryindicator", "false");
    battery_x = iniparser_getint(ini, "theme:battery_x", 0);
    battery_y = iniparser_getint(ini, "theme:battery_y", 0);

    center_text = iniparser_getint(ini, "theme:center_text", 0);
}

int COTTheme::compare_string(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}



void COTTheme::ShowThemeChooser(Device* device) {
    
    // Make sure internal storage is mounted
    COTStorage::MountInternalStorage();
    
    static const char* headers[] = { "Choose Theme",
        "",
        NULL
    };
    DIR* d;
    struct dirent* de;
    String8 base_path(get_primary_storage_path());
    base_path += "/0/cot/themes";
    d = opendir(base_path.string());
    if (d == NULL) {
        LOGE("error opening %s: %s\n", base_path.string(), strerror(errno));
        return;
    }

    int b_size = 0;
    int b_alloc = 10;
    char** base = (char**)malloc(b_alloc * sizeof(char*));
    int th_size = 0;
    int th_alloc = 10;
    char** themes = (char**)malloc(th_alloc * sizeof(char*));

    int d_size = 0;
    int d_alloc = 10;
    char** dirs = (char**)malloc(d_alloc * sizeof(char*));
    int z_size = 0;
    int z_alloc = 10;
    char** zips = (char**)malloc(z_alloc * sizeof(char*));

    
    while ((de = readdir(d)) != NULL) {
        int name_len = strlen(de->d_name);
        
        if (de->d_type == DT_DIR) {
            // skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' &&
                de->d_name[1] == '.') continue;

            int theme_len = strlen(GetThemeName(de->d_name));
            
            if (d_size >= d_alloc) {
                d_alloc *= 2;
                dirs = (char**)realloc(dirs, d_alloc * sizeof(char*));
            }

            if (b_size >= b_alloc) {
                b_alloc *= 2;
                base = (char**)realloc(base, b_alloc * sizeof(char*));
            }

            dirs[d_size] = (char*)malloc(name_len + 2);
            strcpy(dirs[d_size], de->d_name);
            dirs[d_size][name_len] = '\0';
            ++d_size;

            base[b_size] = (char*)malloc(theme_len + 2);
            strcpy(base[b_size], GetThemeName(de->d_name));
            base[b_size][theme_len] = '\0';
            ++b_size;
        }
    }
    closedir(d);
    
    // append dirs to the zips list
    if (d_size + z_size + 1 > z_alloc) {
        z_alloc = d_size + z_size + 1;
        zips = (char**)realloc(zips, z_alloc * sizeof(char*));
    }

    // append themes
    if (b_size + th_size + 1 > th_alloc) {
        th_alloc = b_size + th_size + 1;
        themes = (char**)realloc(themes, th_alloc * sizeof(char*));
    }

    memcpy(zips + z_size, dirs, d_size * sizeof(char*));
    memcpy(themes + th_size, themes, b_size * sizeof(char*));

    free(dirs);
    free(themes);

    z_size += d_size;
    th_size = b_size;

    zips[z_size] = NULL;
    themes[th_size] = NULL;
    
    int result;
    int chosen_item = 0;
    
    for (;;) {
        chosen_item = get_menu_selection(headers, themes, 1, chosen_item, device);
        if (chosen_item == Device::kGoBack) {
            return;
        }
        
        char* item = zips[chosen_item];
        int item_len = strlen(item);
        
        char new_path[PATH_MAX];
        strlcpy(new_path, item, PATH_MAX);
        
        LOGI("Chose %s ...\n", item);
        int i;
        COTTheme::chosen_theme = item;
        COTTheme::use_theme = true;
        COTTheme::LoadTheme(item);
        
        COTSettings::CreateOrSaveSettings(0);
        
        ui->ResetIcons();

        for (i = 0; i < z_size; ++i) free(zips[i]);
        for (i = 0; i < th_size; ++i) free(themes[i]);

        free(zips);
        free(themes);

        return;
    }
}

void COTTheme::ChooseThemeMenu(Device *device) {
    static const char* ThemeMenuHeaders[] = { "Choose Theme",
            "",
            NULL
    };

    static const char* ThemeMenuItems[] = { "Default Theme",
            "Theme Chooser",
            NULL
    };

#define DEFAULT_THEME 0
#define THEME_CHOOSER 1

    for (;;) {
        int ThemeSelection = get_menu_selection(ThemeMenuHeaders, ThemeMenuItems, 0, 0, device);
        switch (ThemeSelection) {
            case DEFAULT_THEME:
            {
                COTTheme::chosen_theme = "default";
                COTTheme::use_theme = true;
                COTTheme::LoadTheme("default");
                COTSettings::CreateOrSaveSettings(0);
                ui->ResetIcons();
                break;
            }
            case THEME_CHOOSER:
                COTTheme::ShowThemeChooser(device);
                break;
            case Device::kGoBack:
                return;
        }
    }
}
