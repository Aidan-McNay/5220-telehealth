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
  void update();
  bool is_pressed() const;
  bool just_pressed() const;
  bool just_released() const;
  bool is_released() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // int            gpio_num;
  // button_state_t curr_state;
 private:
  int gpio_num;  // GPIO pin number for the button
  button_state_t curr_state = NotPressed;
  bool prev_physical = false;
  bool justPressed = false;
  bool justReleased = false;
};

#endif  // UI_BUTTON_H