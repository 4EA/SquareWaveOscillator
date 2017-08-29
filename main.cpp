/* Square (PWM) Oscillator With Keyboard Control (CV/gate)
 * Language:  C
 * Author:  Cole Jepson
 * Date:  8/15/2017
 * References: https://developer.mbed.org/media/uploads/phill/mbed_course_notes_-_pulse_width_modulation.pdf
 * http://pages.mtu.edu/~suits/notefreqs.html,
 * http://www.liutaiomottola.com/formulae/freqtab.htm
 */

#include "mbed.h"

AnalogIn ADC16(p16);        // CV input from CV keyboard/drum machine)
//AnalogOut Sine(p18);      // Sine Wave Output
PwmOut Square(p22);         // Square Wave (PWM) Output
InterruptIn gate(p30);      // dynamic gate from CV keyboard/drum machine
DigitalIn oct_select(p21);  // octave select DIP switch
DigitalOut gate_on(LED1);   // setup on-board LED1 to show the gate logic level
DigitalOut CVerror(LED2);   // setup on-board LED2 to report logic level error
DigitalOut Oct1(LED3);      // setup on-board LED3 to show if Octave 1 is selected
DigitalOut Oct2(LED4);      // setup on-board LED4 to show if Octave 2 is selected

int note_count = 0;         // declare note counter
float CV_in;                // declare ADC variable

// 0-5V in CV values for 5V logic level devices (arduino etc.)
/*static const float CV[61] =
{
    0, 0.083, 0.16, 0.25, 0.3, 0.416, 0.5, 0.583, 0.6, 0.75, 0.83, 0.916,
    1, 1.083, 1.16, 1.25, 1.3, 1.416, 1.5, 1.583, 1.6, 1.75, 1.83, 1.916,
    2, 2.083, 2.16, 2.25, 2.3, 2.416, 2.5, 2.583, 2.6, 2.75, 2.83, 2.916,
    3, 3.083, 3.16, 3.25, 3.3, 3.416, 3.5, 3.583, 3.6, 3.75, 3.83, 3.916,
    4, 4.083, 4.16, 4.25, 4.3, 4.416, 4.5, 4.583, 4.6, 4.75, 4.83, 4.916,
    5
}; */

// 0-3.3V in CV Values (divided from 0-5V) for 3.3V logic level devices (mbed)
static const float CV[61] =
{
    0, 0.054, 0.104, 0.165, 0.2, 0.2704, 0.325, 0.381, 0.39, 0.4875, 0.5395, 0.5954,
    0.65, 0.7039, 0.754, 0.82, 0.86, 0.9204, 0.975, 1.04, 1.055, 1.1375, 1.1895, 1.2454,
    1.3, 1.354, 1.404, 1.47, 1.5, 1.5704, 1.625, 1.689, 1.72, 1.7875, 1.8395, 1.905,
    1.96, 2.004, 2.054, 2.12, 2.17, 2.2204, 2.275, 2.35, 2.39, 2.4375, 2.4895, 2.5454,
    2.61, 2.654, 2.704, 2.78, 2.839, 2.89, 2.925, 2.99, 3.05, 3.0875, 3.1395, 3.1954,
    3.204
}; // B4 and C5 can't be reached with current hardware setup

static const float note_freqOct1[61] =
{
    16.35, 17.32, 18.35, 19.45, 20.6, 21.83, 23.12, 24.5, 25.96, 27.5, 29.14, 30.87,
    32.7, 34.65, 36.71, 38.89, 41.2, 43.65, 46.25, 49, 51.91, 55, 58.27, 61.74,
    65.41, 69.3, 73.42, 77.78, 82.41, 87.31, 92.5, 98, 103.83, 110, 116.54, 123.47,
    130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185, 196, 207.65, 220, 233.08, 246.94,
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392, 415.3, 440, 466.16, 493.88,
    523.25    
}; // Highest B and C can't be reached with current hardware setup

static const float note_freqOct2[60] =
{
    523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880, 932.33, 987.77,
    1046.5, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760, 1864.66, 1975.53,
    2093, 2217.46, 2349.32, 2489.02, 2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520, 3729.31, 3951.07,
    4186.01, 4434.92, 4698.63, 4978.03, 5274.04, 5587.65, 5919.91, 6271.93, 6644.88, 7040, 7458.62, 7902.13,
    8372.02, 8869.84, 9397.27, 9956.06, 10548.08, 11175.3, 11839.82, 12543.86, 13289.75, 14080, 14917.24, 15804.26
}; // Highest B and C can't be reached with current hardware setup

void open_gate()            // note on interrupt
{
    gate_on = 1;            // assert LED1 (gate on)
    CV_in = ADC16.read();   // read floating pin voltage (seed) on DIP16
    CV_in = CV_in*3.3;      // normalize input to 3.3V logic level
    note_count = 0;         // initialize counter to zero
    
    if(CV_in >= 3.2)        // with the current voltage divider setup, only 
    {                       // voltages less than 3.2V are supported
        CVerror = 1;        // B4 and C5 unsupported (assert LED2, error)
    }
    
    else
    {
         while(CV_in >= CV[note_count]) // check CV_in against CV lookup table
        {
            note_count++;   // increment CV lookup table index until the 
        }                   // the correct CV note is reached
        note_count = note_count - 1;
    }
    
    Square = 0.5;           // set PWM duty cycle (50%)     
    if(Oct2 == 1)
    {                     
        Square.period(1/(note_freqOct2[note_count]));// set PWM/Square wave period
    }
    else
    {
        Square.period(1/(note_freqOct1[note_count]));// set PWM/Square wave period
    }
}

void closed_gate()          // note off interrupt
{
    gate_on = 0;            // turn off LED1 because the gate is shut
    CVerror = 0;            // turn off LED2 if it was on
    Square = 0.0;           // turn off PWM output
}

int main() 
{
    gate.mode(PullNone);    // turn off pull up and pull down resistors
    gate.rise(&open_gate);  // setup interrupt for when the gate voltage is high
    gate.fall(&closed_gate);// setup interrupt for when the gate voltage is low
    
    while(1)
    {
        // wait indefinitely for a gate signal to trigger the interrupts
        if(oct_select == 1)
        {                     
            Oct2 = 1;   // output correct on-board LED for high or low octave
            Oct1 = 0;
        }
        else
        {
            Oct2 = 0;
            Oct1 = 1;
        }
    }
}
