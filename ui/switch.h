// =======================================================================
// switch.h
// =======================================================================
// Declaration of the switch utilities

#ifndef UI_SWITCH_H
#define UI_SWITCH_H

enum switch_state_t {
  NotFlipped,
  MaybeFlipped,
  Flipped,
  MaybeNotFlipped,
};

class Switch {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  Switch( int gpio_num );
  void update();
  bool is_flipped() const;
  bool just_flipped() const;
  bool just_unflipped() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 private:
  int gpio_num;  // GPIO pin number for the switch
  switch_state_t curr_state = NotFlipped;
  bool prev_physical = false;
  bool justFlipped = false;
  bool justUnflipped = false;
};

#endif  // UI_SWITCH_H