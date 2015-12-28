#include <j3p.h>
#include <common-config.h>
#include <tick.h>
#include <string.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "beep.h"
#include "settings.h"
#include "config.h"
#include "btn.h"
#include "input.h"

static struct j3p_master_ctx g_j3p_master_ctx;

static comm_buf g_master_buf;

#define IE_MENU IE_BTN1_LONG

#define IE_ENTER IE_BTN1_LONG
#define IE_UP IE_BTN0_SHORT
#define IE_DOWN IE_BTN1_SHORT
#define IE_BACK IE_BTN0_LONG
#define IE_EXIT IE_BOTH

struct page {
  void (*render)(void);
  void (*input_event)(enum input_event);
};

static volatile struct {
  // The word we currently display
  struct anim_word anim_word;

  // Our idea of the present slaves.  The slave ids start at 1, so 0 means
  // "no slave present".
  struct slave_seq slave_seq;

  // Incremented once per clock cycle.  Once this reaches SLAVE_QUERY_TIMER_MAX,
  // we initiate a slave query.
  uint16_t slave_query_timer;
  uint8_t slave_has_answered;

  struct btn_state btn0;
  struct btn_state btn1;
  struct input_state input;
  struct beep_state beep;

  struct {
    struct page *cur_page;
    uint8_t cur_custom_anim_word_idx;
  } menu;
} g_state;


static uint8_t beep_freq_table[] = {
  25,
  50,
  100,
  125,
  150,
  175,
  200,
  225,
  250,
};

static void beep_menu (struct beep_note *song)
{
  if (settings_get_sound_menu ()) {
    beep (&g_state.beep, song);
  }
}
/*
static void beep_melody (struct beep_note *song)
{
  if (settings_get_sound_melody ()) {
    beep (song);
  }
}*/

#define PAGE_RENDER(name) \
static void page_##name##_render (void)

#define PAGE_INPUT_EVENT(name) \
static void page_##name##_input_event (enum input_event ev)

#define PAGE(name) \
extern struct page page_##name; \
PAGE_RENDER(name); \
PAGE_INPUT_EVENT(name); \
struct page page_##name = { \
  page_##name##_render, \
  page_##name##_input_event, \
}

PAGE(about);
PAGE(about_scroll);
PAGE(custom);
PAGE(custom_select);
PAGE(custom_show);
PAGE(custom_show_show);
PAGE(custom_set);
PAGE(custom_enable);
PAGE(custom_chars);
PAGE(custom_chars_edit);
PAGE(custom_anim);
PAGE(custom_anim_edit);
PAGE(sound);
PAGE(sound_menu);
PAGE(sound_melody);

#define PAGE_GOTO(input, dest) \
case input: \
  g_state.menu.cur_page = &page_##dest; \
  break

#define MENU_QUIT(input) \
case input: \
  g_state.menu.cur_page = NULL; \
  break

PAGE_RENDER(about)
{

}

PAGE_INPUT_EVENT(about)
{
  switch (ev) {
  PAGE_GOTO(IE_UP, custom);
  PAGE_GOTO(IE_DOWN, sound);
  PAGE_GOTO(IE_ENTER, about_scroll);
  MENU_QUIT(IE_BACK);

  default:
    break;
  }
}


PAGE_RENDER(about_scroll)
{

}

PAGE_INPUT_EVENT(about_scroll)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, about);
  PAGE_GOTO(IE_ENTER, about);
  PAGE_GOTO(IE_DOWN, about);
  PAGE_GOTO(IE_UP, about);

  default:
    break;
  }
}

PAGE_RENDER(custom)
{

}

PAGE_INPUT_EVENT(custom)
{
  switch (ev) {
  PAGE_GOTO(IE_UP, sound);
  PAGE_GOTO(IE_DOWN, about);
  MENU_QUIT(IE_BACK);

  case IE_ENTER:
    g_state.menu.cur_page = &page_custom_select;
    g_state.menu.cur_custom_anim_word_idx = 0;
    break;

  default:
    break;
  }
}

PAGE_RENDER(custom_select)
{

}

PAGE_INPUT_EVENT(custom_select)
{
  switch (ev) {
  PAGE_GOTO(IE_ENTER, custom_show);
  PAGE_GOTO(IE_BACK, custom);

  case IE_UP:
    if (g_state.menu.cur_custom_anim_word_idx == 0) {
      g_state.menu.cur_custom_anim_word_idx = NUM_CUSTOM_ANIM_WORDS - 1;
    } else {
      g_state.menu.cur_custom_anim_word_idx--;
    }
    break;

  case IE_DOWN:
    if (g_state.menu.cur_custom_anim_word_idx == (NUM_CUSTOM_ANIM_WORDS - 1)) {
      g_state.menu.cur_custom_anim_word_idx = 0;
    } else {
      g_state.menu.cur_custom_anim_word_idx++;
    }
    break;

  default:
    break;
  }
}

PAGE_RENDER(custom_show)
{

}

PAGE_INPUT_EVENT(custom_show)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_select);
  PAGE_GOTO(IE_UP, custom_anim);
  PAGE_GOTO(IE_DOWN, custom_set);
  PAGE_GOTO(IE_ENTER, custom_show_show);

  default:
    break;
  }
}

PAGE_RENDER(custom_show_show)
{

}

PAGE_INPUT_EVENT(custom_show_show)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_show);
  PAGE_GOTO(IE_ENTER, custom_show);
  PAGE_GOTO(IE_DOWN, custom_show);
  PAGE_GOTO(IE_UP, custom_show);

  default:
    break;
  }
}

PAGE_RENDER(custom_set)
{

}

PAGE_INPUT_EVENT(custom_set)
{
  switch (ev) {

  case IE_ENTER:
    // TODO save slave_seq / custom anim
    break;

  PAGE_GOTO(IE_BACK, custom_select);
  PAGE_GOTO(IE_DOWN, custom_enable);
  PAGE_GOTO(IE_UP, custom_show);

  default:
    break;
  }
}

PAGE_RENDER(custom_enable)
{

}

PAGE_INPUT_EVENT(custom_enable)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_select);
  PAGE_GOTO(IE_DOWN, custom_chars);
  PAGE_GOTO(IE_UP, custom_set);

  case IE_ENTER:
    // TODO: save settings
    break;

  default:
    break;
  }
}

PAGE_RENDER(custom_chars)
{

}

PAGE_INPUT_EVENT(custom_chars)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_select);
  PAGE_GOTO(IE_ENTER, custom_chars_edit);
  PAGE_GOTO(IE_DOWN, custom_anim);
  PAGE_GOTO(IE_UP, custom_enable);

  default:
    break;
  }
}

PAGE_RENDER(custom_chars_edit)
{

}

PAGE_INPUT_EVENT(custom_chars_edit)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_chars);

  case IE_ENTER:
    break;
  case IE_DOWN:
    break;
  case IE_UP:
    break;
    // All 3 todo

  default:
    break;
  }
}

PAGE_RENDER(custom_anim)
{

}

PAGE_INPUT_EVENT(custom_anim)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_select);
  PAGE_GOTO(IE_ENTER, custom_anim_edit);
  PAGE_GOTO(IE_DOWN, custom_show);
  PAGE_GOTO(IE_UP, custom_chars);

  default:
    break;
  }
}

PAGE_RENDER(custom_anim_edit)
{

}

PAGE_INPUT_EVENT(custom_anim_edit)
{
  switch (ev) {
  PAGE_GOTO(IE_BACK, custom_anim);

  case IE_ENTER:
    break;
  case IE_DOWN:
    break;
  case IE_UP:
    break;
    // All 3 todo

  default:
    break;
  }
}

PAGE_RENDER(sound)
{

}

PAGE_INPUT_EVENT(sound)
{
  switch (ev) {
  PAGE_GOTO(IE_UP, about);
  PAGE_GOTO(IE_DOWN, custom);
  PAGE_GOTO(IE_ENTER, sound_menu);
  MENU_QUIT(IE_BACK);

  default:
    break;
  }
}

PAGE_RENDER(sound_menu)
{

}

PAGE_INPUT_EVENT(sound_menu)
{
  switch (ev) {
  PAGE_GOTO(IE_UP, sound_melody);
  PAGE_GOTO(IE_DOWN, sound_melody);
  PAGE_GOTO(IE_BACK, sound);

  case IE_ENTER:
    settings_set_sound_menu (!settings_get_sound_menu ());
    break;

  default:
    break;
  }
}

PAGE_RENDER(sound_melody)
{

}

PAGE_INPUT_EVENT(sound_melody)
{
  switch (ev) {
  PAGE_GOTO(IE_UP, sound_menu);
  PAGE_GOTO(IE_DOWN, sound_menu);
  PAGE_GOTO(IE_BACK, sound);

  case IE_ENTER:
    settings_set_sound_melody (!settings_get_sound_melody ());
    break;

  default:
    break;
  }
}

/*
 * Get the current sequence of slaves present.
 *
 * @param slave_sequence Pointer to an array of MAX_CUBES uint8_t's.
 */
void get_slave_sequence (struct slave_seq *slave_seq)
{
  ATOMIC_BLOCK (ATOMIC_FORCEON) {
    memcpy (slave_seq, (void *) &g_state.slave_seq, sizeof (*slave_seq));
  }
}

/*
 * Get the number of slaves present.
 */
uint8_t get_slave_count (void)
{
  uint8_t cnt = 0;

  ATOMIC_BLOCK (ATOMIC_FORCEON) {
    for (cnt = 0;
         cnt < MAX_CUBES && g_state.slave_seq.ids[cnt] != 0;
         cnt++);
  }

  return cnt;
}

/*
 * Set the currently displayed word and animation pattern.  The information is<
 * so that it will be propagated the next time we send a message to the
 * slaves.
 *
 * @param word Pointer to an array of MAX_CUBES characters.  Unused characters
 *             should be set to 0.
 * @param patterns Pointer to an array of MAX_CUBES animation patterns.  */

void set_current_anim_word (struct anim_word *anim_word)
{
  ATOMIC_BLOCK (ATOMIC_FORCEON) {
    memcpy ((void *) &g_state.anim_word, anim_word, sizeof (struct anim_word));
  }
}

static void status_led_red (void)
{
  STATUS_LED_RED_PORT &= ~STATUS_LED_RED_MASK;
  STATUS_LED_BLUE_PORT |= STATUS_LED_BLUE_MASK;
  STATUS_LED_GREEN_PORT |= STATUS_LED_GREEN_MASK;
}

static void status_led_green (void)
{
  STATUS_LED_RED_PORT |= STATUS_LED_RED_MASK;
  STATUS_LED_BLUE_PORT |= STATUS_LED_BLUE_MASK;
  STATUS_LED_GREEN_PORT &= ~STATUS_LED_GREEN_MASK;
}

static void status_led_blue (void)
{
  STATUS_LED_RED_PORT |= STATUS_LED_RED_MASK;
  STATUS_LED_BLUE_PORT &= ~STATUS_LED_BLUE_MASK;
  STATUS_LED_GREEN_PORT |= STATUS_LED_GREEN_MASK;
}

static struct beep_note song_down[] = {
  { MS_TO_TICKS(50), 5 },
  { 0, 0 },
};

static struct beep_note song_up[] = {
  { MS_TO_TICKS(50), 7 },
  { 0, 0 },
};

static struct beep_note song_back[] = {
  { MS_TO_TICKS(50), 5 },
  { MS_TO_TICKS(50), 7 },
  { 0, 0 },
};

static struct beep_note song_enter[] = {
  { MS_TO_TICKS(50), 7 },
  { MS_TO_TICKS(50), 5 },
  { 0, 0 },
};

static struct beep_note song_exit_menu[] = {
  { MS_TO_TICKS(50), 5 },
  { MS_TO_TICKS(50), 6 },
  { MS_TO_TICKS(50), 7 },
  { 0, 0 },
};

static struct beep_note song_enter_menu[] = {
  { MS_TO_TICKS(50), 7 },
  { MS_TO_TICKS(50), 6 },
  { MS_TO_TICKS(50), 5 },
  { 0, 0 },
};

static void menu_input_event (enum input_event ev)
{
  if (ev == IE_EXIT) {
    g_state.menu.cur_page = NULL;
  } else {
    g_state.menu.cur_page->input_event (ev);
  }

  if (g_state.menu.cur_page == NULL) {
    beep_menu (song_exit_menu);
  } else {
    switch (ev) {
    case IE_UP:
      beep_menu (song_up);
      break;

    case IE_ENTER:
      beep_menu (song_enter);
      break;

    case IE_DOWN:
      beep_menu (song_down);
      break;

    case IE_BACK:
      beep_menu (song_back);
      break;

    case IE_EXIT:
      break;
    }
  }
}

static void input_event (enum input_event ev)
{
  if (g_state.menu.cur_page != NULL) {
    menu_input_event (ev);
  } else {
    if (ev == IE_MENU) {
      beep_menu (song_enter_menu);
      g_state.menu.cur_page = &page_sound;
    }
  }
}

/* Called when a slave query times out. */
static void slave_timeout (void)
{
  memset ((void *) &g_state.slave_seq, 0,
          sizeof (g_state.slave_seq));;
}

static void slave_is_alive (void)
{

}

static void rising (void)
{
  j3p_master_on_rising (&g_j3p_master_ctx);

  g_state.slave_query_timer++;

  if (g_state.slave_query_timer >= SLAVE_QUERY_TIMER_MAX) {
    /* If we are about to send another query and the slave hasn't
     * answered the previous one, assume he is not there. */
    if (!g_state.slave_has_answered) {
      slave_timeout ();
    }

    // Fill message to slave.
    g_master_buf.m2s.rank = 0;
    memcpy (&g_master_buf.m2s.anim_word, (void *) &g_state.anim_word, sizeof (struct anim_word));

    // Send it!
    j3p_master_query (&g_j3p_master_ctx);
    g_state.slave_has_answered = 0;
    g_state.slave_query_timer = 0;
  }

  tick ();
}

static void falling (void)
{
  j3p_master_on_falling (&g_j3p_master_ctx);
}

ISR(TIMER0_COMP_vect)
{
  // FIXME: I think we could read back from PORT, but I am not sure.  I put this
  // just to be sure.
  static uint8_t clock_value = 0;

  if (clock_value == 0) {
    clock_value = 1;
    MASTER_CLK_OUTPUT_PORT |= MASTER_CLK_OUTPUT_MASK;
    rising ();
  } else {
    clock_value = 0;
    MASTER_CLK_OUTPUT_PORT &= ~MASTER_CLK_OUTPUT_MASK;
    falling ();
  }
}

static void master_line_up (void)
{
  COMM_MASTER_DDR &= ~COMM_MASTER_MASK;
}

static void master_line_down (void)
{
  COMM_MASTER_DDR |= COMM_MASTER_MASK;
}

static uint8_t master_read_line (void)
{
  return (COMM_MASTER_PIN & COMM_MASTER_MASK) != 0;
  return 0;
}

static void master_query_complete (uint8_t *_buf)
{
  comm_buf *buf = (comm_buf *) _buf;

  g_state.slave_has_answered = 1;
  slave_is_alive();

  memcpy ((void *) &g_state.slave_seq, &buf->s2m.slave_seq, sizeof (g_state.slave_seq));
}

static void init_state (void)
{
  memset ((void *) &g_state, 0, sizeof (g_state));
}

static void init_comm (void)
{
  // TODO: init comm pins
  j3p_master_init (&g_j3p_master_ctx,
                   master_line_up, master_line_down,
                   master_read_line,
                   sizeof(struct m2s_data),
                   sizeof(struct s2m_data),
                   (uint8_t *) &g_master_buf,
                   master_query_complete);
}

static void init_clk (void)
{
  MASTER_CLK_OUTPUT_DDR |= MASTER_CLK_OUTPUT_MASK;
  MASTER_CLK_OUTPUT_PORT &= ~MASTER_CLK_OUTPUT_MASK;

  // CTC, clk/8 (= 1 MHz)
  TCCR0 |= _BV(WGM01) | _BV(CS01);

#define TIMER_PRESCALER 8
#define TIMER_FREQ (F_CPU / TIMER_PRESCALER)
#define TIMER_TOP (TIMER_FREQ / (2 * MASTER_CLK_FREQ))

  OCR0 = TIMER_TOP;

  // Reset the timer, just for fun.
  TCNT0 = 0;

  // Enable timer interrupt
  TIMSK |= _BV(OCIE0);
}

static void init_state_led (void)
{
  STATUS_LED_RED_DDR |= STATUS_LED_RED_MASK;
  STATUS_LED_GREEN_DDR |= STATUS_LED_GREEN_MASK;
  STATUS_LED_BLUE_DDR |= STATUS_LED_BLUE_MASK;
  STATUS_LED_ANODE_DDR |= STATUS_LED_ANODE_MASK;

  STATUS_LED_RED_PORT |= STATUS_LED_RED_MASK;
  STATUS_LED_GREEN_PORT |= STATUS_LED_GREEN_MASK;
  STATUS_LED_BLUE_PORT |= STATUS_LED_BLUE_MASK;
  STATUS_LED_ANODE_PORT |= STATUS_LED_ANODE_MASK;
}

static void init_buttons (void)
{
  BTN0_DDR &= ~BTN0_MASK;
  BTN1_DDR &= ~BTN1_MASK;

  uint8_t btn0_val (void)
  {
    return BTN0_PIN & BTN0_MASK;
  }

  uint8_t btn1_val (void)
  {
    return BTN1_PIN & BTN1_MASK;
  }

  init_btn (&g_state.btn0, btn0_val);
  init_btn (&g_state.btn1, btn1_val);
}

static void init_inputs (void)
{
  enum btn_state_value btn0_state (void)
  {
    return btn_get_state (&g_state.btn0);
  }

  enum btn_state_value btn1_state (void)
  {
    return btn_get_state (&g_state.btn1);
  }

  init_input (&g_state.input, btn0_state, btn1_state, input_event);
}

static void init_alsa (void)
{
  // CTC mode, toggle OC2 on compare, prescaler = 64
  TCCR2 |= _BV(WGM21) | _BV(CS22);
  TCNT2 = 0x00;
  OCR2 = 0xff;

  PIEZO_PASSIVE_DDR |= PIEZO_PASSIVE_MASK;
  PIEZO_PASSIVE_PORT &= ~PIEZO_PASSIVE_MASK;

  PIEZO_ACTIVE_DDR |= PIEZO_ACTIVE_MASK;


  void beep_off (void)
  {
    TCCR2 &= ~_BV(COM20);
  }

  void beep_on (uint8_t freq_idx)
  {
    TCCR2 |= _BV(COM20);
    OCR2 = beep_freq_table[freq_idx];
  }

  init_beep (&g_state.beep, beep_on, beep_off);
}

static void render_loop (void)
{

}

static void status_led_loop (void)
{
  if (g_state.menu.cur_page != NULL) {
    status_led_green ();
  } else if (get_slave_count() > 0) {
    status_led_blue ();
  } else {
    status_led_red ();
  }
}

static void loop ()
{
  for (;;) {
    btn_loop (&g_state.btn0);
    btn_loop (&g_state.btn1);
    input_loop (&g_state.input);
    beep_loop (&g_state.beep);
    render_loop ();
    status_led_loop ();
  }
}

static void play_intro (void)
{
  // TODO
}

int main (void)
{
  init_state ();
  init_comm ();
  init_clk ();
  init_state_led ();
  init_buttons ();
  init_inputs ();
  init_alsa ();
  init_settings ();

  play_intro ();

  sei();

  loop();
}
