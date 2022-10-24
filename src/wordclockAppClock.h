#include <Arduino.h>
#include <time.h>

#include "wordclockDisplay.h"

#ifndef WORDCLOCK_APP_CLOCK_H
#define WORDCLOCK_APP_CLOCK_H

typedef struct {
   const uint16_t x;
   const uint16_t y;
   const uint16_t len;
} draw_cmd_line_t;

typedef struct {
   const uint16_t          num;
   const draw_cmd_line_t * cmds;
} draw_cmds_t;

enum wordclockAppClock_pre_time_enum { the_time_is=1, it_is=2 }; 
const draw_cmds_t wordclockAppClock_pre_time[] = {
                     // 0 - None
                     { .num = 0, .cmds = NULL },
                     // 1 - The Time Is
                     { .num = 3,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=1,  .y=0,  .len=3 },
                          { .x=6,  .y=0,  .len=4 },
                          { .x=13, .y=0,  .len=2 }
                       }
                     },
                     // 2 - It is
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=5,  .y=0,  .len=2 },
                          { .x=13, .y=0,  .len=2 }
                       }
                     }
                  };

const draw_cmds_t wordclockAppClock_minutes[] = {
                     // 0 - None
                     { .num = 0, .cmds = NULL },
                     // 1 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=7,  .y=1,  .len=3 } }},
                     // 2 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=10, .y=1,  .len=3 } }},
                     // 3 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=1,  .y=2,  .len=5 } }},
                     // 4 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=6,  .y=2,  .len=4 } }},
                     // 5 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=11, .y=1,  .len=4 } }},
                     // 6 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=8,  .y=3,  .len=3 } }},
                     // 7 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=11, .y=3,  .len=5 } }},
                     // 8 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=0,  .y=4,  .len=5 } }},
                     // 9 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=7,  .y=4,  .len=4 } }},
                     //10 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=12, .y=4,  .len=3 } }},
                     //11 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=10, .y=5,  .len=6 } }},
                     //12 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=0,  .y=6,  .len=6 } }},
                     //13 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=1,  .y=5,  .len=4 },
                          { .x=5,  .y=5,  .len=4 }
                       }
                     },
                     //14 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=6,  .y=2,  .len=4 },
                          { .x=5,  .y=5,  .len=4 }
                       }
                     },
                     //15 - QUARTER
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=0,  .y=3,  .len=7 } }},
                     //16 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=8,  .y=3,  .len=3 },
                          { .x=5,  .y=5,  .len=4 }
                       }
                     },
                     //17 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=11, .y=3,  .len=5 },
                          { .x=5,  .y=5,  .len=4 }
                       }
                     },
                     //18 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=0,  .y=4,  .len=5 },
                          { .x=5,  .y=5,  .len=4 }
                       }
                     },
                     //19 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=7,  .y=4,  .len=4 },
                          { .x=5,  .y=5,  .len=4 }
                       }
                     },
                     //20 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=0,  .y=1,  .len=6 } }},
                     //21 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=7,  .y=1,  .len=3 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //22 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=10, .y=1,  .len=3 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //23 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=1,  .y=2,  .len=5 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //24 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=7,  .y=2,  .len=4 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //25 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=11, .y=1,  .len=4 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //26 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=8,  .y=3,  .len=3 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //27 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=11, .y=3,  .len=5 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //28 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=0,  .y=4,  .len=5 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
                     //29 -
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=7,  .y=4,  .len=4 },
                          { .x=0,  .y=1,  .len=6 }
                       }
                     },
		     //30 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=1,  .y=7,  .len=4 } }}
                  };

enum wordclockAppClock_words_enum { minute=1, s=2, past=3, to=4 }; 
const draw_cmds_t wordclockAppClock_words[] = {
                     // 0 - None
                     { .num = 0, .cmds = NULL },
                     // 1 - MINUTE
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=9,  .y=6, .len=6 } }},
                     // 2 - S
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=15, .y=6, .len=1 } }},
                     // 3 - PAST
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=8,  .y=7, .len=4 } }},
                     // 4 - TO
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=5,  .y=7, .len=2 } }}
                  };

const draw_cmds_t wordclockAppClock_hours[] = {
                     // 0 - 12
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=7,  .y=11, .len=6 } }},
                     // 1 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=13, .y=7,  .len=3 } }},
                     // 2 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=0,  .y=8,  .len=3 } }},
                     // 3 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=5,  .y=8,  .len=5 } }},
                     // 4 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=11, .y=8,  .len=4 } }},
                     // 5 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=1,  .y=9,  .len=4 } }},
                     // 6 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=6,  .y=9,  .len=3 } }},
                     // 7 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=10, .y=9,  .len=5 } }},
                     // 8 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=0,  .y=10, .len=5 } }},
                     // 9 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=6,  .y=10, .len=4 } }},
                     //10 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=11, .y=10, .len=3 } }},
                     //11 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=1,  .y=11, .len=6 } }},
                     //12 -
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=7,  .y=11, .len=6 } }}
                  };

enum wordclockAppClock_tod_enum { morning=1, afternoon=2, evening=3, night=4, noon=5 }; 
const draw_cmds_t wordclockAppClock_tod[] = {
                     // 0 - None
                     { .num = 0, .cmds = NULL },
                     // 1 - IN THE MORNING
                     { .num = 3,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=14, .y=11,  .len=2 },
                          { .x=1,  .y=12,  .len=3 },
                          { .x=9,  .y=12,  .len=7 }
                       }
                     },
                     // 2 - IN THE AFTERNOON
                     { .num = 3,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=14, .y=11,  .len=2 },
                          { .x=1,  .y=12,  .len=3 },
                          { .x=0,  .y=13,  .len=9 }
                       }
                     },
                     // 3 - IN THE EVENING
                     { .num = 3,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=14, .y=11,  .len=2 },
                          { .x=1,  .y=12,  .len=3 },
                          { .x=9,  .y=13,  .len=7 }
                       }
                     },
                     // 4 - AT NIGHT
                     { .num = 2,
                       .cmds=(draw_cmd_line_t[]){
                          { .x=0,  .y=12,  .len=2 },
                          { .x=4,  .y=12,  .len=5 }
                       }
                     },
		     // 5 - NOON
                     { .num = 1, .cmds=(draw_cmd_line_t[]){ { .x=5,  .y=13, .len=4 } }}
                  };

void wordclockAppClockDrawCmds( const draw_cmds_t * cmds, 
                                color_t color,
				wordclockDisplay * disp);

void wordclockAppClockDrawTime( struct tm* time, 
                                color_t color,
		                wordclockDisplay * disp);

#endif /* WORDCLOCK_APP_CLOCK_H */
