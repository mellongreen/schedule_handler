/*
schedule entry
Copyright (C) 2015 Mellon Green

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
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
