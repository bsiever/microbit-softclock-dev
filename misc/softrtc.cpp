#include "softrtc.h"
#include "Microbit.h"
#include "MicroBitSystemTimer.h"

// TODO: Microbit PXT changes
//#include "pxt.h"
//using namespace pxt;



//  Accurate counter is the vital part of this!!!

// I'll do time storage / calc different and push much more into type script once we have the algorithm (which I have....)


// Static variable(s) for "zero time"
static time_t timeAtSetpoint = 0;  // Unix time when the time was set
static uint64_t setpoint = 0;      // This CPU's clock count when the time was set

// The last read time
static struct tm currentTime = {};



/* 
  Return the current system time in ms 
*/
static time_t systemTime() {
    static uint32_t upperTimeUS = 0;    
    static uint32_t lastTimeUS = 0;
    uint32_t thisTimeUS = us_ticker_read();
    
    // Check for rollover
    if(thisTimeUS<lastTimeUS) {
        upperTimeUS++;
    }
    lastTimeUS = thisTimeUS;

    uint64_t timeUS = ((uint64_t)upperTimeUS)<<32 | ((uint64_t)thisTimeUS);
    return timeUS/1000000;
}


/*
  Perform periodic update of clock. 
  
  It must be called frequently enough for both call-backs (minute-by-minute) and 
  to handle overflows in the system clock (every 2^32 uS = 4294 S = 1.2 hours) 
*/ 
static void periodicUpdate() {
    while(true) {
        updateCurrentTime();
        fiber_sleep(5*1000);  // Sleep 5 seconds
    }
}


/* 
   Ensure the periodic task is running to do updates / callbacks 
*/
static void ensureUpdates() {
    static bool running = false; 
    if(running == false) {
        (void)invoke(periodicUpdate); // create fiber and schedule it.
        running = true;
    }
}


void updateCurrentTime() {
    time_t delta_s = systemTime() - setpoint;
    
    time_t now = timeAtSetpoint + delta_s; 

    struct tm newTime;
    gmtime_r(&now, &newTime);
    // // Check for changed and do callbacks
    // if(newTime.tm_hour!=currentTime.tm_hour) {

    // } 
    // if(newTime.tm_min!=currentTime.tm_min) {

    // } 
    // if(newTime.tm_mday!=currentTime.tm_mday) {
    // }
    currentTime = newTime;
}

/*
  Set the time from a 24-hour format (positive values only;  Modular arithmetic ensures valid range)
*/ 
void setTimeFrom24hour(int hour, int minute, int sec) {
    // Since the clock will be used, ensure it's updating
    ensureUpdates();

    // Ensure values in appropriate ranges
    hour = hour%24;
    minute = minute%60;
    sec = sec%60;

    // Get the "now" time
    updateCurrentTime();
    // Change the hours and use the updated version as the new time at the setpoint
    currentTime.tm_hour = hour;
    currentTime.tm_min = minute;
    currentTime.tm_sec = sec;
    timeAtSetpoint = mktime(&currentTime);

    // Set the setpoint
    setpoint = systemTime();
}


/*
  Set date from traditional calendar representation, like 2020-03-20 for March 20, 2020
*/
void setDate(int year, int month, int day) {
    ensureUpdates();

    updateCurrentTime();              // Update to the current time
    currentTime.tm_year = year-1900;  // Years since 1900
    currentTime.tm_mon = month-1;     // Months in 0-11 (adjust calendar rep to tm struct rep)
    currentTime.tm_mday = day; 
    timeAtSetpoint = mktime(&currentTime);
    setpoint = systemTime();
}



void setTime(int hour, int minute, int sec, bool isAm) {
    ensureUpdates();

    hour = hour%13;
    minute = minute%60;
    sec = sec%60;

    // Adjust to 24-hour time format
    if(isAm && hour == 12)   // 12am -> 0 hundred hours
        hour = 0;
    else if(hour<12)         // PMs other than 12 get shifted after 12:00 hours
            hour = hour+12;  

    setTimeFrom24hour(hour, minute, sec);
}




void advanceTimeDateBy(int amount, int units) {
    time_t factor = 0;

    switch(units) {
        case 0: // ms
            factor = 1;
        break;
        case 1: // S
            factor = 1000*1;
        break;
        case 2: // Min
            factor = 60*1000;
        break;
        case 3: // Hours 
            factor = 60*60*1000;
        break;
        case 4: // Days
            factor = 24*60*60*1000;
        break;
    }
    timeAtSetpoint += amount*factor; 
}




int getYear() {
    return currentTime.tm_year+1900;
}
int getMonth() {
    return currentTime.tm_mon+1;
}
int getDay() {
    return currentTime.tm_mday;
}

int getWeekday() {
    return currentTime.tm_wday;
}

int getHour24hour() {
    return currentTime.tm_hour;
}

int getDayOfYear() {
    return currentTime.tm_yday;
}

int getHour() {
    int hour = currentTime.tm_hour;
    if(hour == 0)  // 12 am
        return 12;
    if(hour > 12)  
        return hour-12;
    return hour;
}

bool isAm() {
    return currentTime.tm_hour<12;
}

int getMinute() {
    return currentTime.tm_min;
}

int getSecond() {
    return currentTime.tm_sec;
}




/*
Blocks initial prototypes: 

enum MornNight {
    //% block="am"
    AM,
    //% block="pm"
    PM
}

enum TimeUnit {
    //% block="ms"
    Milliseconds,
    //% block="Seconds"
    Seconds, 
    //% block="Minutes"
    Minutes, 
    //% block="Hours"
    Hours, 
    //% block="Days"
    Days
}

enum TimeFormat {
    //% block="with am / pm"
    AMPM,
    //% block="as 24-hr"
    HHMM24hr
}

enum DateFormat {
    //% block="as Day/Month"
    DM,
    //% block="as Day/Month/Year"
    DMYYYY,
    //% block="as YEAR-MONTH-DAY"
    YYYY_MM_DD
}

// Fontawesome Unicode maybe: "&#xf017;"
//% color="#AA278D"  icon="\u23f0" 
namespace timeAndDate {


    //% block="set time from 24-hour time |  %hour | : %minute | . %second"
    //% hour.min=0 hour.max=23
    //% minute.min=0 minute.max=59
    //% second.min=0 second.max=59
    export function set24HourTime(hour: number, minute: number, second: number) {

    }
    //% block="set time to |  %hour | : %minute | . %second | %ampm"
    //% hour.min=0 hour.max=23
    //% minute.min=0 minute.max=59
    //% second.min=0 second.max=59
    //% inlineInputMode=inline
    export function setTime(hour: number, minute: number, second: number, ampm: MornNight) {

    }


    //% block="set date to | Day %day | / Month %month | / Year %year"
    //% day.min=0 day.max=31
    //% month.min=1 month.max=12
    //% year.min=2020 year.max=2050
    export function setDate(day: number, month: number, year: number) {

    }

    // //% block="time in %tformat format | and date in %dformat format"
    // export function timeAndDate(tformat: TimeFormat, dformat: DateFormat) : String {
    //    return ""
    // }

    // // Format:  "YYYY-MM-DD HH:MM" in 24-hour format
    // //% block="time stamp"
    // export function timestamp() : String {
    //    return ""
    // }


    // This can cause overflow or underflow (adding 1 minute could change the hour)
    // Add or subtract time with the given unit. 
    //% block="advance time/date by | %amount | %unit "
    export function advanceBy(amount: number, unit: TimeUnit) {

    }


    //% block="current time as numbers $hour:$minute.$second on $weekday, $day/$month/$year, $dayOfYear" advanced=true
    //% draggableParameters=variable
    //% handlerStatement=1
    export function numericTime(handler: (hour: number, minute: number, second: number, weekday: number, day: number, month: number, year: number, dayOfYear: number) => void) {

    }


    //% block="current time $format"
    export function time(format: TimeFormat) : string {
        return ""
    }

    //% block="current date formatted $format"
    export function date(format: DateFormat) : string {
        return ""
    }

    //% block="date and time"
    export function dateTime() : string {
        return ""
    }
    // //% block="current time formatted with $tformat | date formatted like $dformat"
    // //% draggableParameters=variable
    // //% handlerStatement=1
    // export function stringTime(tformat: TimeFormat, weekday: string, dformat: DateFormat, handler: (time: string, date: string, weekday: string) => void) {

    // }


    //% block="minute changed" advanced=true
    export function onMinuteChanged(handler: () => void) {

    }

    // //% block="second changed" advanced=true
    // export function onSecondChanged(handler: () => void) {

    // }
    //% block="hour changed" advanced=true
    export function onHourChanged(handler: () => void) {

    }
    //% block="day changed" advanced=true
    export function onDayChanged(handler: () => void) {

    }


    //  Advanced: 
    
    // Get time;  time->day/month/year/hours(24)/hours/amPM string / minutes/seconds/weekday/ daystring w/format  / year string w/format
}
*/