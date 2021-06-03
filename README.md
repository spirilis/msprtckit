# msprtckit
RTC kit for MSP430 microcontrollers with basic 1-second tick

## Contents

* rtckit.c
* rtckit.h

## Usage

Include "rtckit.h" in your code.  Several functions are provided:

* ``rtc_init()``
* ``rtc_interpret()``
* ``rtc_epoch()``

---
The *rtc_init()* function is intended to be used on hardware that supports an RTC peripheral
for keeping track of the time.  1 parameter is expected:

*unsigned int* ``rtc_clock_source``

This variable may be passed several options:

* RTC_CLOCK_XT1CLK
* RTC_CLOCK_VLOCLK
* RTC_CLOCK_SMCLK

The XT1CLK and VLOCLK will be the most popular options - VLOCLK requires zero setup but is not
entirely accurate.  XT1CLK requires the user configure the clock system and activate the
32.768KHz crystal before starting.  SMCLK is flexible, and auto-detects the speed of the
processor and SMCLK clock divider, but you may not enter sleep states below LPM0 for this
to work correctly.

Upon initialization, the RTC will run an interrupt service routine - given that you have left
the statement:

    #define RTCKIT_LIBRARY_PROVIDES_ISR 1

fully intact within the *rtckit.h* file.  This ISR will increment an ``rtcepoch`` global variable,
and every time the RTC fires (once per second), the ``rtc_status`` global variable will have its
``RTC_TICK`` bitfield set and will wake the processor up from sleep.

*It is important* that you set ``rtcepoch`` ahead of time somehow - through a special configuration
mode of your software, or if you intend to reset it every time the CPU restarts, using a statement
in your ``main()`` function before calling *rtc_init()*

There are also two alarms supported - ``rtcalarm0`` and ``rtcalarm``, when ``rtcepoch``
reaches these numbers, *IF they are set to a value > 0* some alarm bitfields are set in ``rtc_status``:

* ``RTCALARM_0_TRIGGERED``
* ``RTCALARM_1_TRIGGERED``

These bitfields may be tested - and cleared - from your main loop.  Optionally, two extra variables
exist:

* ``rtcalarm0_incr``
* ``rtcalarm1_incr``

If these variables are set to a value > 0, the RTC Interrupt Service Routine will automatically
increment ``rtcalarm0`` or ``rtcalarm1`` by the appropriate amount every time it fires, allowing
your code to simply respond to alarms and not worry about incrementing the alarm counter.

---
The *rtc_interpret()* function takes a timestamp in "epoch" format - the number of seconds that
has elapsed since January 1, 1970 at midnight UTC.  It will return a pointer to a
``struct tm`` object - see the C library *<time.h>* header for the format, but replicated here
is the format of this data structure:

```c
struct tm
{
    int tm_sec;      /* seconds after the minute   - [0,59]  */
    int tm_min;      /* minutes after the hour     - [0,59]  */
    int tm_hour;     /* hours after the midnight   - [0,23]  */
    int tm_mday;     /* day of the month           - [1,31]  */
    int tm_mon;      /* months since January       - [0,11]  */
    int tm_year;     /* years since 1900                     */
    int tm_wday;     /* days since Sunday          - [0,6]   */
    int tm_yday;     /* days since Jan 1st         - [0,365] */
    int tm_isdst;    /* Daylight Savings Time flag           */
};
```

Note the *rtc_interpret()* function returns a pointer to a static buffer.  Subsequent calls to
*rtc_interpret()* will overwrite these contents so it's important to use the values immediately
or copy them to another ``struct tm`` buffer.

*rtc_interpret()* receives 1 function parameter:

*unsigned long* ``epoch``

It outputs a pointer to a *struct tm* buffer.

---
The *rtc_epoch()* function does the opposite of *rtc_interpret()* - you supply a pointer to a
``struct tm`` buffer, and *rtc_epoch()* does its best to interpret the actual time - in epoch
format - represented by this buffer.  This is very useful for setting the ``rtcepoch`` variable
based on a clock input parameter.

*rtc_epoch()* receives 1 function parameter:

*struct tm* ``timebuf``

It outputs an *unsigned long* return value.

---

All of these functions correct for Leap Years.  The timezone assumed is UTC - timezone conversion
will be added at a later date to the library, most likely by using the *rtc_interpret()* function
to analyze the time window in which timezone correction is expected and adjusting the epoch
likewise.
