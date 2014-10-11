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

// Slightly modified from CWM and previous COT versions
int COTPackage::VerifyRootAndRecovery(Device* device) {		
    if (ensure_path_mounted("/system") != 0)
        return 0;
    
    int ret = 0;
    struct stat st;
    
    static const char* VerifyRootHeaders[] = { "Verify Root and Recovery",
        "",
        NULL
    };
    
    #define FIX_ROOT_OR_RECOVERY 0
    
    // check to see if install-recovery.sh is going to clobber recovery
    // install-recovery.sh is also used to run the su daemon on stock rom for 4.3+
    // so verify that doesn't exist...
    if (0 != lstat("/system/etc/.installed_su_daemon", &st)) {
        // check install-recovery.sh exists and is executable
        if (0 == lstat("/system/etc/install-recovery.sh", &st)) {
            if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                ret = 1;
                
                /* I'm not sure how best to notify the user of what's going on
                 * here without a super long option, hopefully this will make
                 * sense to users. */
                static const char* VerifyRootItems[] = { "Disable stock recovery flash and reboot",
                    "Reboot without disabling",
                    NULL
                };
                
                for (;;) {
                    int VerifyRootSelection = get_menu_selection(VerifyRootHeaders, VerifyRootItems, 0, 0, device);
                    switch (VerifyRootSelection) {
                        case FIX_ROOT_OR_RECOVERY:
                            __system("chmod -x /system/etc/install-recovery.sh");
                            break;
                        default: break;
                    }
                }
            }
        }
    }
    
    int exists = 0;
    
    if (0 == lstat("/system/bin/su", &st)) {
        exists = 1;
        if (S_ISREG(st.st_mode)) {
            if ((st.st_mode & (S_ISUID | S_ISGID)) != (S_ISUID | S_ISGID)) {
                ret = 1;
                
                /* I'm not sure how best to notify the user of what's going on
                 * here without a super long option, hopefully this will make
                 * sense to users. */
                static const char* VerifyRootItems[] = { "Fix root access before rebooting",
                    "Reboot without fixing root",
                    NULL
                };
                
                for (;;) {
                    int VerifyRootSelection = get_menu_selection(VerifyRootHeaders, VerifyRootItems, 0, 0, device);
                    switch (VerifyRootSelection) {
                        case FIX_ROOT_OR_RECOVERY:
                            __system("chmod 6755 /system/bin/su");
                            break;
                        default: break;
                    }
                }
            }
        }
    }
    
    if (!exists) {
        ret = 1;
        
        /* I'm not sure how best to notify the user of what's going on
         * here without a super long option, hopefully this will make
         * sense to users. */
        static const char* VerifyRootItems[] = { "Install root before rebooting",
            "Reboot without installing root",
            NULL
        };
        
        for (;;) {
            int VerifyRootSelection = get_menu_selection(VerifyRootHeaders, VerifyRootItems, 0, 0, device);
            switch (VerifyRootSelection) {
                case FIX_ROOT_OR_RECOVERY:
                    __system("/sbin/install-su.sh");
                    break;
                default: break;
            }
        }
    }
    
    ensure_path_unmounted("/system");
    return ret;
}
