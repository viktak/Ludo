#ifndef NTP_H
#define NTP_H

#define NTP_UPDATE_INTERVAL_MS 360000 //  synchronize time with NTP server once an hour

void setupNTP();
void loopNTP();

#endif
