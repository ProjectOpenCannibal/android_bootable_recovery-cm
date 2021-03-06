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

class COTBackup {
public:
    static int MakeBackup(int system, int data, int cache, int boot, int recovery, Device* device);
    static int RestoreBackup(String8 backup_path, Device* device);
    static int DeleteBackup(String8 backup_path, Device* device);

    static void ShowBackupMenu(Device* device);
    static void ShowRestoreMenu(Device* device);
    static void ShowDeleteMenu(Device* device);
    
    static void ShowMainMenu(Device* device);
    
private:
    static void GenerateBackupPath(char* backup_path);
    static char* GetAndroidVersion();
};
