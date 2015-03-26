#pragma once
class CLock
{
	CRITICAL_SECTION& cs;  
public:
	CLock::CLock(CRITICAL_SECTION& lock);
	CLock(void);
	~CLock(void);
};

