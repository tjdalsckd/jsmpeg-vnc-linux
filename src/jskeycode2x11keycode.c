/* Transforms JavaScript KeyEvent.keyCodes to X11 keycodes

https://guacamole.incubator.apache.org/doc/0.9.5/guacamole-common-js/symbols/src/src_main_webapp_modules_Keyboard.js.html
http://www.cl.cam.ac.uk/~mgk25/ucs/keysym2ucs.c
https://api.kde.org/4.12-api/kdenetwork-apidocs/krfb/html/keysym_8h_source.html
https://rffr.de/wp-content/uploads/manuell/de-dvorak-keysym.xmodmap */

#include <X11/keysym.h>
#include "jskeycode2x11keycode.h"

KeyCode js_keycode_to_x11keycode(Display *display, unsigned short keycode) {
  /* a-z and 0-9 are the same */
  KeySym keysym;

  switch (keycode) {
    case (8): keysym = XK_BackSpace; break;
    case (186): keysym = XK_semicolon; break;
    case (189): keysym = XK_underscore; break;
    case (187): keysym = XK_equal; break;
    case (36) : keysym = XK_Home; break;
     case (35) : keysym = XK_End; break;
     case (46) : keysym = XK_Delete; break;
case (105) :keysym = XK_9;break;
case (104) :keysym = XK_8;break;
case (103) :keysym = XK_7;break;
case (102) :keysym = XK_6;break;
case (101) :keysym = XK_5;break;
case (100) :keysym = XK_4;break;
case (99) :keysym = XK_3;break;
case (98) :keysym = XK_2;break;
case (97) :keysym = XK_1;break;
case (96) :keysym = XK_0;break;
case (111) :keysym = XK_KP_Divide;break;
case (191) :keysym = XK_slash;break;
case (106) :keysym = XK_KP_Multiply;break;
case (109) :keysym = XK_KP_Subtract;break;
case (107) :keysym = XK_KP_Add;break;
case (110) :keysym = XK_period;break;
case (188) :keysym = XK_comma;break;
case (220) :keysym = XK_backslash;break;
case (222) :keysym = XK_apostrophe;break;
case (192) :keysym = XK_quoteleft;break;
case (45) :keysym = XK_Insert;break;
case (33) :keysym = XK_Page_Up;break;
case (34) :keysym = XK_Page_Down;break;

case (112) :keysym = XK_F1;break;
case (113) :keysym = XK_F2;break;
case (114) :keysym = XK_F3;break;
case (115) :keysym = XK_F4;break;
case (116) :keysym = XK_F5;break;
case (117) :keysym = XK_F6;break;
case (118) :keysym = XK_F7;break;
case (119) :keysym = XK_F8;break;
case (120) :keysym = XK_F9;break;
case (121) :keysym = XK_F10;break;
case (122) :keysym = XK_F11;break;
case (123) :keysym = XK_F12;break;

    case (9): keysym = XK_Tab; break;
    case (13): keysym = XK_Return; break;
    case (16): keysym = XK_Shift_L; break;
    case (17): keysym = XK_Control_L; break;
    case (18): keysym = XK_Alt_L; break;
    case (21): keysym = XK_Alt_R; break;    
    case (20): keysym = XK_Caps_Lock; break;
    case (27): keysym = XK_Escape; break;
case (25): keysym = XK_Control_R; break;
    
    case (37): keysym = XK_Left; break;
    case (38): keysym = XK_Up; break;
    case (39): keysym = XK_Right; break;
    case (40): keysym = XK_Down; break;
    case (190): keysym = XK_period;break;
    default: keysym = keycode; break;

  }

  return XKeysymToKeycode(display, keysym);
}
