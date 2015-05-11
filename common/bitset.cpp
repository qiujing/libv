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

CBitSet::CBitSet(int bitsCount)
{
    _count = 0;
    _len = bitsCount;
    // the real length of _mblock is (bitsCount / BITS_PER_CHAR + 1) in Byte
    _len2 = _len / BITS_PER_CHAR + 1;
    _mblock = new char[_len2];
    assert(_mblock != NULL);
    memset(_mblock, 0, sizeof(char) * _len2);
}

CBitSet::~CBitSet()
{
    if (_mblock != NULL)
    {
        delete [] _mblock;
    }
}

int CBitSet::size()
{
    return _len;
}

int CBitSet::count()
{
    return _count;
}

void CBitSet::reset_all()
{
    memset(_mblock, 0x00, _len2 * sizeof(char));
    _count = 0;
}

void CBitSet::reset(int pos)
{
    int i, j;

    if (pos >= _len)
    {
        fprintf(stderr, "bit postion :%d is invalid!\n", pos);
        return;
    }

    i = pos / BITS_PER_CHAR;
    j = pos - i * BITS_PER_CHAR;
    if (isset(pos))
    {
        _mblock[i] &= mask2[j];
        _count--;
    }
}

void CBitSet::set_all()
{
    memset(_mblock, 0xFF, _len2 * sizeof(char));
    _count = _len;
}

void CBitSet::set(int pos)
{
    int i, j;

    if (pos >= _len)
    {
        fprintf(stderr, "bit_set() bit postion :%d is invalid!\n", pos);
        return;
    }

    i = pos / BITS_PER_CHAR;
    j = pos - i * BITS_PER_CHAR;
    if (!isset(pos))
    {
        _count++;
        _mblock[i] |= mask1[j];
    }
}

int CBitSet::isset(int pos)
{
    int i, j;

    if (pos >= _len)
    {
        fprintf(stderr, "bitset_isset() bit postion :%d is invalid!\n", pos);
        return 0;
    }

    i = pos / BITS_PER_CHAR;
    j = pos - i * BITS_PER_CHAR;
    if (_mblock[i] & mask1[j])
    {
        return 1;
    }
    return 0;
}

void CBitSet::print()
{
    int i;
    for (i = 0; i < _len; ++i)
    {
        printf("%c", (isset(i) ? '1' : '0'));
    }
    printf("\n");
}

int CBitSet::any()
{
    int i;
    for (i = 0; i < _len2; i++)
    {
        if (_mblock[i] > 0)
        {
            return 1;
        }
    }
    return 0;
}

// Check if equal to bs
bool CBitSet::equal(CBitSet* bs)
{
    if (bs->getByteLength() != _len2)
    {
        return false;
    }
    return memcmp(_mblock, bs->getBuffer(), _len2) == 0;
}

int CBitSet::getByteLength()
{
    return _len2;
}

char* CBitSet::getBuffer()
{
    return _mblock;
}

bool CBitSet::operator[](unsigned index)
{
    return isset(index);
}