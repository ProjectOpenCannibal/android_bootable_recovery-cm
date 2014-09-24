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
extern static int signature_verification_enabled;

int COTPackage::ShowSigVerifMenu(Device* device) {
	static const char* headers[] = { "Signature Verification",
		"",
		NULL
	};
	
	static const char* menuitems[] = {"Disable signature verification - resets on reboot",
		NULL
	};
	
	for (;;) {
		int selected = get_menu_selection(headers, menuitems, 0, 0, device);
		return selected;
	}
}

void COTPackage::ShowZipOptionsMenu(Device* device) {
	static const char* headers[] = { "ZIP Options",
		"",
		NULL
	};
	
	static const char* menuitems[] = {"Signature Verification",
		NULL
	};
	
	for (;;) {
		int selected = get_menu_selection(headers, menuitems, 0, 0, device);
		if (selected == 0) {
			COTPackage::ShowSigVerifMenu(device);
		}
		return;
	}
}

int COTPackage::InstallUntrustedZip(Device* device) {
	static const char* headers[] = { "Testing",
		"",
		NULL
	};
	
	static const char* menuitems[] = {"Yes - install untrusted ZIP",
		"No - abort ZIP installation",
		NULL
	};
	
	for (;;) {
		int selected = get_menu_selection(headers, menuitems, 0, 0, device);
		return selected;
	}
}
