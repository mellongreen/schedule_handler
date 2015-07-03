/*
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pwurein 2
 * LCD R/W pin to ground
 * LCD 15 to +5V
 * LCD 16 to ground
 * LCD 3 to pot 
*/
#include <Time.h>
#include <stdlib.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <string.h>
#include "StopWatch.h"
#include "schedule_entry.h"
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>     /* strtol */
#include <assert.h>
#include <SoftwareSerial.h>
#include "LinkedList.h"

#define _XOPEN_SOURCE
#define DS3231_I2C_ADDRESS 0x68
#define LCDCOLUMNS 16
#define LCDROWS 2
#define BUFFSIZE 128
#define TIMEOUT 500
#define PORT1PIN 7
#define PORT2PIN 8
#define YEAROFFSET 2000

//timezone offset
#define OFFSETHOUR -8
#define OFFSETMIN 0

//dst offset to be added to local time
#define DSTOFFSETHOUR 1
#define DSTOFFSETMIN 0

//ordinal of the dayofweek; 2 means the second
#define DSTBEGINORDINAL 2
//0 means sunday; value should be 0 (SUN) to 6 (SAT)
#define DSTBEGINDAYOFWEEK 0
//in 24-hr sys, all in localtime
#define DSTBEGINHOUR 2
#define DSTBEGINMIN 0

#define DSTENDORDINAL 1
#define DSTENDDAYOFWEEK 0
#define DSTENDHOUR 1
#define DSTENDMIN 0


char cmdbuffer[BUFFSIZE];
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int curcol,currow,count;
StopWatch stopwatch;
char space[LCDCOLUMNS+1];
char offsethour,offsetmin;
byte monthdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
//std::list<int> schedule;
LinkedList<schedule_entry> schedule;

PROGMEM const uint32_t crc_table[16] = 
{
   0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
   0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
   0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
   0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};
unsigned long crc_update(unsigned long crc, byte data)
{
   byte tbl_idx;
   tbl_idx = crc ^ (data >> (0 * 4));
   crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
   tbl_idx = crc ^ (data >> (1 * 4));
   crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
   return crc;
}
unsigned long crc_string(char *s)
{
 unsigned long crc = ~0L;
 while (*s!='\0')
   crc = crc_update(crc, *s++);
 crc = ~crc;
 
 return crc;
}
/*void lcdprint(String msg)
{
//  Serial.begin(115200);
  lcd.setCursor(curcol,currow);
  lcd.print(msg);
}*/
void newline()
{
  curcol=0;
  currow=(currow+1)%LCDROWS;
  lcd.setCursor(curcol,currow);
}
void setup() 
{
  offsethour=OFFSETHOUR;
  offsetmin=OFFSETMIN;
  Wire.begin();
  Serial.begin(14400);

  pinMode(PORT1PIN,OUTPUT);
  digitalWrite(PORT1PIN,HIGH);
  pinMode(PORT2PIN,OUTPUT);
  digitalWrite(PORT2PIN,HIGH);

  lcd.begin(LCDCOLUMNS, LCDROWS);
  curcol = 0;
  currow = 0;
  int i=0;
  for(;i++;i<LCDCOLUMNS)
  {
  space[i]=' ';
  }
  space[i]='\0';
}

int getcmd()
{
  count=0;
  int term=0;
  unsigned long since;
  int firstbyte=1;
  stopwatch.reset();
  while(!term)
  {
    if(!Serial.available())
    {
      since = stopwatch.elapsed();
      if(since>TIMEOUT)
      {
        return 0;  
      } 
      continue;
    }
    if(firstbyte)
    {
    stopwatch.start();
    firstbyte = 0;  
    } 
    if(count>=BUFFSIZE)
    {
     stopwatch.reset();
     delay(TIMEOUT);
     while(Serial.available())Serial.read(); 
     return 0;
    }
    char x = Serial.read();
    if(x=='\n')term=1;
    else
    {
    cmdbuffer[count] = x;
    count++;
    }
    stopwatch.reset();
    stopwatch.start();
  }
  stopwatch.stop();
  stopwatch.reset();
  cmdbuffer[count]='\0';
  return 1;
}
int isleapyear(long x)
{
if (x % 100 == 0) 
  return x%400==0?1:0;
else 
  return x%4==0?1:0;
}
long gettime()
{
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  
   long currentTime = second + minute*60L + hour*3600L;
    long dayaccum =0;
  
    dayaccum += (dayOfMonth-1);
   int curyear = (int)year+YEAROFFSET;
   
   for(int i=1;i<month;i++)
   {
     dayaccum +=monthdays[i-1];
     if(i==2 && isleapyear(curyear))dayaccum++;
   }
//   Serial.println(curyear);
   //currentTime += dayaccum*86400L;
   for(int i=1970;i<curyear;i++)
   {
     if(isleapyear(i))dayaccum+=366;
     else dayaccum+=365;
   }
   //Serial.println(dayaccum);
   //dayaccum++;
   currentTime+=(dayaccum*86400L);
   return currentTime;
 }
int processschedule()
{
  schedule_entry iter;
  long currentTime = gettime(); 
//   Serial.println(currentTime);
//   delay(1000);
//Serial.println(currentTime);
//delay(1000);
   for(int i=0;i<schedule.size();i++)
   {
     iter = schedule.get(i);
     /*Serial.println("#######");
     Serial.println(iter.active);
     Serial.println(currentTime);
     Serial.println(iter.begin);
     delay(1000);
     */
     //if not active and overdue, activate
     if(!iter.active && iter.begin <= currentTime)
     {
       iter.activate();
       schedule.set(i,iter);
       digitalWrite(iter.pin,LOW);
     }//if active and over, pop
     else if(iter.active && iter.end <=currentTime)
     {
       digitalWrite(iter.pin,HIGH);
       schedule.remove(i);
       i--;
     }
   }
   int actcount=0;
   for(int i=0;i<schedule.size();i++)
   {
     iter = schedule.get(i);
     if(iter.active)actcount++;
   }
   return actcount;
}
void loop() 
{
  count=0;
//  lcd.setCursor(0,currow);
 // lcd.print("                ");
  //displayTime();

//busy wait while serial is not available
 while(!Serial.available())
 {
   int actcount = processschedule();
   for(int i=0;i<LCDROWS;i++)
   {
    lcd.setCursor(0,i);
    lcd.print(space);
   }
    currow =0;
    curcol =0;
    lcd.home();
    displayTime();
  lcd.print(",");
  lcd.print(actcount,DEC);
  lcd.print(" IP");
  
  //lcd.setCursor(0,currow);
    //delay(20);
 }
//get cmd in TIMEOUT or less
 if(!getcmd())
 {
  count=0;
  return;
 }
 else
 {
   //if get cmd then?
process();
   /* if(!strcmp("leek",cmdbuffer))
   {
   digitalWrite(13,LOW);
   delay(1000);
   digitalWrite(13,HIGH);
   
   }*/
 }
}
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
void adjustTimeZone
(
byte *inminute,
byte *inhour,
byte *indayOfWeek,
byte *indayOfMonth,
byte *inmonth,
byte *inyear,
char offmin,
char offhour)
{
  	char minute = *inminute;
	char hour = *inhour;
	char dayOfWeek = *indayOfWeek;
	char dayOfMonth = *indayOfMonth;
	char month = *inmonth;
	char year = *inyear;
  minute = minute+offmin;
  if(minute<0)
  {
  minute+=60;
  hour--;
  }
  else if (minute>=60)
  {
  minute = minute%60;
  hour++;
  }
  hour =hour+offhour;
  if(hour<0)
  {
  hour=hour+24;
  dayOfMonth =dayOfMonth-1;
  if(dayOfWeek==0)dayOfWeek=6;
  else dayOfWeek=dayOfWeek-1;
    if(dayOfMonth<=0)
    {
      month =month-1;
      if(month<=0)
      {
        year=year-1;
        month=month+12;
      }
      dayOfMonth = monthdays[month-1];
      if(year%4==0 && month==2)dayOfMonth=dayOfMonth+1;
    }
  }
  else if(hour>=24)
  {
    hour=hour%24;
    dayOfMonth +=1;
    if(dayOfWeek==6)dayOfWeek=0;
    else dayOfWeek+=1;
    
      if(dayOfMonth>monthdays[month-1])
      {
        month +=1;
        if(month>12)
        {
          year+=1;
          month=1;
        }
        dayOfMonth = 1;
      }
  }
  *inminute = minute;
  *inhour=hour;
  *indayOfWeek=dayOfWeek;
  *indayOfMonth=dayOfMonth;
  *inmonth=month;
  *inyear=year;
}
int isindst(byte month, byte dayOfWeek,byte dayOfMonth,byte hour,byte minute)
{
if(3<=month && month<=11)
{
  if(month==3)
  {
    byte dstbegindate = getdate(dayOfMonth,dayOfWeek,DSTBEGINORDINAL,DSTBEGINDAYOFWEEK);
    if(dstbegindate<dayOfMonth)return 1;
    else if (dstbegindate==dayOfMonth)
    {
      if(DSTBEGINHOUR<hour)return 1;
      else if(DSTBEGINHOUR==hour) return DSTBEGINMIN<=minute?1:0;
      else return 0;
    }
    else return 0;
  }
  else if (month==11)
  {
     byte dstenddate = getdate(dayOfMonth,dayOfWeek,DSTENDORDINAL,DSTENDDAYOFWEEK);
    if(dayOfMonth<dstenddate)return 1;
    else if (dstenddate==dayOfMonth)
    {
      /*if(DSTBEGINHOUR<hour)return 1;
      else if(DSTBEGINHOUR==hour) return DSTBEGINMIN<=minute?1:0;
      else return 0;*/
      if(hour<DSTENDHOUR) return 1;
      else if (hour == DSTENDHOUR) return minute<DSTENDMIN?1:0;
      else return 0;      
    }
    else return 0;
  }
  else return 1;
}
return 0;
}

byte getdate(byte curdayOfMonth,byte curdayOfWeek,byte ordinal,byte targetdayOfWeek)
{
while(curdayOfMonth!=1)
{
curdayOfMonth--;
if(curdayOfWeek==0)curdayOfWeek=6;
else curdayOfWeek--;
}
byte count =0;
while(count!=ordinal)
{
  if(curdayOfWeek==targetdayOfWeek)count++;
  curdayOfMonth++;
  if(curdayOfWeek==6)curdayOfWeek=0;
  else curdayOfWeek++;
}
return curdayOfMonth-1;
}

void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
 // send it to the serial monitor
 // Serial.print(hour, DEC);
 adjustTimeZone(&minute,&hour,&dayOfWeek,&dayOfMonth,&month,&year,offsetmin,offsethour);
//  Serial.println(dayOfWeek);
if(isindst(month,dayOfWeek,dayOfMonth,hour,minute))
   adjustTimeZone(&minute,&hour,&dayOfWeek,&dayOfMonth,&month,&year,DSTOFFSETMIN,DSTOFFSETHOUR);
  
  lcd.print(hour,DEC);
  lcd.print(":");
  
  // convert the byte variable to a decimal number when displayed
  //Serial.print(":");
  if (minute<10)
  {
    lcd.print("0");
  }
  lcd.print(minute, DEC);
  //lcd.print(":");
  //if (second<10)
  //{
  //  lcd.print("0");
  //}
  //lcd.print(second, DEC);
  lcd.print(" ");
  lcd.print(dayOfMonth, DEC);
  lcd.print("/");
  lcd.print(month, DEC);
  lcd.print("/");
  lcd.print(year, DEC);
//  currow++;
  //lcd.setCursor(0,currow);
  newline();
  lcd.print(schedule.size(),DEC);
  lcd.print(" schedule");  
  //lcd.print(" Day of week: ");
 /* switch(dayOfWeek){
  case 1:
    lcd.print("S");
    break;
  case 2:
    lcd.print("M");
    break;
  case 3:
    lcd.print("T");
    break;
  case 4:
    lcd.print("W");
    break;
  case 5:
    lcd.print("R");
    break;
  case 6:
    lcd.print("F");
    break;
  case 7:
    lcd.print("Saturday");
    break;
  }*/
}

void complain()
{
  writeWithHash("nak;");   
}
//experimental
int makeschedule(int pin,long length,long beg)
{
  schedule_entry current(beg,length,pin); 
  schedule_entry iter;
  for(int i=0;i<schedule.size();i++)
  {
    iter = schedule.get(i);
    if(iter.isconflict(current))return 0;
  }
  schedule.add(current);
  return 1;
}
int getpin(int port)
{
  switch(port)
  {
  case 1:
    return PORT1PIN;
  case 2:
    return PORT2PIN;
  default:
    return 0;
  }
}
int getport(int pin)
{
  switch(pin)
  {
  case PORT1PIN:
    return 1;
  case PORT2PIN:
    return 2;
  default:
    return 0;
  }
}

void process()
{
  char response[BUFFSIZE*4];
  if(iscorrupt(cmdbuffer))
  {
   complain();   
  }
  else
  {
    char** tokens;
    tokens = str_split(cmdbuffer+8, ';');
    int i;
    if (tokens)
    {
        if(!strcmp(*(tokens),"syn"))
        {
            if(!synchronize((*(tokens + 2)+strlen("systime")),*(tokens + 1)))complain();       
        }
        else if(!strcmp(*(tokens),"sch"))
        {
          //Serial.write("OK");
          int port;
          long length,beg;
          for (i = 0; *(tokens + i + 1); i++)
          {
            processtoken(*(tokens + i),port,length,beg);
          }
          //here
          if(makeschedule(getpin(port),length,beg))
          {
           sprintf( response, "ack;Schedule entry added. *%ld* for %ld minute at port %d;%s;",beg,length/60,port,*(tokens + i) );
           writeWithHash(response);
          }
          else
          {
           sprintf( response, "nak;Schedule entry not added. Schedule conflict. *%ld* for %ld minute at port %d;%s;",beg,length/60,port,*(tokens + i) );
           writeWithHash(response);
          }
         }
        else if(!strcmp(*(tokens),"shw"))
        {
         sprintf( response, "ack;Showing schedule.\\");
         getschedule(response);
         response[strlen(response)-1]= ';';
         sprintf( response+strlen(response),"%s;", *(tokens+1));
         writeWithHash(response);          
        }
        else
        {
          complain();
        }
        
    }
    for (i = 0; *(tokens + i); i++)free(*(tokens + i));
    free(tokens);
  }
}
void getschedule(char* buffer)
{
  schedule_entry iter;
  for(int i=0;i<schedule.size();i++)
  {
    iter = schedule.get(i);
    sprintf(buffer+strlen(buffer),"*%ld* for %ld minute at port %d\\",iter.begin,(iter.end-iter.begin)/60,getport(iter.pin));
  }
}
void processtoken(char* token,int& port,long& length,long& beg)
{
  if(beginsWith(token,"port"))
  {
    port = atoi(token+strlen("port"));
  }
  else if(beginsWith(token,"length"))
  {
    length = atol(token+strlen("length"));
  }
  else if(beginsWith(token,"begin"))
  {
    beg = atol(token+strlen("begin"));
  }
}
int beginsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}
void calcDate(unsigned long seconds, byte *clock_second,
byte *clock_minute,
byte *clock_hour,
byte *clock_dayOfWeek,
byte *clock_dayOfMonth,
byte *clock_month,
byte *clock_year)
{
  uint32_t minutes, hours, days, year, month;
  uint32_t dayOfWeek;
 
  /* calculate minutes */
  minutes  = seconds / 60;
  seconds -= minutes * 60;
  /* calculate hours */
  hours    = minutes / 60;
  minutes -= hours   * 60;
  /* calculate days */
  days     = hours   / 24;
  hours   -= days    * 24;

  /* Unix time starts in 1970 on a Thursday */
  year      = 1970;
  dayOfWeek = 4;

  while(1)
  {
    bool     leapYear   = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    uint16_t daysInYear = leapYear ? 366 : 365;
    if (days >= daysInYear)
    {
      dayOfWeek += leapYear ? 2 : 1;
      days      -= daysInYear;
      if (dayOfWeek >= 7)
        dayOfWeek -= 7;
      ++year;
    }
    else
    {
      //*clock_dayOfMonth = days;
      dayOfWeek  += days;
      dayOfWeek  %= 7;

      /* calculate the month and day */
      static const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      for(month = 0; month < 12; ++month)
      {
        uint8_t dim = daysInMonth[month];

        /* add a day to feburary if this is a leap year */
        if (month == 1 && leapYear)
          ++dim;

        if (days >= dim)
          days -= dim;
        else
          break;
      }
      break;
    }
  }

  *clock_second  = seconds;
  *clock_minute  = minutes;
  *clock_hour = hours;
  *clock_dayOfMonth = days + 1;
  *clock_month  = month+1;
  *clock_year = year%100;
  *clock_dayOfWeek = dayOfWeek;
}
int synchronize(char* token,char* ts)
{
  char response[80];
  
  char* t;
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  unsigned long systime = strtoul(token,&t,10);
//Serial.println(token);
//Serial.println(systime);
  if(token==t)return 0;
  calcDate(systime,&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

//Serial.println(dayOfWeek);
//Serial.println(month);
//Serial.println(dayOfMonth);
//
setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
schedule.clear();
  sprintf( response, "ack;Unit synced. All schedules removed.;" );
  sprintf(response + strlen(response),ts);
  sprintf(response + strlen(response),";");
  
  writeWithHash(response);
return 1;
//  Serial.print(systime);
  //Serial.write(token);
  //Serial.write('\n'); 
}


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t county     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            county++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    county += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    county++;

    result = (char**)malloc(sizeof(char*) * county);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < county);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == county - 1);
        *(result + idx) = 0;
    }

    return result;
}
int iscorrupt(char* msg)
{
if(count<=10) return 1;
char buffer[9];
memcpy(buffer,msg,8);
buffer[8]='\0';

char* t;
unsigned long hash = strtoul(buffer,&t,16);
if(t==buffer)return 1;

if(hash != crc_string(msg+8))return 1;
return 0;
}

void writeWithHash(char* msg)
{
  char buffer[9];
  sprintf(buffer, "%08lx", crc_string(msg) & 0xFFFFFFFFL);
  Serial.write(buffer);
  Serial.write(msg);
  Serial.write('\n');
  Serial.flush();
}

