// =======================================================================
// button.h
// =======================================================================
// Declaration of the button utilities

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

enum button_state_t {
  NotPressed,
  MaybePressed,
  Pressed,
  MaybeNotPressed,
};

class Button {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  Button( int gpio_num );
  bool is_pressed();
  bool just_pressed();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  int            gpio_num;
  button_state_t curr_state;
};

#endif  // UI_BUTTON_H