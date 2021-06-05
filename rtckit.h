/**
  * MSP430 Real Time Clock Kit
  *
  * This is a library for managing Clock Date & Time using a simple 1-second clock tick,
  * the C library's built-in <time.h> and struct tm - storing date & time information,
  * and converting between Epoch time format (unsigned long integer as # seconds since
  * the Epoch, January 1 1970 12:00AM UTC).
  *
  * @author Eric Brundick <spirilis at linux dot com>
  * @file rtckit.h
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

#ifndef RTCKIT_H
#define RTCKIT_H
#include <time.h>

/// User configuration YOU MAY MODIFY THESE

#define RTCKIT_LIBRARY_PROVIDES_ISR 1
#define RTCKIT_STORE_VARIABLES_IN_SECTION ".infoA"

/// End of User configuration


/// Public data variables - can be referenced inside your code

/// RTC_GENERAL_ERROR should only get set in rtc_init() if hardware init fails
///   due to compatibility problems
#define RTC_GENERAL_ERROR 0x8000

/// RTC_TICK bitfield inside rtc_status indicates RTC_ISR has fired
#define RTC_TICK 0x0001

/// User setting RTC_TICK_DOES_WAKEUP tells the ISR to wake up the chip
/// for every 1-second tick, clearing this avoids that (unless an alarm triggers)
#define RTC_TICK_DOES_WAKEUP 0x0100

/// RTCALARM_0_TRIGGERED bitfield inside rtc_status indicates Alarm#0 has triggered
#define RTCALARM_0_TRIGGERED 0x0002
/// RTCALARM_1_TRIGGERED bitfield inside rtc_status indicates Alarm#1 has triggered
#define RTCALARM_1_TRIGGERED 0x0004
/// rtc_status is a user-testable variable - see rtckit.h for bitfields
extern volatile unsigned int rtc_status;

extern volatile unsigned long rtcepoch;  ///< The current timestamp; RTC_ISR increments this.
/// The epoch timestamp at which Alarm#0 will trigger.  0 disables this alarm.
extern volatile unsigned long rtcalarm0;
/// The epoch timestamp at which Alarm#1 will trigger.  0 disables this alarm.
extern volatile unsigned long rtcalarm1;

/** If rtcalarm0 is reached, rtc_status will reflect RTCALARM_0_TRIGGERED and
 *  if this variable's value is > 0, rtcalarm0 will automatically increment by this
 *  amount inside RTC_ISR so re-setting the alarm after each trigger is not necessary.
 */
extern volatile unsigned long rtcalarm0_incr;

/** If rtcalarm1 is reached, rtc_status will reflect RTCALARM_1_TRIGGERED and
 *  if this variable's value is > 0, rtcalarm1 will automatically increment by this
 *  amount inside RTC_ISR so re-setting the alarm after each trigger is not necessary.
 */
extern volatile unsigned long rtcalarm1_incr;


/// Data structure indicating day-of-week indices, shortnames and long names
struct rtcDayInfo
{
    char shortName[4];
    char longName[10];
};

/// Data structure listing months and # of days in each month
struct rtcMonthInfo
{
    unsigned char days;    /* # of Days in month */
    char    shortName[4];
    char    longName[10];
};

/// A predefined instance of s_MonthInfo for general use
extern const struct rtcMonthInfo monthInfo[];

/// A predefined instance of s_DayInfo for general use
extern const struct rtcDayInfo dayInfo[];

/** User functions
 */

/** Initialize MSP430 RTC peripheral
 *
 *  @param[in] A parameter (see below) corresponding to which clock source
 *             is to be used for the RTC peripheral.
 */
void rtc_init(unsigned int rtc_clock_source);

#define RTC_CLOCK_XT1CLK    RTCSS__XT1CLK
#define RTC_CLOCK_SMCLK     RTCSS__SMCLK
#define RTC_CLOCK_VLOCLK    RTCSS__VLOCLK

/** Convert RTC Epoch seconds into a struct tm
 *
 * @param[in] Epoch time in seconds since the epoch (Jan 1 1970 midnight UTC)
 * @param[out] Time/Datestamp in "struct tm" format - Note a static buffer is used here,
               be sure to copy or use contents before executing this function again.
 */
struct tm * rtc_interpret(unsigned long);

/** Convert struct tm time structure into Epoch seconds
 *
 * @param[in] A pointer to a "struct tm" with tm_year, tm_mon, tm_mday, tm_hour, tm_min,
 *            tm_sec filled out - optionally tm_mon/tm_mday may be omitted if tm_yday is supplied.
 * @param[out] Timestamp in epoch - seconds since Jan 1 1970 midnight UTC
 */
unsigned long rtc_epoch(struct tm *);

/// Internal defines used by the library
#define TOTAL_SECONDS_PER_LEAP_CYCLE ((86400*365)*3 + (86400*366))
#define EPOCH_AFTER_FIRST_LEAPYEAR ((86400*365)*2 + (86400*366))


#endif /* RTCKIT_H */
