

# Notes:
#    Simulator input.runningTime() == Time in ms (and convt to s)
#    Device:  Use C code based on us_ticker_read()/1000 (and handle roll overs)


# State variables:

# Start Point has
year = 0                  # Year of "set date"
timeToSetpoint = 0        # The clock time that was "set"
cpuTimeAtSetpoint = 0     # The CPU time when the clock time was set

#
#  Year Start          Time Date/Time set        CurrentCPUTime
#  |                   | (in s)                  | (in s)
#  V                   V                         V
#  |-------------------+-------------------------|
#                      ^
#                      | 
#                      Known dd/mm/yy hh:mm,.s
#                      AND cpuTimeAtSetpoint (in s)
#   |------------------|-------------------------|
#      timeToSetpoint          deltaTime
#      (in s)                  ( in s)




# Cummulative days of year at the start of each month (non-leap years)
#  (Prefix sum)
# Padded for 1-based indices
cdoy = [0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365]


def isLeapYear(y):
    return (y%400==0 or (y%100!=0 and y%4==0))

def dateToDayOfYear(m, d, y):
    # Assumes a valid date 
    dayOfYear = cdoy[m] + d  
    # Handle after Feb in leap years:
    if m>2 and isLeapYear(y):
        dayOfYear += 1
    return dayOfYear

def dayOfYearToMonthAndDay(d, y):
    # If it's after Feb in a leap year, adjust
    if isLeapYear(y):
        if d==60:
            return (2, 29)
        elif d>60:   # Adjust for leap day
            d -= 1
    for i in range(1,len(cdoy)):  # Adjust for 1-based index
        # If the day lands in (not through) this month, return it
        if d<=cdoy[i+1]:
            return (i, d-cdoy[i])
    return (-1,-1)

def secondsSoFarForYear(d,m,y, hh, mm, ss):
    # ((((Complete Days * 24hrs/day)+complete hours)*60min/hr)+complete minutes)* 60s/min + complete seconds
    return (((dateToDayOfYear(m,d,y)-1)*24 + hh)*60+mm)*60+ss

# Find the date+time (24 hr format) given a cpu time
def timeFor(cpuTime):
    deltaTime = cpuTime - cpuTimeAtSetpoint
    sSinceStartOfYear = timeToSetpoint + deltaTime
    # Find elapsed years by counting up from start year and subtracting off complete years
    y = year
    leap = isLeapYear(y)
    while (not leap and sSinceStartOfYear>365*24*60*60) or (sSinceStartOfYear>366*24*60*60):
        if leap:
            print("Advancing Leap Year")
            sSinceStartOfYear -= 366*24*60*60 
        else:
            print("Advancing NORMAL Year")
            sSinceStartOfYear -= 365*24*60*60 
        y += 1
        leap = isLeapYear(y)

    # sSinceStartOfYear and leap are now for "y", not "year"

    # Find elapsed days
    daysFromStartOfYear = sSinceStartOfYear//(24*60*60)+1  # Offset for 1/1 being day 1
    secondsSinceStartOfDay = sSinceStartOfYear%(24*60*60)

    # Find elapsed hours
    hoursFromStartOfDay = secondsSinceStartOfDay//(60*60)
    secondsSinceStartOfHour = secondsSinceStartOfDay%(60*60)

    # Find elapsed minutes
    minutesFromStartOfHour = secondsSinceStartOfHour//(60)
    # Find elapsed seconds
    secondsSinceStartOfMinute = secondsSinceStartOfHour%(60)

    # Convert days to dd/mm
    (mm, dd) = dayOfYearToMonthAndDay(daysFromStartOfYear, y) # current year, y, not start year
    # TODO: Make this a date/time object
    return (mm, dd, y, hoursFromStartOfDay, minutesFromStartOfHour, secondsSinceStartOfMinute, daysFromStartOfYear)


# Set date at given cpuTime
def setDate(mm,dd, yy, cpuTime):
    (_, _, _, h, m, s, _) = timeFor(cpuTime)    
    global year, cpuTimeAtSetpoint, timeToSetpoint
    year = yy
    cpuTimeAtSetpoint = cpuTime
    timeToSetpoint = secondsSoFarForYear(dd, mm, yy, h, m, s)

# Set time at given cpuTime
def setTime(h,m, s, cpuTime):
    (mm, dd, yy, _, _, _, _) = timeFor(cpuTime)    
    global year, cpuTimeAtSetpoint, timeToSetpoint
    cpuTimeAtSetpoint = cpuTime
    timeToSetpoint = secondsSoFarForYear(dd, mm, yy, h, m, s)

def dayOfWeek(m, d, y):
    # f = k + [(13*m-1)/5] + D + [D/4] + [C/4] - 2*C.
    # Zeller's Rule from http://mathforum.org/dr.math/faq/faq.calendar.html
    D = y%100
    C = y//100 
    # Use integer division
    return d + (13*m-1)//5 + D + D//4 + C//4 - 2*C


def advanceTime(amount, unit):
    units = [1, 60*1, 60*60*1, 24*60*60*1]
    global year, cpuTimeAtSetpoint, timeToSetpoint
    cpuTimeAtSetpoint -= amount*units[unit]
