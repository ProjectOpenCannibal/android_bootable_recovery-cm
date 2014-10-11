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

// Keep the settings dictionary in memory so we can access it anywhere
dictionary * COTSettings::settingsini;

String8 COTSettings::zip_sigverif("1");

void COTSettings::CreateOrSaveSettings(int is_new) {
    // Make sure internal storage is mounted
    COTStorage::MountInternalStorage();

    if (is_new == 1) {
        FILE * ini;
        String8 base_path(get_primary_storage_path());
        base_path += "/0/cot/settings.ini";
        ini = fopen_path(base_path.string(), "w");
        fprintf(ini,
            "; COT Settings INI\n"
            ";\n"
            "\n"
            "[settings]\n"
            "theme = default\n"
            "zip_sigverif = 1\n"
            "\n");
        fclose(ini);
      } else {
          ui->DialogShowInfo("Saving settings...");
          FILE * ini;
          String8 base_path(get_primary_storage_path());
          base_path += "/0/cot/settings.ini";
          ini = fopen_path(base_path.string(), "w");
          iniparser_set(COTSettings::settingsini, "settings", NULL);
          iniparser_set(COTSettings::settingsini, "settings:theme", COTTheme::chosen_theme.string());
          iniparser_set(COTSettings::settingsini, "settings:zip_sigverif", COTSettings::zip_sigverif.string());
          iniparser_dump_ini(COTSettings::settingsini, ini);
          fclose(ini);
          sleep(2);
          ui->DialogDismiss();
        }
    return;
  }

void COTSettings::LoadSettings() {
    // Make sure internal storage is mounted
    COTStorage::MountInternalStorage();

    String8 base_path(get_primary_storage_path());
    base_path += "/0/cot/settings.ini";
    COTSettings::settingsini = iniparser_load(base_path.string());
    if (COTSettings::settingsini == NULL) {
        COTSettings::CreateOrSaveSettings(1);
        COTSettings::settingsini = iniparser_load(base_path.string());
      }
    COTSettings::zip_sigverif = iniparser_getstring(COTSettings::settingsini, "settings:zip_sigverif", NULL);
    COTTheme::LoadTheme(iniparser_getstring(COTSettings::settingsini, "settings:theme", NULL));

    ui->ResetIcons();
  }

void COTSettings::ShowMainMenu(Device* device) {
    static const char* SettingsMenuHeaders[] = { "Settings",
        "",
        NULL
      };

    static const char* SettingsMenuItems[] = { "Theme",
        "Zip Signature Verification",
        NULL
      };

#define THEME_OPTIONS 0
#define ZIP_VERIF_OPTIONS 1

    for (;;) {
        int SettingsSelection = get_menu_selection(SettingsMenuHeaders, SettingsMenuItems, 0, 0, device);
        switch (SettingsSelection) {
            case THEME_OPTIONS:
            //COTPackage::ShowZipOptionsMenu(device);
            COTTheme::ChooseThemeMenu(device);
                break;
            case ZIP_VERIF_OPTIONS:
            COTSettings::ShowZipVerifMenu(device);
                break;
            case Device::kGoBack:
            return;
          }
      }
  }

void COTSettings::ShowZipVerifMenu(Device* device) {
    static const char* ZipVerifMenuHeaders[] = { "Zip Verification",
        "",
        NULL
      };

    static const char* ZipVerifMenuItems[] = { "Enable Zip Verification",
        "Disable Zip Verification",
        NULL
      };

#define ZIP_VERIF_ON 0
#define ZIP_VERIF_OFF 1

    for (;;) {
        int ZipVerifSelection = get_menu_selection(ZipVerifMenuHeaders, ZipVerifMenuItems, 0, 0, device);
        switch (ZipVerifSelection) {
            case ZIP_VERIF_ON:
            COTSettings::zip_sigverif = "1";
                break;
            case ZIP_VERIF_OFF:
            COTSettings::zip_sigverif = "0";
                break;
            case Device::kGoBack:
            return;
          }
        COTSettings::CreateOrSaveSettings(0);
        return;
      }
  }
