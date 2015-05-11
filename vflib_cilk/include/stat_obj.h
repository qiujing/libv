
#ifndef STAT_OBJ_H
#define STAT_OBJ_H

#include <stdio.h>

class StatObj{
 public:
      int dc_count, sc_count, np_count, fp_count, ap_count, bt_count;
      double dc_time, sc_time, np_time, fp_time, ap_time, bt_time;
      StatObj(){
	dc_count = sc_count = np_count = fp_count = ap_count = bt_count = 0;
	dc_time = sc_time = np_time = fp_time = ap_time = bt_time = 0.0;
      };
      void printTime(){
	printf("DC: %d \t %f\n", dc_count, dc_time);
	printf("SC: %d \t %f\n", sc_count, sc_time);
	printf("NP: %d \t %f\n", np_count, np_time);
	printf("FP: %d \t %f\n", fp_count, fp_time);
	printf("AP: %d \t %f\n", ap_count, ap_time);
	printf("BT: %d \t %f\n", bt_count, bt_time);
      };
};

#endif
