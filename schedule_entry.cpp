#include "schedule_entry.h"
schedule_entry::schedule_entry()
{
 begin = 0;
 end = 0;
 pin =0;
 active =0;
}
schedule_entry::schedule_entry(long x,long y,int z)
{
 begin = x;
 end = x+y;
 pin =z;
 active =0;
}
void schedule_entry::activate()
{
 //digitalWrite(pin,LOW);
  active = 1;
}
int schedule_entry::isconflict(schedule_entry x)
{
return (x.pin==pin && ((begin <= x.begin && x.begin <= end)|| (begin <= x.end && x.end <= end)))?1:0;
}
