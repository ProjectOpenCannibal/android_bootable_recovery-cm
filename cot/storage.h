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

#include "iniparser/dictionary.h"
#include "iniparser/iniparser.h"

class COTStorage {
public:
    static void MountInternalStorage();
    static void UnmountInternalStorage();
    static void EnsureDirectoryExists(const char* dir);
    static char* ChooseFileMenu(const char* directory, const char* fileExtensionOrDirectory, const char* headers[], Device* device);
private:
    static void FreeStringArray(char** array);
    static char** gather_files(const char* directory, const char* fileExtensionOrDirectory, int* numFiles);
};
