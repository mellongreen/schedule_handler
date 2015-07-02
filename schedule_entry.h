#ifndef schedule_entry_h
#define schedule_entry_h

class schedule_entry
{
public:
	schedule_entry();
	schedule_entry(long x,long y,int z);
	void activate();
        int isconflict(schedule_entry x);      
	int pin;
	long begin; 
	long end;
	int active;
};
#endif
