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
const bool COTTheme::use_theme = false;

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
				COTTheme::use_theme = false;
				break;
			case 1:
				COTTheme::use_theme = true;
				break;
		}
	}
}
