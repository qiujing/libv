#ifndef _BIT_SET_H
#define _BIT_SET_H

#define BITS_PER_CHAR 8
class CBitSet
{
public:
	CBitSet(int bitsCount);
	~CBitSet();
	int size();
	int count();
	void reset(int pos);
	void reset_all();
	void set(int pos);
	void set_all();
	int isset(int pos);
	void print();
	int any();
	bool equal(CBitSet* bs);
	int getByteLength();
	char* getBuffer();
	bool operator[](unsigned int index);
private:
	char* _mblock;
	int _len; //the number of bits specified by users
	int _len2;//the number of bytes in the _mblock
	int _count;	//the number of bits that is set to 1 in the _mblock
};

typedef CBitSet* PCBitSet;
#endif
