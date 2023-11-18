# Arduino-UNO-R4-ADC-timings-and-measurement-of-the-RA4M1-chip-die-temperature
Arduino UNO R4 test code to measure the RA4M1 chip die-temperature with register level ADC operation, and ADC conversion timing measurments using the AGT1 timer.

Also demonstrates a hijack of IDE defined AGT0 millisecond timer to enable a user timing interrupt which can be set to 1.0mS / 1.0kHz or another timing interval e.g. 10mS / 100Hz, as well as run additional code from within that interrupt.

The AGT1 timing method allows users without an oscilloscope to get sub-uS accurate timing of code exercution, e.g. to investigate code options, how long the Serial.print() takes, etc.
