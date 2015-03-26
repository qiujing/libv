/********************************************************************
	created:	2012/04/17
	created:	17:4:2012   22:02
	filename: 	f:\øÏ≈Ã\study\lib\lib\utility.h
	file path:	f:\øÏ≈Ã\study\lib\lib
	file base:	utility
	file ext:	h
	author:		qj
	
	purpose:	utility
*********************************************************************/
#ifndef UTILITY_HEADER
#define UTILITY_HEADER
#include <sstream>
void dot_graph(std::stringstream &dot);
void gdl_graph(std::stringstream &dot);
int RVA2OFFSET(int RVA, unsigned char *pBuff);
#endif
