#ifndef SOFTRTC_H
#define SOFTRTC_H

#include<time.h>

void updateCurrentTime();
void setTimeFrom24hour(int hour, int minute, int sec);
void setDate(int year, int month, int day);
void setTime(int hour, int minute, int sec, bool isAm);
void advanceTimeDateBy(int amount, int units);
int getYear();
int getMonth();
int getDay();
int getWeekday();
int getHour24hour();
int getDayOfYear();
int getHour();
bool isAm();
int getMinute();
int getSecond();


#endif 