#ifndef J3P_SEND_H
#define J3P_SEND_H

#include <avr/io.h>

#include "j3p-common.h"

struct j3p_send_fsm {
  enum {
    J3P_SEND_STATE_START_BIT,
    J3P_SEND_STATE_BYTE,
    J3P_SEND_STATE_STOP_BIT,
    J3P_SEND_STATE_DONE,
  } state;

  j3p_set_line_op line_up, line_down;
  volatile uint8_t *buf;
  uint8_t bytes_left;

  uint8_t bits_left;
  uint8_t cur_byte;
};

void j3p_send_init (volatile struct j3p_send_fsm *fsm,
                    j3p_set_line_op line_up,
                    j3p_set_line_op line_down,
                    uint8_t bytes_out,
                    volatile uint8_t *send_buf);
void j3p_send_on_rising (volatile struct j3p_send_fsm *fsm);

static inline uint8_t j3p_send_is_done (volatile struct j3p_send_fsm *fsm)
{
  return fsm->state == J3P_SEND_STATE_DONE;
}


#endif /* J3P_SEND_H */
