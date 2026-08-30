#include "noftypes.h"
#include <eadk.h>
#undef false
#undef true
#undef bool
#include <osd.h>
#include <event.h>
#include <nesinput.h>

static int do_loadstate=0;

void osd_queue_loadstate() {
	do_loadstate=5;
}

void osd_getinput(void) {
  typedef struct {
    eadk_key_t key;
    int event;
  } key_event_mapping;
  const key_event_mapping key_to_events[] = {
    {eadk_key_left, event_joypad1_left},
    {eadk_key_up, event_joypad1_up},
    {eadk_key_down, event_joypad1_down},
    {eadk_key_right, event_joypad1_right},
    {eadk_key_ok, event_joypad1_b},
    {eadk_key_back, event_joypad1_a},
    {eadk_key_shift, event_joypad1_select},
    {eadk_key_backspace, event_joypad1_start},
    {eadk_event_tangent, event_hard_reset},
    {eadk_event_sqrt, event_state_save},
    {eadk_event_zero, event_state_save},
  };

  static bool exitNextIteration=false;
  static uint64_t old_keyboard_state = 0x0000000000000000;
  uint64_t current_keyboard_state = eadk_keyboard_scan();

	//do_loadstate is set to a certain number on bootup, because for some reason loading the state
	//doesn't work directly after boot-up. This causes it to wait for a few frames.
	if (do_loadstate>0) do_loadstate--;
	if (do_loadstate==1) {
		event_get(event_state_slot_0)(INP_STATE_MAKE);
		event_get(event_state_load)(INP_STATE_MAKE);
	}

  for (int i=0; i<sizeof(key_to_events)/sizeof(key_to_events[0]); i++) {
    bool wasUp = eadk_keyboard_key_down(old_keyboard_state, key_to_events[i].key);
    bool isUp = eadk_keyboard_key_down(current_keyboard_state, key_to_events[i].key);
    if (isUp != wasUp) {
      event_t evt = event_get(key_to_events[i].event);
      evt(isUp ? INP_STATE_MAKE : INP_STATE_BREAK);

      if (key_to_events[i].key == eadk_event_zero) {
        exitNextIteration = true;
      }
    }
  }

  if (exitNextIteration) {
    event_get(event_quit)(INP_STATE_MAKE);
  }

  old_keyboard_state = current_keyboard_state;
}
