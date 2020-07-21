#include <ctime>
#include <stdlib.h>

int randomgen(int min, int max)
{
    int range = max-min+1 ;
    return rand()%range + min ;
}

int Time_second()
{
   time_t now = time(0);
   tm *ltm = localtime(&now);

   return ltm->tm_sec;
}

int Time_minute()
{
   time_t now = time(0);
   tm *ltm = localtime(&now);

   return ltm->tm_min;
}

int Time_hour()
{
   time_t now = time(0);
   tm *ltm = localtime(&now);

   return ltm->tm_hour;
}

u_int16_t constrain(u_int16_t x, u_int16_t a, u_int16_t b)
{
    if (x < a) {
        return a;
    }

    if (x > b) {
        return b;
    }

    return x;
}
