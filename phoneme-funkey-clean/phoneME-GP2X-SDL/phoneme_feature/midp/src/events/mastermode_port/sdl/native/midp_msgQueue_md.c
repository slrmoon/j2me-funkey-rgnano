/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <midp_logging.h>
#include <midp_mastermode_port.h>
#include <keymap_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

static int GP2X_Default_keys[19];
static int KeyDebug = 0;
static int EventDebug = 0;
static int PhoneKeyProfile = 0;
static int JoyAxisKeys[8];
static int JoyHatKey = KEYMAP_KEY_INVALID;
static int KeyHoldCounts[384];
static int KeyboardPhysicalDown[512];
static int KeyboardPhysicalKey[512];
static int CurrentProfileSel = 0;
static long SignalCalls = 0;
static long SignalNoEvent = 0;
static long SignalDelivered = 0;
static long SignalWaitForever = 0;
static long SignalLastTrace = 0;
static const int OverlayBindMap[12] = {
  0, 4, 2, 6, 12, 13, 14, 15, 10, 11, 8, 9
};

typedef struct {
  const char *id;
  const char *label;
  int defaults[12];
} RuntimeKeyProfilePreset;

static const RuntimeKeyProfilePreset RuntimeKeyProfiles[] = {
  { "universal", "UNIVERSAL",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_GAMEB, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, KEYMAP_KEY_SOFT2, KEYMAP_KEY_SOFT1 } },
  { "doja", "DOJA/I-APPLI",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_SOFT1, KEYMAP_KEY_CLEAR, KEYMAP_KEY_SOFT2,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, KEYMAP_KEY_SOFT2, KEYMAP_KEY_SOFT1 } },
  { "nokia-s40", "NOKIA/S40",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_5, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, KEYMAP_KEY_SOFT2, KEYMAP_KEY_SOFT1 } },
  { "sony-ericsson", "SONY ERICSSON",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_5, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_CLEAR, KEYMAP_KEY_POUND, KEYMAP_KEY_SOFT1, KEYMAP_KEY_SOFT2 } },
  { "samsung", "SAMSUNG",
    { KEYMAP_KEY_2, KEYMAP_KEY_8, KEYMAP_KEY_4, KEYMAP_KEY_6,
      KEYMAP_KEY_5, KEYMAP_KEY_SELECT, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, KEYMAP_KEY_SOFT2, KEYMAP_KEY_SOFT1 } },
  { "lg", "LG",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_5, KEYMAP_KEY_CLEAR, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, KEYMAP_KEY_SOFT1, KEYMAP_KEY_SOFT2 } },
  { "motorola", "MOTOROLA",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_GAMEA, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_SEND, KEYMAP_KEY_END, KEYMAP_KEY_SOFT2, KEYMAP_KEY_SOFT1 } },
  { "soft-21-22", "SOFT 21/22",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_5, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, 22, 21 } },
  { "motorola-soft", "MOTO SOFT -21/-22",
    { KEYMAP_KEY_UP, KEYMAP_KEY_DOWN, KEYMAP_KEY_LEFT, KEYMAP_KEY_RIGHT,
      KEYMAP_KEY_SELECT, KEYMAP_KEY_GAMEA, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_SEND, KEYMAP_KEY_END, -22, -21 } },
  { "siemens-soft", "SIEMENS SOFT -1/-4",
    { KEYMAP_KEY_2, KEYMAP_KEY_8, KEYMAP_KEY_4, KEYMAP_KEY_6,
      KEYMAP_KEY_5, KEYMAP_KEY_SELECT, KEYMAP_KEY_GAMEC, KEYMAP_KEY_GAMED,
      KEYMAP_KEY_ASTERISK, KEYMAP_KEY_POUND, KEYMAP_KEY_RIGHT, KEYMAP_KEY_UP } }
};

#define RUNTIME_KEY_PROFILE_COUNT ((int)(sizeof(RuntimeKeyProfiles) / sizeof(RuntimeKeyProfiles[0])))

int PhoneMEOverlayHandleSDLEvent(SDL_Event *event);
int PhoneMEOverlayConsumeShutdown(void);
void PhoneMEOverlayRefresh(void);

int PhoneMEInputGetBindCount()
{
  return 12;
}

int PhoneMEInputGetBind(int index)
{
  if (index < 0 || index >= 12) return KEYMAP_KEY_INVALID;
  return GP2X_Default_keys[OverlayBindMap[index]];
}

void PhoneMEInputSetBind(int index, int key)
{
  if (index < 0 || index >= 12) return;
  GP2X_Default_keys[OverlayBindMap[index]] = key;
}

int PhoneMEInputGetProfileCount()
{
  return RUNTIME_KEY_PROFILE_COUNT;
}

const char *PhoneMEInputGetProfileId(int index)
{
  if (index < 0 || index >= RUNTIME_KEY_PROFILE_COUNT) return RuntimeKeyProfiles[0].id;
  return RuntimeKeyProfiles[index].id;
}

const char *PhoneMEInputGetProfileLabel(int index)
{
  if (index < 0 || index >= RUNTIME_KEY_PROFILE_COUNT) return RuntimeKeyProfiles[0].label;
  return RuntimeKeyProfiles[index].label;
}

int PhoneMEInputFindProfile(const char *id)
{
  int i;
  if (id == NULL) return 0;
  for (i = 0; i < RUNTIME_KEY_PROFILE_COUNT; i++) {
    if (strcmp(id, RuntimeKeyProfiles[i].id) == 0) return i;
  }
  return 0;
}

int PhoneMEInputGetProfileIndex()
{
  return CurrentProfileSel;
}

void PhoneMEInputApplyProfile(int index)
{
  int i;
  if (index < 0) index = RUNTIME_KEY_PROFILE_COUNT - 1;
  if (index >= RUNTIME_KEY_PROFILE_COUNT) index = 0;
  CurrentProfileSel = index;
  for (i = 0; i < 12; i++) {
    GP2X_Default_keys[OverlayBindMap[i]] = RuntimeKeyProfiles[index].defaults[i];
  }
}

static const char *KeyName(int Key)
{
  switch (Key) {
    case KEYMAP_KEY_UP: return "UP";
    case KEYMAP_KEY_DOWN: return "DOWN";
    case KEYMAP_KEY_LEFT: return "LEFT";
    case KEYMAP_KEY_RIGHT: return "RIGHT";
    case KEYMAP_KEY_SELECT: return "SELECT";
    case KEYMAP_KEY_SOFT1: return "SOFT1";
    case KEYMAP_KEY_SOFT2: return "SOFT2";
    case KEYMAP_KEY_CLEAR: return "CLEAR";
    case KEYMAP_KEY_SEND: return "SEND";
    case KEYMAP_KEY_END: return "END";
    case KEYMAP_KEY_POWER: return "POWER";
    case KEYMAP_KEY_GAMEA: return "GAMEA";
    case KEYMAP_KEY_GAMEB: return "GAMEB";
    case KEYMAP_KEY_GAMEC: return "GAMEC";
    case KEYMAP_KEY_GAMED: return "GAMED";
    case KEYMAP_KEY_SCREEN_ROT: return "SOFT2_MOTO/SCREEN_ROT";
    case -21: return "SOFT1_MOTO";
    case 21: return "SOFT1_21";
    case 22: return "SOFT2_22";
    case KEYMAP_KEY_ASTERISK: return "ASTERISK";
    case KEYMAP_KEY_POUND: return "POUND";
    case KEYMAP_KEY_SPACE: return "SPACE";
    case KEYMAP_KEY_INVALID: return "INVALID";
    default: break;
  }
  if ((Key >= KEYMAP_KEY_0) && (Key <= KEYMAP_KEY_9)) {
    static char Name[2];
    Name[0] = (char)Key;
    Name[1] = '\0';
    return Name;
  }
  return "OTHER";
}

static const char *ActionName(int Action)
{
  switch (Action) {
    case KEYMAP_STATE_PRESSED: return "pressed";
    case KEYMAP_STATE_RELEASED: return "released";
    default: break;
  }
  return "other";
}

static int EnvKey(const char *Name, int Default)
{
  char *Value = getenv(Name);
  return (Value != NULL) ? atoi(Value) : Default;
}

void InitGP2XKeys()
{ int i;
  const char *KeyProfile = getenv("PHONEME_KEY_PROFILE");
  KeyDebug = (getenv("PHONEME_KEY_DEBUG") != NULL);
  EventDebug = (getenv("PHONEME_EVENT_DEBUG") != NULL);
  PhoneKeyProfile = (KeyProfile != NULL) && (strcmp(KeyProfile, "phone") == 0);
  for (i = 0; i < 8; i++) {
    JoyAxisKeys[i] = KEYMAP_KEY_INVALID;
  }
  JoyHatKey = KEYMAP_KEY_INVALID;
  GP2X_Default_keys[0] = KEYMAP_KEY_UP;
  GP2X_Default_keys[1] = KEYMAP_KEY_1;
  GP2X_Default_keys[2] = KEYMAP_KEY_LEFT;
  GP2X_Default_keys[3] = KEYMAP_KEY_7;
  GP2X_Default_keys[4] = KEYMAP_KEY_DOWN;
  GP2X_Default_keys[5] = KEYMAP_KEY_9;
  GP2X_Default_keys[6] = KEYMAP_KEY_RIGHT;
  GP2X_Default_keys[7] = KEYMAP_KEY_3;
  GP2X_Default_keys[8] = KEYMAP_KEY_SOFT2;
  GP2X_Default_keys[9] = KEYMAP_KEY_SOFT1;
  GP2X_Default_keys[10] = KEYMAP_KEY_POUND;
  GP2X_Default_keys[11] = KEYMAP_KEY_ASTERISK;
  GP2X_Default_keys[12] = KEYMAP_KEY_GAMEA;
  GP2X_Default_keys[13] = KEYMAP_KEY_GAMEB;
  GP2X_Default_keys[14] = KEYMAP_KEY_GAMEC;
  GP2X_Default_keys[15] = KEYMAP_KEY_GAMED;
  GP2X_Default_keys[16] = KEYMAP_KEY_CLEAR;
  GP2X_Default_keys[17] = KEYMAP_KEY_SELECT;
  GP2X_Default_keys[18] = KEYMAP_KEY_5;
  GP2X_Default_keys[0] = EnvKey("J2ME_GP2X_JOYU", GP2X_Default_keys[0]);
  GP2X_Default_keys[1] = EnvKey("J2ME_GP2X_JOYUL", GP2X_Default_keys[1]);
  GP2X_Default_keys[2] = EnvKey("J2ME_GP2X_JOYL", GP2X_Default_keys[2]);
  GP2X_Default_keys[3] = EnvKey("J2ME_GP2X_JOYDL", GP2X_Default_keys[3]);
  GP2X_Default_keys[4] = EnvKey("J2ME_GP2X_JOYD", GP2X_Default_keys[4]);
  GP2X_Default_keys[5] = EnvKey("J2ME_GP2X_JOYDR", GP2X_Default_keys[5]);
  GP2X_Default_keys[6] = EnvKey("J2ME_GP2X_JOYR", GP2X_Default_keys[6]);
  GP2X_Default_keys[7] = EnvKey("J2ME_GP2X_JOYUR", GP2X_Default_keys[7]);
  GP2X_Default_keys[8] = EnvKey("J2ME_GP2X_START", GP2X_Default_keys[8]);
  GP2X_Default_keys[9] = EnvKey("J2ME_GP2X_SELECT", GP2X_Default_keys[9]);
  GP2X_Default_keys[10] = EnvKey("J2ME_GP2X_LEFT", GP2X_Default_keys[10]);
  GP2X_Default_keys[11] = EnvKey("J2ME_GP2X_RIGHT", GP2X_Default_keys[11]);
  GP2X_Default_keys[12] = EnvKey("J2ME_GP2X_BUTA", GP2X_Default_keys[12]);
  GP2X_Default_keys[13] = EnvKey("J2ME_GP2X_BUTB", GP2X_Default_keys[13]);
  GP2X_Default_keys[14] = EnvKey("J2ME_GP2X_BUTX", GP2X_Default_keys[14]);
  GP2X_Default_keys[15] = EnvKey("J2ME_GP2X_BUTY", GP2X_Default_keys[15]);
  GP2X_Default_keys[16] = EnvKey("J2ME_GP2X_VOLU", GP2X_Default_keys[16]);
  GP2X_Default_keys[17] = EnvKey("J2ME_GP2X_VOLD", GP2X_Default_keys[17]);
  GP2X_Default_keys[18] = EnvKey("J2ME_GP2X_JOYC", GP2X_Default_keys[18]);
  CurrentProfileSel = PhoneMEInputFindProfile(KeyProfile);
  for (i = 0; i < 384; i++) {
    KeyHoldCounts[i] = 0;
  }
  for (i = 0; i < 512; i++) {
    KeyboardPhysicalDown[i] = 0;
    KeyboardPhysicalKey[i] = KEYMAP_KEY_INVALID;
  }
  if (KeyDebug) {
    fprintf(stderr,
            "GP2X keymap: U=%s(%d) D=%s(%d) L=%s(%d) R=%s(%d) "
            "A=%s(%d) B=%s(%d) X=%s(%d) Y=%s(%d) "
            "L1=%s(%d) R1=%s(%d) START=%s(%d) SELECT=%s(%d) JOYC=%s(%d)\n",
            KeyName(GP2X_Default_keys[0]), GP2X_Default_keys[0],
            KeyName(GP2X_Default_keys[4]), GP2X_Default_keys[4],
            KeyName(GP2X_Default_keys[2]), GP2X_Default_keys[2],
            KeyName(GP2X_Default_keys[6]), GP2X_Default_keys[6],
            KeyName(GP2X_Default_keys[12]), GP2X_Default_keys[12],
            KeyName(GP2X_Default_keys[13]), GP2X_Default_keys[13],
            KeyName(GP2X_Default_keys[14]), GP2X_Default_keys[14],
            KeyName(GP2X_Default_keys[15]), GP2X_Default_keys[15],
            KeyName(GP2X_Default_keys[10]), GP2X_Default_keys[10],
            KeyName(GP2X_Default_keys[11]), GP2X_Default_keys[11],
            KeyName(GP2X_Default_keys[8]), GP2X_Default_keys[8],
            KeyName(GP2X_Default_keys[9]), GP2X_Default_keys[9],
            KeyName(GP2X_Default_keys[18]), GP2X_Default_keys[18]);
  }
}

static void SetKeyEvent(int Key, int Action, MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent)
{
  pNewSignal->waitingFor = UI_SIGNAL;
  pNewMidpEvent->type = MIDP_KEY_EVENT;
  pNewMidpEvent->CHR = Key;
  pNewMidpEvent->ACTION = Action;
  SignalDelivered++;
  if (EventDebug) {
    fprintf(stderr, "MIDP key event: %s(%d) %s(%d) delivered=%ld\n",
            KeyName(Key), Key, ActionName(Action), Action, SignalDelivered);
  }
}

static int HeldKeyIndex(int Key)
{
  if (Key >= 0 && Key < 256) {
    return Key;
  }
  if (Key < 0 && Key > -128) {
    return 256 + (-Key);
  }
  return -1;
}

static int SetPhysicalKeyEvent(int Physical, int Key, int Pressed,
                               MidpReentryData* pNewSignal,
                               MidpEvent* pNewMidpEvent)
{
  int KeyIndex;
  int StoredKey;

  if (Key == KEYMAP_KEY_INVALID) {
    return 0;
  }
  if (Physical < 0 || Physical >= 512) {
    SetKeyEvent(Key, Pressed ? KEYMAP_STATE_PRESSED : KEYMAP_STATE_RELEASED,
                pNewSignal, pNewMidpEvent);
    return 1;
  }

  if (Pressed) {
    if (KeyboardPhysicalDown[Physical]) {
      return 0;
    }
    KeyboardPhysicalDown[Physical] = 1;
    KeyboardPhysicalKey[Physical] = Key;
    KeyIndex = HeldKeyIndex(Key);
    if (KeyIndex >= 0) {
      if (KeyHoldCounts[KeyIndex]++ > 0) {
        return 0;
      }
    }
    SetKeyEvent(Key, KEYMAP_STATE_PRESSED, pNewSignal, pNewMidpEvent);
    return 1;
  }

  if (!KeyboardPhysicalDown[Physical]) {
    return 0;
  }
  StoredKey = KeyboardPhysicalKey[Physical];
  KeyboardPhysicalDown[Physical] = 0;
  KeyboardPhysicalKey[Physical] = KEYMAP_KEY_INVALID;
  KeyIndex = HeldKeyIndex(StoredKey);
  if (KeyIndex >= 0 && KeyHoldCounts[KeyIndex] > 0) {
    KeyHoldCounts[KeyIndex]--;
    if (KeyHoldCounts[KeyIndex] > 0) {
      return 0;
    }
  }
  SetKeyEvent(StoredKey, KEYMAP_STATE_RELEASED, pNewSignal, pNewMidpEvent);
  return 1;
}

static void TraceSignalLoop(jlong timeout, int delivered)
{ long Now = (long)JVM_JavaMilliSeconds();
  SignalCalls++;
  if (timeout == -1) {
    SignalWaitForever++;
  }
  if (!delivered) {
    SignalNoEvent++;
  }
  if (!EventDebug) {
    return;
  }
  if ((SignalLastTrace == 0) || ((Now - SignalLastTrace) >= 1000)) {
    fprintf(stderr,
            "midp_signal: timeout=%ld calls=%ld noevent=%ld delivered=%ld wait_forever=%ld\n",
            (long)timeout, SignalCalls, SignalNoEvent, SignalDelivered,
            SignalWaitForever);
    SignalLastTrace = Now;
  }
}

int KeyboardCheck(SDL_Event *event, MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent)
{ int Key = KEYMAP_KEY_INVALID;
  int Physical = event->key.keysym.sym;
  switch(event->key.keysym.sym)
        { case SDLK_TAB:
          case SDLK_LCTRL:  Key = KEYMAP_KEY_SOFT1; break;
          case SDLK_BACKSPACE:
          case SDLK_ESCAPE:
          case SDLK_RCTRL:  Key = KEYMAP_KEY_SOFT2; break;
          case SDLK_UP:     Key = KEYMAP_KEY_UP; break;
          case SDLK_DOWN:   Key = KEYMAP_KEY_DOWN; break;
          case SDLK_LEFT:   Key = KEYMAP_KEY_LEFT; break;
          case SDLK_RIGHT:  Key = KEYMAP_KEY_RIGHT; break;
          case SDLK_RETURN:
          case SDLK_SPACE:
          case SDLK_LALT:   Key = KEYMAP_KEY_SELECT; break;
          case SDLK_CLEAR:
          case SDLK_DELETE: Key = KEYMAP_KEY_CLEAR; break;
          case SDLK_0:      Key = KEYMAP_KEY_0; break;
          case SDLK_1:      Key = KEYMAP_KEY_1; break;
          case SDLK_2:      Key = KEYMAP_KEY_2; break;
          case SDLK_3:      Key = KEYMAP_KEY_3; break;
          case SDLK_4:      Key = KEYMAP_KEY_4; break;
          case SDLK_5:      Key = KEYMAP_KEY_5; break;
          case SDLK_6:      Key = KEYMAP_KEY_6; break;
          case SDLK_7:      Key = KEYMAP_KEY_7; break;
          case SDLK_8:      Key = KEYMAP_KEY_8; break;
          case SDLK_9:      Key = KEYMAP_KEY_9; break;
          case SDLK_KP0:    Key = KEYMAP_KEY_0; break;
          case SDLK_KP1:    Key = KEYMAP_KEY_7; break; // KEYPADNUM reverse KEYPHONE
          case SDLK_KP2:    Key = KEYMAP_KEY_8; break; // KEYPADNUM reverse KEYPHONE
          case SDLK_KP3:    Key = KEYMAP_KEY_9; break; // KEYPADNUM reverse KEYPHONE
          case SDLK_KP4:    Key = KEYMAP_KEY_4; break;
          case SDLK_KP5:    Key = KEYMAP_KEY_5; break;
          case SDLK_KP6:    Key = KEYMAP_KEY_6; break;
          case SDLK_KP7:    Key = KEYMAP_KEY_1; break; // KEYPADNUM reverse KEYPHONE
          case SDLK_KP8:    Key = KEYMAP_KEY_2; break; // KEYPADNUM reverse KEYPHONE
          case SDLK_KP9:    Key = KEYMAP_KEY_3; break; // KEYPADNUM reverse KEYPHONE
          case SDLK_KP_MULTIPLY: Key = KEYMAP_KEY_ASTERISK; break;
          case SDLK_KP_PLUS:     Key = KEYMAP_KEY_POUND; break;
          case SDLK_ASTERISK:    Key = KEYMAP_KEY_ASTERISK; break;
          case SDLK_HASH:        Key = KEYMAP_KEY_POUND; break;
          case SDLK_HOME:   Key = KEYMAP_KEY_SEND; break;
          case SDLK_END:    Key = KEYMAP_KEY_END; break;
          case SDLK_PAGEUP: Key = KEYMAP_KEY_GAMEC; break;
          case SDLK_PAGEDOWN: Key = KEYMAP_KEY_GAMED; break;
          case SDLK_F1:     Key = KEYMAP_KEY_SOFT1; break;
          case SDLK_F2:     Key = KEYMAP_KEY_SOFT2; break;
          case SDLK_F3:     Key = KEYMAP_KEY_SCREEN_ROT; break;
          case SDLK_F9:     Key = KEYMAP_KEY_GAMEA; break;
          case SDLK_F10:    Key = KEYMAP_KEY_GAMEB; break;
          case SDLK_F11:    Key = KEYMAP_KEY_GAMEC; break;
          case SDLK_F12:    Key = KEYMAP_KEY_GAMED; break;
          /* Confirmed FunKey S / RG Nano baseline mapping from KeyDebugMidlet:
           * dpad -> directional game actions, A -> Select, side keys ->
           * softkeys, dedicated center button -> 5, lower aux -> star/pound.
           */
          case SDLK_u:      Key = GP2X_Default_keys[0]; break;
          case SDLK_r:      Key = GP2X_Default_keys[6]; break;
          case SDLK_d:      Key = GP2X_Default_keys[4]; break;
          case SDLK_l:      Key = GP2X_Default_keys[2]; break;
          case SDLK_k:      Key = GP2X_Default_keys[9]; break;
          case SDLK_s:      Key = GP2X_Default_keys[8]; break;
          case SDLK_m:      Key = GP2X_Default_keys[10]; break;
          case SDLK_n:      Key = GP2X_Default_keys[11]; break;
          case SDLK_a:      Key = GP2X_Default_keys[12]; break;
          case SDLK_b:      Key = GP2X_Default_keys[13]; break;
          case SDLK_x:      Key = GP2X_Default_keys[14]; break;
          case SDLK_y:      Key = GP2X_Default_keys[15]; break;
          default: Key = KEYMAP_KEY_INVALID;
        }
  if (KeyDebug) {
    fprintf(stderr, "SDL key %s sym=%d scancode=%d -> %s(%d)\n",
            (event->key.state == SDL_PRESSED) ? "down" : "up",
            event->key.keysym.sym, event->key.keysym.scancode,
            KeyName(Key), Key);
  }
  if (Key == KEYMAP_KEY_INVALID) {
    return 0;
  }
  return SetPhysicalKeyEvent(Physical, Key, event->key.state == SDL_PRESSED,
                             pNewSignal, pNewMidpEvent);
}

int JoystickButtonCheck(SDL_Event *event, MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent)
{ int Key = event->jbutton.button;
  if ((Key>=0) && (Key<=18)) Key = GP2X_Default_keys[Key];
  else Key = KEYMAP_KEY_INVALID;
  if (KeyDebug) {
    fprintf(stderr, "SDL joy button=%d state=%d -> %s(%d)\n",
            event->jbutton.button, event->jbutton.state, KeyName(Key), Key);
  }
  if (Key == KEYMAP_KEY_INVALID) {
    return 0;
  }
  SetKeyEvent(Key,
              (event->jbutton.state == SDL_PRESSED) ? KEYMAP_STATE_PRESSED : KEYMAP_STATE_RELEASED,
              pNewSignal, pNewMidpEvent);
  return 1;
}

int JoystickAxisCheck(SDL_Event *event, MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent)
{ int Axis = event->jaxis.axis;
  int Value = event->jaxis.value;
  int OldKey;
  int NewKey = KEYMAP_KEY_INVALID;
  const int DeadZone = 16000;

  if ((Axis < 0) || (Axis >= 8)) {
    if (KeyDebug) {
      fprintf(stderr, "SDL joy axis=%d value=%d ignored\n", Axis, Value);
    }
    return 0;
  }

  if (Axis == 0) {
    if (Value < -DeadZone) NewKey = KEYMAP_KEY_LEFT;
    else if (Value > DeadZone) NewKey = KEYMAP_KEY_RIGHT;
  } else if (Axis == 1) {
    if (Value < -DeadZone) NewKey = KEYMAP_KEY_UP;
    else if (Value > DeadZone) NewKey = KEYMAP_KEY_DOWN;
  } else if (Axis == 2) {
    if (Value < -DeadZone) NewKey = KEYMAP_KEY_SOFT1;
    else if (Value > DeadZone) NewKey = KEYMAP_KEY_SOFT2;
  } else if (Axis == 3) {
    if (Value < -DeadZone) NewKey = KEYMAP_KEY_ASTERISK;
    else if (Value > DeadZone) NewKey = KEYMAP_KEY_POUND;
  }

  OldKey = JoyAxisKeys[Axis];
  if (NewKey == OldKey) {
    return 0;
  }
  JoyAxisKeys[Axis] = NewKey;

  if (KeyDebug) {
    fprintf(stderr, "SDL joy axis=%d value=%d old=%s(%d) new=%s(%d)\n",
            Axis, Value, KeyName(OldKey), OldKey, KeyName(NewKey), NewKey);
  }

  if (OldKey != KEYMAP_KEY_INVALID) {
    SetKeyEvent(OldKey, KEYMAP_STATE_RELEASED, pNewSignal, pNewMidpEvent);
    return 1;
  }
  if (NewKey != KEYMAP_KEY_INVALID) {
    SetKeyEvent(NewKey, KEYMAP_STATE_PRESSED, pNewSignal, pNewMidpEvent);
    return 1;
  }
  return 0;
}

int JoystickHatCheck(SDL_Event *event, MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent)
{ int Value = event->jhat.value;
  int OldKey = JoyHatKey;
  int NewKey = KEYMAP_KEY_INVALID;

  if (Value & SDL_HAT_UP) NewKey = KEYMAP_KEY_UP;
  else if (Value & SDL_HAT_DOWN) NewKey = KEYMAP_KEY_DOWN;
  else if (Value & SDL_HAT_LEFT) NewKey = KEYMAP_KEY_LEFT;
  else if (Value & SDL_HAT_RIGHT) NewKey = KEYMAP_KEY_RIGHT;

  if (NewKey == OldKey) {
    return 0;
  }
  JoyHatKey = NewKey;

  if (KeyDebug) {
    fprintf(stderr, "SDL joy hat=%d value=%d old=%s(%d) new=%s(%d)\n",
            event->jhat.hat, Value, KeyName(OldKey), OldKey,
            KeyName(NewKey), NewKey);
  }

  if (OldKey != KEYMAP_KEY_INVALID) {
    SetKeyEvent(OldKey, KEYMAP_STATE_RELEASED, pNewSignal, pNewMidpEvent);
    return 1;
  }
  if (NewKey != KEYMAP_KEY_INVALID) {
    SetKeyEvent(NewKey, KEYMAP_STATE_PRESSED, pNewSignal, pNewMidpEvent);
    return 1;
  }
  return 0;
}

static int RequestGracefulShutdown(MidpReentryData* pNewSignal,
                                   MidpEvent* pNewMidpEvent)
{
  pNewSignal->waitingFor = AMS_SIGNAL;
  pNewMidpEvent->type = SHUTDOWN_EVENT;
  if (EventDebug) {
    fprintf(stderr, "MIDP shutdown requested for graceful save\n");
  }
  return 1;
}

int CheckEvent(SDL_Event *event, MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent)
{ if (event->type == SDL_QUIT)
     { return RequestGracefulShutdown(pNewSignal, pNewMidpEvent);
     }
  if (PhoneMEOverlayHandleSDLEvent(event)) {
    PhoneMEOverlayRefresh();
    if (PhoneMEOverlayConsumeShutdown()) {
      return RequestGracefulShutdown(pNewSignal, pNewMidpEvent);
    }
    return 0;
  }
  if ((event->type == SDL_KEYDOWN) || (event->type == SDL_KEYUP))
     { return KeyboardCheck(event, pNewSignal, pNewMidpEvent);
     }
  if ((event->type == SDL_JOYBUTTONDOWN) || (event->type == SDL_JOYBUTTONUP))
     { return JoystickButtonCheck(event, pNewSignal, pNewMidpEvent);
     }
  if (event->type == SDL_JOYAXISMOTION)
     { return JoystickAxisCheck(event, pNewSignal, pNewMidpEvent);
     }
  if (event->type == SDL_JOYHATMOTION)
     { return JoystickHatCheck(event, pNewSignal, pNewMidpEvent);
     }
  if (KeyDebug) {
    fprintf(stderr, "SDL event type=%d ignored\n", event->type);
  }
  return 0;
}

/*
 * This function is called by the VM periodically. It has to check if
 * system has sent a signal to MIDP and return the result in the
 * structs given.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until a signal sent to MIDP, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the system for a signal but do not block. Return to the
 *       caller immediately regardless of the if a signal was sent.
 *  -1 = Do not timeout. Block until a signal is sent to MIDP.
 */
void checkForSystemSignal(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent, jlong timeout) 
{ SDL_Event event;
  jlong currentTime = JVM_JavaMilliSeconds(), stopTime;
  if (timeout == -1)
     { while (SDL_WaitEvent(&event))
          { if (CheckEvent(&event, pNewSignal, pNewMidpEvent))
               { TraceSignalLoop(timeout, 1);
                 return;
               }
          }
       TraceSignalLoop(timeout, 0);
       return;
     }
  do { if (SDL_PollEvent(&event))
          { if (CheckEvent(&event, pNewSignal, pNewMidpEvent))
               { TraceSignalLoop(timeout, 1);
                 return;
               }
          }
       stopTime = JVM_JavaMilliSeconds();
     } while((stopTime - currentTime) < timeout);
  TraceSignalLoop(timeout, 0);
}
