/*  Copyright 2016 ALPS ELECTRIC CO., LTD.
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/


#ifndef BLUETOOTH_API_H
#define BLUETOOTH_API_H


#define LEFT_SHIFT(shift)    (0x00000001ul << (shift) )

#define BD_NONE 0x00000000
#define BD_PATH LEFT_SHIFT(0)
#define BD_ADDER LEFT_SHIFT(1)
#define BD_NAME LEFT_SHIFT(2)
#define BD_ALIAS LEFT_SHIFT(3)
#define BD_PAIRED LEFT_SHIFT(4)
#define BD_TRUSTED LEFT_SHIFT(5)
#define BD_BLOCKED LEFT_SHIFT(6)
#define BD_ACLCONNECTED LEFT_SHIFT(7)
#define BD_AVCONNECTED LEFT_SHIFT(8)
#define BD_HFPCONNECTED LEFT_SHIFT(9)
#define BD_LEGACYPAIRING LEFT_SHIFT(10)
#define BD_RSSI LEFT_SHIFT(11)
#define BD_AVRCP_TITLE LEFT_SHIFT(12)
#define BD_AVRCP_ARTIST LEFT_SHIFT(13)
#define BD_AVRCP_STATUS LEFT_SHIFT(14)
#define BD_AVRCP_DURATION LEFT_SHIFT(15)
#define BD_AVRCP_POSITION LEFT_SHIFT(16)
#define BD_TRANSPORT_STATE LEFT_SHIFT(17)
#define BD_TRANSPORT_VOLUME LEFT_SHIFT(18)
#define BD_UUID_PROFILES LEFT_SHIFT(19)


/* -------------- PLUGIN DEFINITIONS ----------------- */

typedef struct {
  void *bt_server;          /* handle to implementation  */
  unsigned int index;       /* currently selected media file       */
} BtCtxHandleT;


#endif /* BLUETOOTH_API_H */



/****************************** The End Of File ******************************/


