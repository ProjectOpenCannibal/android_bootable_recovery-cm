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
extern ScreenRecoveryUI* screen;

const char* COTTheme::theme_path = "custom";
bool COTTheme::use_theme = false;
int COTTheme::C_HEADER[4] = { 111, 111, 111, 255 };
int COTTheme::C_TOP[4] = { 208, 208, 208, 255};
int COTTheme::C_MENU_SEL_FG[4] = { 25, 160, 210, 255 };
int COTTheme::C_MENU_SEL_BG[4] = { 60, 60, 61, 255 };
int COTTheme::C_LOG[4] = { 76, 76, 76, 255 };
int COTTheme::C_TEXT_FILL[4] = { 0, 0, 0, 255 };
int COTTheme::C_ERROR_TEXT[4] = { 255, 0, 0, 255 };
int COTTheme::C_DEFAULT[4] = { 255, 255, 255, 255 };

void COTTheme::LoadTheme(Device* device, const char* themename) {
	ensure_path_mounted("/data/media");
	ensure_path_mounted("/sdcard");
	dictionary * ini;
	if (strcmp(themename, "default")) {
		COTTheme::use_theme = true;
		char * ini_file = "/sdcard/themes/custom/theme.ini";
		ini = iniparser_load(ini_file);
	} else {
		COTTheme::use_theme = false;
		char * ini_file = "/res/images/default_theme.ini";
		ini = iniparser_load(ini_file);
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

void COTTheme::ChooseThemeMenu(Device* device) {
	static const char* headers[] = { "Choose Theme",
		"",
		NULL
	};
	
	static const char* menuitems[] = { "Use default theme",
		"Use custom theme",
		NULL
	};
	
	for (;;) {
		int result = get_menu_selection(headers, menuitems, 0, 0, device);
		switch (result) {
			case 0:
				COTTheme::LoadTheme(device, "default");
				ui->ResetIcons(0);
				return;
			case 1:
				COTTheme::LoadTheme(device, "custom");
				ui->ResetIcons(1);
				return;
			case Device::kGoBack:
                return;
		}
	}
}
