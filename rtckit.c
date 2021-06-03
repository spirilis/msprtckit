/**
  * MSP430 Real Time Clock Kit
  *
  * This is a library for managing Clock Date & Time using a simple 1-second clock tick,
  * the C library's built-in <time.h> and struct tm - storing date & time information,
  * and converting between Epoch time format (unsigned long integer as # seconds since
  * the Epoch, January 1 1970 12:00AM UTC).
  *
        BSD 2-Clause License

        Copyright (c) 2021, Eric
        All rights reserved.

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this
        list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
        AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
        DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
        FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
        DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
        SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
        CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
        OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
        OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#include <msp430.h>
#include "rtckit.h"

/// Data variables
#pragma DATA_SECTION(rtcepoch, RTCKIT_STORE_VARIABLES_IN_SECTION)
volatile unsigned long rtcepoch;

#pragma DATA_SECTION(rtcalarm0, RTCKIT_STORE_VARIABLES_IN_SECTION)
volatile unsigned long rtcalarm0;
#pragma DATA_SECTION(rtcalarm0_incr, RTCKIT_STORE_VARIABLES_IN_SECTION)
volatile unsigned long rtcalarm0_incr;
#pragma DATA_SECTION(rtcalarm1, RTCKIT_STORE_VARIABLES_IN_SECTION)
volatile unsigned long rtcalarm1;
#pragma DATA_SECTION(rtcalarm1_incr, RTCKIT_STORE_VARIABLES_IN_SECTION)
volatile unsigned long rtcalarm1_incr;

volatile unsigned int rtc_status;


const struct rtcMonthInfo monthInfo[12] =
{
    {31, "Jan", "January"},
    {28, "Feb", "February"},
    {31, "Mar", "March"},
    {30, "Apr", "April"},
    {31, "May", "May"},
    {30, "Jun", "June"},
    {31, "Jul", "July"},
    {31, "Aug", "August"},
    {30, "Sep", "September"},
    {31, "Oct", "October"},
    {30, "Nov", "November"},
    {31, "Dec", "December"}
};

const struct rtcDayInfo dayInfo[7] =
{
    {"Sun", "Sunday"},
    {"Mon", "Monday"},
    {"Tue", "Tuesday"},
    {"Wed", "Wednesday"},
    {"Thu", "Thursday"},
    {"Fri", "Friday"},
    {"Sat", "Saturday"}
};

// RTC hardware implementation

/** Initialize RTC peripheral
 *  For RTC_CLK_SMCLK, the speed is guessed using DCOCLK bits and DIVS divider bits.
 *
 *  @param[in] RTC clock source - see rtckit.h for RTC_CLK_* defines.
 *
 */
void rtc_init(unsigned int rtc_clock_source)
{
    rtc_status = 0;
    RTCCTL &= ~RTCIF;
    switch (rtc_clock_source) {
    case RTC_CLOCK_XT1CLK:
        RTCCTL = RTCSS__XT1CLK | RTCPS__256;
        RTCMOD = 32768 / 256;
        break;
    case RTC_CLOCK_VLOCLK:
        RTCCTL = RTCSS__VLOCLK | RTCPS__100;
        RTCMOD = 10000 / 100;
        break;
    #ifdef __MSP430_HAS_CS__
    case RTC_CLOCK_SMCLK:
        if ( (CSCTL4 & SELMS_7) != SELMS_0 ) {
            rtc_status |= RTC_GENERAL_ERROR;
            return;  // Error condition - we don't really support SMCLK that's not DCO-derived.
        }
        // How fast is SMCLK?  First, how fast is DCOCLK-
        unsigned long speed;
        switch (CSCTL1 & DCORSEL_7) {
        case DCORSEL_0:
            speed = 1000000;
            break;
        case DCORSEL_1:
            speed = 2000000;
            break;
        case DCORSEL_2:
            speed = 4000000;
            break;
        case DCORSEL_3:
            speed = 8000000;
            break;
        case DCORSEL_4:
            speed = 12000000;
            break;
        case DCORSEL_5:
            speed = 16000000;
            break;
        case DCORSEL_6:
            speed = 20000000;
            break;
        case DCORSEL_7:
            speed = 24000000;
            break;
        }
        // Get SMCLK divider
        speed = speed / ( (CSCTL5 & DIVS_3) >> 4 );
        speed /= 1000;  // Using RTCPS__1000 divider
        RTCCTL = RTCSS__SMCLK | RTCPS__1000;
        RTCMOD = (unsigned int)speed;  // Should be 24000 at the highest now.
        break;
    #endif /* ifdef __MSP430_HAS_CS__ for FR4xxx/2xxx chips */
    default:
        rtc_status |= RTC_GENERAL_ERROR;
        return;  // Error condition - should never get here
    }
    RTCCTL |= RTCSR;
    RTCCTL |= RTCIE;
}

// RTC hardware ISR
#ifdef RTCKIT_LIBRARY_PROVIDES_ISR

#if defined(__MSP430_HAS_RTC__)
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(RTC_VECTOR))) RTC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if (RTCIV & RTCIV_RTCIF) {
        rtc_status |= RTC_TICK;
        rtcepoch++;

        if (rtcalarm0 > 0 && rtcepoch == rtcalarm0) {
            rtc_status |= RTCALARM_0_TRIGGERED;
            if (rtcalarm0_incr > 0) {
                rtcalarm0 += rtcalarm0_incr;
            }
        }
        if (rtcalarm1 > 0 && rtcepoch == rtcalarm1) {
            rtc_status |= RTCALARM_1_TRIGGERED;
            if (rtcalarm1_incr > 0) {
                rtcalarm1 += rtcalarm1_incr;
            }
        }
        __bic_SR_register_on_exit(LPM4_bits);
    }
}
#endif /* if defined __MSP430_HAS_RTC__ */
#endif /* ifdef RTC_LIBRARY_PROVIDES_ISR */


// RTC timestamp interpretation


struct tm_leapyear {
    unsigned long startOfCycle;
    unsigned long remains;
    unsigned int years;
};

/// Function prototypes of utility helper functions
unsigned int rtc_calculate_yday(struct tm *, unsigned long, unsigned int);
unsigned long rtc_calculate_last_leapyear(struct tm_leapyear *, unsigned long);

/// This buffer is offered out to user functions as return value of rtc_interpret()
static struct tm timebuf;

struct tm * rtc_interpret(unsigned long epoch)
{
    unsigned long remains_years, remains_this_year, weeks_since_epoch;
    unsigned int days_this_year = 365;
    long i, j;
    unsigned int days_february = 28;
    struct tm_leapyear leaps;

    // Calculate the end of the last leap year - respecting that 1970 wasn't a leapyear, 1972 was.
    if (rtc_calculate_last_leapyear(&leaps, epoch) == 0) {
        return NULL;  // Invalid or unsupported date
    }
    remains_years = leaps.remains / (365*86400);
    if (remains_years > 2) {
        days_february = 29;  // Leap year!
        days_this_year = 366;
    }
    if (remains_years > 3) {
        remains_years--;  // Last day of a leap year!
    }

    timebuf.tm_year = leaps.years + remains_years + 1970;
    if (days_this_year == 366) {
        remains_this_year = leaps.remains - (remains_years * 86400 * 365);
    } else {
        remains_this_year = leaps.remains - ((remains_years-1)*86400 * 365) - (86400*days_this_year);
    }
    timebuf.tm_yday = remains_this_year / 86400;

    timebuf.tm_mon = 0;  // Starting with January, calculating up
    i = timebuf.tm_yday;
    j = 0;
    while (i > -1) {
        if (timebuf.tm_mon == 1) {  // Handle February for leap years
            i -= days_february;
            j += days_february;
            timebuf.tm_mon++;
        } else {
            i -= monthInfo[timebuf.tm_mon].days;
            j += monthInfo[timebuf.tm_mon++].days;
        }
    }
    timebuf.tm_mon--;
    if (timebuf.tm_mon == 1) {
        j -= days_february;
    } else {
        j -= monthInfo[timebuf.tm_mon].days;
    }
    timebuf.tm_mday = (remains_this_year - (j*86400)) / 86400 + 1;
    i = remains_this_year - timebuf.tm_yday * 86400;  // i is now the # of seconds for today
    timebuf.tm_hour = i / 3600;
    j = i % 3600;
    timebuf.tm_min = j / 60;
    timebuf.tm_sec = j % 60;

    // Calculating time of week - use the epoch, Jan 1 1970 was on a Thursday.
    weeks_since_epoch = epoch / (7*86400);
    timebuf.tm_wday = (((epoch / 86400) - (weeks_since_epoch * 7)) + 4) % 7;  // 4 = Thursday, which was Jan 1 1970

    return (&timebuf);
}

unsigned long rtc_calculate_last_leapyear(struct tm_leapyear *buf, unsigned long epoch)
{
    unsigned int years = 0;

    if (epoch < EPOCH_AFTER_FIRST_LEAPYEAR) {
        return 0;  // We're not supporting dates before 1973
    }
    unsigned long accum = 0;

    epoch -= EPOCH_AFTER_FIRST_LEAPYEAR;
    accum += EPOCH_AFTER_FIRST_LEAPYEAR;
    years += 3;
    while (epoch >= TOTAL_SECONDS_PER_LEAP_CYCLE) {
        epoch -= TOTAL_SECONDS_PER_LEAP_CYCLE;
        accum += TOTAL_SECONDS_PER_LEAP_CYCLE;
        years += 4;
    }

    if (buf != NULL) {
        buf->startOfCycle = accum;
        buf->remains = epoch;
        buf->years = years;
    }

    return accum;
}

unsigned long rtc_epoch(struct tm *timebuf)
{
    if (timebuf == NULL || timebuf->tm_year < 1973) {
        return 0;
    }
    unsigned long epoch = EPOCH_AFTER_FIRST_LEAPYEAR;  // Starting at 1973
    unsigned int latest_year = 1973, is_leap = 0;

    while ( (timebuf->tm_year - latest_year) >= 4 ) {
        epoch += TOTAL_SECONDS_PER_LEAP_CYCLE;
        latest_year += 4;
    }
    if ( (timebuf->tm_year - latest_year) > 2 ) {
        is_leap = 1;
    }
    while ( (timebuf->tm_year - latest_year) > 0 ) {
        epoch += 86400*365;
        latest_year++;
    }
    if (timebuf->tm_yday > 366) {
        timebuf->tm_yday = 0;
    }
    if (timebuf->tm_yday == 0 && (timebuf->tm_mday > 0 || timebuf->tm_mon > 0)) {
        timebuf->tm_yday = rtc_calculate_yday(timebuf, epoch, is_leap);
    }
    epoch += (unsigned long)timebuf->tm_yday * 86400;
    epoch += (unsigned long)timebuf->tm_hour * 3600;
    epoch += (unsigned long)timebuf->tm_min * 60;
    epoch += timebuf->tm_sec;
    return epoch;
}


unsigned int rtc_calculate_yday(struct tm *timebuf, unsigned long latest_epoch, unsigned int is_leap)
{
    // latest_epoch has us at the beginning of the latest year
    unsigned int month = 0, yday = 0;
    while (month < timebuf->tm_mon) {
        if (month == 1 && is_leap) {
            yday += 29;
        } else {
            yday += monthInfo[month].days;
        }
        month++;
    }
    if (timebuf->tm_mday > 0) {
        yday += timebuf->tm_mday - 1;  // tm_mday starts at 1
    }

    return yday;
}
