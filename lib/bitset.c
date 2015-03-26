/**
 * File: bitset.c
 * Purpose: implementation of bitset in C
 * Author: puresky
 * Date: 2011/05/03
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitset.h"
#include <assert.h>

static const unsigned char mask1[8] = 
{ 
	0x01, /* 00000001 */
	0x02, /* 00000010 */
	0x04, /* 00000100 */
	0x08, /* 00001000 */
	0x10, /* 00010000 */
	0x20, /* 00100000 */
	0x40, /* 01000000 */
	0x80  /* 10000000 */
};

static const unsigned char mask2[8] =
{
	0xFE, /* 11111110 */
	0xFD, /* 11111101 */
	0xFB, /* 11111011 */
	0xF7, /* 11110111 */
	0xEF, /* 11101111 */
	0xDF, /* 11011111 */
	0xBF, /* 10111111 */
	0x7F  /* 01111111 */
};

BitSet *bitset_new(int bitsCount)
{
	BitSet *bs = (BitSet *)malloc(sizeof(BitSet));
	assert(bs!=NULL);
	bs->_count = 0;
	bs->_len = bitsCount;
	// the real length of _mblock is (bitsCount / BITS_PER_CHAR + 1) in Byte 
	bs->_len2 = bs->_len / BITS_PER_CHAR + 1;
	bs->_mblock = (char *)malloc(sizeof(char) * bs->_len2);
	assert(bs->_mblock!=NULL);
	memset(bs->_mblock, 0, sizeof(char) * bs->_len2);

	return bs;
}

BitSet *bitset_new2(const char *bits)
{
	int i, len;
	BitSet *bs;
	len = strlen(bits);
	bs = bitset_new(len);
	for (i = 0; i < len; ++i)
		if (bits[i] == '1')
			bitset_reset(bs, i);
	return bs;
}

void bitset_free(BitSet *bs)
{
	if (bs)
	{
		free(bs->_mblock);
		free(bs);
	}
}

int bitset_size(BitSet *bs)
{
	return bs->_len;
}

int bitset_count(BitSet *bs)
{
	return bs->_count;
}

void bitset_reset_all(BitSet *bs)
{
	memset(bs->_mblock, 0x00, bs->_len2*sizeof(char));
	bs->_count = 0;
}

void bitset_reset(BitSet *bs, int pos)
{
	int i, j;

	if (pos >= bs->_len)
	{
		fprintf(stderr, "bit postion :%d is invalid!\n", pos);
		return;
	}

	i = pos / BITS_PER_CHAR;
	j = pos - i * BITS_PER_CHAR;
	if (bitset_isset(bs, pos))
	{
		bs->_mblock[i] &= mask2[j];
		bs->_count--;
	}

}

void bitset_set_all(BitSet *bs)
{
	memset(bs->_mblock, 0xFF, bs->_len2*sizeof(char));
	bs->_count = bs->_len;
}

void bitset_set(BitSet *bs, int pos)
{
	int i, j;

	if (pos >= bs->_len)
	{
		fprintf(stderr, "bit_set() bit postion :%d is invalid!\n", pos);
		return;
	}

	i = pos / BITS_PER_CHAR;
	j = pos - i * BITS_PER_CHAR;
	if (!bitset_isset(bs, pos))
	{
		bs->_count++;
		bs->_mblock[i] |= mask1[j];
	}
}

int bitset_isset(BitSet *bs, int pos)
{
	int i, j;

	if (pos >= bs->_len)
	{
		fprintf(stderr, "bitset_isset() bit postion :%d is invalid!\n", pos);
		return 0;
	}

	i = pos / BITS_PER_CHAR;
	j = pos - i * BITS_PER_CHAR;
	if (bs->_mblock[i] & mask1[j])
		return 1;
	return 0;
}

void bitset_print(BitSet *bs)
{
	int i;
	for (i = 0; i < bs->_len; ++i)
		printf("%c", (bitset_isset(bs, i) ? '1' : '0'));
	printf("\n");
}

char *bitset_to_str(BitSet *bs)
{
	int i;
	char *str = (char *)malloc(sizeof(char) * (bs->_len + 1));
	assert(str!=NULL);
	memset(str, 0, sizeof(char) * (bs->_len + 1));
	for (i = 0; i < bs->_len; ++i)
		str[i] = (bitset_isset(bs, i) ? '1' : '0');
	return str;
}

int bitset_any(BitSet* bs)
{
	int i;
	for (i=0;i<bs->_len2;i++)
	{
		if (bs->_mblock[i]>0)
		{
			return 1;
		}
	}
	return 0;
}
