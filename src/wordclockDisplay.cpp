//#include <Arduino.h>
//#include <Adafruit_NeoPixel.h>
   #include "wordclockDisplay.h"

   // Constructor: at minimum pass in the size of the display 
   // 
   //    xSize: number of pixels in the x direction of the display
   //    ySize: number of pixels in the y direction of the display
   // 
   wordclockDisplay::wordclockDisplay(uint16_t xSize, uint16_t ySize, uint16_t pin): 
	   hyperdisplay(xSize, ySize),
	   strip(xSize*ySize, pin, NEO_GRB + NEO_KHZ800)
   {
     buffer(); // Always enable buffer mode.
   }		   

   void wordclockDisplay::begin(uint16_t brightness) {
     strip.begin();
     strip.setBrightness(20);
   }

   // getoffsetColor: allows hyperdisplay to use your custom color type
   //
   //    base:      the pointer to the first byte of the array that holds the color data
   //    numPixels: the number of pixels away from the beginning that the function 
   //               should return the pointer to
   //      
   color_t wordclockDisplay::getOffsetColor(color_t base, uint32_t numPixels){
      color_t pret = NULL;
      uint32_t * ptemp = (uint32_t *)base; // TODO: Make a custom type that saves a byte
      pret = (color_t)(ptemp + numPixels);
      return pret; 	
   }

   // hwPixel: the method by which hyperdisplay actually changes your screen
   //
   //	 x0, y0:           the x and y coordinates at which to place the pixel. 0,0 is the 
   //	                   upper-left corner of the screen, x is horizontal and y is vertical
   //    data:             the pointer to where the color data is stored
   //    colorCycleLength: this indicates how many pixels worth of valid color data exist
   //                      contiguously after the memory location pointed to by color.  
   //    startColorOffset: this indicates how many pixels to offset by from the 
   //                      color pointer to arrive at the actual color to display
   //
   void    wordclockDisplay::hwpixel( hd_hw_extent_t x0, hd_hw_extent_t y0, color_t data,
		                      hd_colors_t colorCycleLength, 
				      hd_colors_t startColorOffset ) {
     // TODO: Work out if colorCycleLength/startColorOffset are useful for anything.
     uint16_t sp;
     uint16_t rev = y0 & 0x0001; // odd rows are reverse order.

     // Row offset
     sp = ((yExt - 1) - y0) * xExt; // Hopefully when xExt is a pow2 compiler will use shift.
     // Column offset
     sp += ( rev > 0 ) ?  (xExt-1) - x0 : x0;

     strip.setPixelColor(sp, *(uint32_t*)data);
   }

   void    wordclockDisplay::swpixel( hd_extent_t x0, hd_extent_t y0, color_t data,
		                      hd_colors_t colorCycleLength, 
				      hd_colors_t startColorOffset ) {
     uint32_t * buff;
     uint16_t sp;

     sp = (y0 * xExt) + x0;
     buff = (uint32_t *)(pCurrentWindow->data);
     buff[sp] = *(uint32_t*)data; 
   }
   
   // Outputs the current window's buffered data to the display
   void    wordclockDisplay::show(wind_info_t * wind){
      hyperdisplay::show(wind);
      strip.show();
   }

   // Override to Disable, Neopixel Library has built in buffer.
   void    wordclockDisplay::direct(wind_info_t * wind){
      return;
   }

   // Propagate Neopixel Color function
   uint32_t wordclockDisplay::color(uint8_t r, uint8_t g, uint8_t b) {
      return strip.Color(r,g,b) ;
   } 

   void wordclockDisplay::clear() {
      uint32_t empty =  color(0,0,0);
      fillWindow( &empty );
   }

