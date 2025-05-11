// =======================================================================
// state_machine.cpp
// =======================================================================

#include "pico/stdlib.h"
#include "ui/state_machine.h"
#include "utils/debug.h"
#include <stdio.h>

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

FSM::FSM( int button_gpio, int status_led_gpio, int error_led_gpio,
          int power_led_gpio )
    : button( button_gpio ),
      status_led( status_led_gpio ),
      error_led( error_led_gpio ),
      power_led( power_led_gpio ),
      curr_state( IDLE ),
      time_since_start( 0 ),
      last_transition_ms( 0 )
{
  debug( "FSM start\n" );

  power_led.on();
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

fsm_state_t next_state( fsm_state_t curr_state, bool button_pressed,
                        bool omron_done, bool lorawan_joined,
                        bool lorawan_sent )
{
  // debug( "[FSM] Current State: %d (%d, %d, %d, %d)\n", curr_state,
  //        button_pressed, omron_done, lorawan_joined, lorawan_sent );
  switch ( curr_state ) {
    case IDLE:
      return button_pressed ? START_MEASURE : IDLE;
    case START_MEASURE:
      return WAIT_MEASURE;
    case WAIT_MEASURE:
      return omron_done ? START_TRANSMIT : WAIT_MEASURE;
    case START_TRANSMIT:
      return lorawan_joined ? WAIT_TRANSMIT : START_TRANSMIT;
    case WAIT_TRANSMIT:
      return lorawan_sent ? DONE : WAIT_TRANSMIT;
    case DONE:
      return IDLE;
    default:
      return IDLE;
  }
}

// -----------------------------------------------------------------------
// pack_data
// -----------------------------------------------------------------------
// Packs our blood pressure data into an array of bytes

uint8_t get_byte( uint16_t data, bool high )
{
  if ( high ) {
    data = data >> 8;
  }
  return data & 0xFF;
}

void pack_data( omron_data_t* unpacked_data, uint8_t packed_data[6] )
{
  packed_data[0] = get_byte( unpacked_data->sys_pressure, true );
  packed_data[1] = get_byte( unpacked_data->sys_pressure, false );
  packed_data[2] = get_byte( unpacked_data->dia_pressure, true );
  packed_data[3] = get_byte( unpacked_data->dia_pressure, false );
  packed_data[4] = get_byte( unpacked_data->bpm, true );
  packed_data[5] = get_byte( unpacked_data->bpm, false );
}

// -----------------------------------------------------------------------
// update
// -----------------------------------------------------------------------

void FSM::update()
{
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Get button updates
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  button.update();
  bool button_pressed = button.just_pressed();

  if ( time_since_start < 300 ) {
    time_since_start += 5;
    return;  // Let start without a phantom button press
             // from getting setup
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Get Omron updates
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  bool omron_done = omron.omron_ready();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Get LoRaWAN updates
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  bool lorawan_joined;
  bool lorawan_sent;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Take action based on state
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t packed_data[6];
  pack_data( &curr_data, packed_data );

  switch ( curr_state ) {
    case IDLE:
      break;
    case START_MEASURE:
      omron.connect_to_server();
      break;
    case WAIT_MEASURE:
      if ( omron_done ) {
        curr_data = omron.curr_data;
      }
      break;
    case START_TRANSMIT:
      lorawan_joined = lorawan.try_join();
      break;
    case WAIT_TRANSMIT:
      lorawan_sent = lorawan.try_send( packed_data, 6 );
      aes128_encrypt_6byte_msg( key[16], msg[6], ciphertext[16]);
      break;
    case DONE:
      omron.omron_reset();
      break;
    default:
      break;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Update LEDs
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint32_t curr_time     = to_ms_since_boot( get_absolute_time() );
  uint32_t time_in_state = curr_time - last_transition_ms;

  switch ( curr_state ) {
    case START_MEASURE:
    case WAIT_MEASURE:
      status_led.on();
    case START_TRANSMIT:
    case WAIT_TRANSMIT:
      status_led.blink( 500 );
      break;
    default:
      status_led.off();
      break;
  }

  switch ( curr_state ) {
    case WAIT_MEASURE:
      if ( ( time_in_state > 5000 ) & !omron.discovered() ) {
        error_led.on();
      }
      else {
        error_led.off();
      }
      break;
    case START_TRANSMIT:
    case WAIT_TRANSMIT:
      if ( time_in_state > 10000 ) {
        error_led.blink( 500 );
      }
      else {
        error_led.off();
      }
      break;
    default:
      error_led.off();
      break;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Update state
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  fsm_state_t old_state = curr_state;
  curr_state = next_state( curr_state, button_pressed, omron_done,
                           lorawan_joined, lorawan_sent );

  if ( old_state != curr_state ) {
    last_transition_ms = curr_time;
  }
}
