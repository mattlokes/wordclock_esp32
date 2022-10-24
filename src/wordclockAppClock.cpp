#include "wordclockAppClock.h"

void wordclockAppClockDrawCmds( const draw_cmds_t * cmds, 
                                color_t color,
				wordclockDisplay * disp) {
   for( int i=0; i< cmds->num; i++ ) {
      disp->xline( cmds->cmds[i].x, cmds->cmds[i].y, cmds->cmds[i].len, color);
   }
}

//enum wordclockAppClock_pre_time_enum { none, the_time_is, it_is } 
//enum wordclockAppClock_words_enum { none, minute, s, past, to } 
//enum wordclockAppClock_tod_enum { none, morning, afternoon, evening, night,noon }
//const draw_cmds_t wordclockAppClock_pre_time[]
//const draw_cmds_t wordclockAppClock_minutes[]
//const draw_cmds_t wordclockAppClock_words[]
//const draw_cmds_t wordclockAppClock_hours[]
//const draw_cmds_t wordclockAppClock_tod[]

void wordclockAppClockDrawTime( struct tm* time, 
                                color_t color,
		                wordclockDisplay * disp) {
   
   uint8_t m = time->tm_min;
   uint8_t h = time->tm_hour;
   bool to_hour = ( m > 30);

   // Normalise hour to 12 hour clock
   h = ( h > 11) ? h - 12: h;
   // If mins > 30, display the next hour not current.
   h = to_hour ? h + 1 : h;

   // Normalise mins to 30
   m = to_hour ? 60 - m : m;

	
   // Draw Pre-Time Words
   wordclockAppClockDrawCmds( 
		    &wordclockAppClock_pre_time[wordclockAppClock_pre_time_enum::it_is],
		    color,
		    disp);

   // Draw Hour Words
   wordclockAppClockDrawCmds( 
		    &wordclockAppClock_hours[h],
		    color,
		    disp);

   // Draw Minutes Words
   wordclockAppClockDrawCmds( 
		    &wordclockAppClock_minutes[m],
		    color,
		    disp);

   // Draw Filler Words
   if ( m == 0) {
    // None
   } else if (m == 30) {
     wordclockAppClockDrawCmds( 
		      &wordclockAppClock_words[wordclockAppClock_words_enum::past],
		      color,
		      disp);
   } else {
     if  ( to_hour ) {
       wordclockAppClockDrawCmds( 
		        &wordclockAppClock_words[wordclockAppClock_words_enum::to],
		        color,
		        disp);
     } else {
       wordclockAppClockDrawCmds( 
		        &wordclockAppClock_words[wordclockAppClock_words_enum::past],
		        color,
		        disp);
     }
     if ( m!= 15) {
       if ( m >  1) {
         wordclockAppClockDrawCmds( 
	  	          &wordclockAppClock_words[wordclockAppClock_words_enum::s],
		          color,
		          disp);
       }
       wordclockAppClockDrawCmds( 
        		&wordclockAppClock_words[wordclockAppClock_words_enum::minute],
		        color,
		        disp);
     }
   }
}
