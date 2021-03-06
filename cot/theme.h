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

class COTTheme {
public:
    static String8 chosen_theme;
    static const char* theme_path;
    static bool use_theme;
    static int C_HEADER[4];
    static int C_TOP[4];
    static int C_MENU_SEL_FG[4];
    static int C_MENU_SEL_BG[4];
    static int C_LOG[4];
    static int C_TEXT_FILL[4];
    static int C_ERROR_TEXT[4];
    static int C_DEFAULT[4];
    static int battery_x;
    static int battery_y;
    static String8 BatteryIndicator;
    static String8 BatteryLevel;
    static int center_text;
    static char* GetThemeName(const char * themepath);
    static void LoadTheme(char * themename);
    static int compare_string(const void* a, const void* b);
    static void ChooseThemeMenu(Device* device);
    static void ShowThemeChooser(Device* device);
};
