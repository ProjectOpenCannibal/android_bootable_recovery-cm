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

class ORS {
public:
  static const char *SCRIPT_FILE_CACHE = "/cache/recovery/openrecoveryscript";
  static const char *SCRIPT_FILE_TMP = "/tmp/openrecoveryscript";
  static void delayed_reboot();
  static int check_for_script_file(void);
  static int run_ors_script_file(void)
};
