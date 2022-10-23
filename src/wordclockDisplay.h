#include <Arduino.h>
#include <NeoPixelBus.h>

#ifndef WORDCLOCK_DISPLAY_CLASS_H
#define WORDCLOCK_DISPLAY_CLASS_H

//////////////////////////////////////////////
// Includes
//////////////////////////////////////////////
#include "hyperdisplay.h"

//////////////////////////////////////////////
// Class Definition
//////////////////////////////////////////////
class wordclockDisplay : virtual public hyperdisplay{
private:
protected:

   NeoPixelBus<NeoGrbFeature,NeoEsp32I2s1800KbpsMethod> strip;

public:
   // Constructor: at minimum pass in the size of the display 
   // 
   //    xSize: number of pixels in the x direction of the display
   //    ySize: number of pixels in the y direction of the display
   // 
   wordclockDisplay(uint16_t xSize, uint16_t ySize, uint16_t pin);												
   void    begin(uint16_t brightness=20);

   // getoffsetColor: allows hyperdisplay to use your custom color type
   //
   //    base:      the pointer to the first byte of the array that holds the color data
   //    numPixels: the number of pixels away from the beginning that the function 
   //               should return the pointer to
   //      
   color_t getOffsetColor(color_t base, uint32_t numPixels);

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
    void    hwpixel(hd_hw_extent_t x0, hd_hw_extent_t y0, color_t data = NULL, 
   		   hd_colors_t colorCycleLength = 1, hd_colors_t startColorOffset = 0);

    void    swpixel(hd_extent_t x0, hd_extent_t y0, color_t data = NULL, 
   		   hd_colors_t colorCycleLength = 1, hd_colors_t startColorOffset = 0);

   
   // Outputs the current window's buffered data to the display
   void    show(wind_info_t * wind = NULL);   

   // Override to Disable, Neopixel Library has built in buffer.
   void    direct(wind_info_t * wind = NULL);

   void clear();

};

#endif /* WORDCLOCK_DISPLAY_CLASS_H */
