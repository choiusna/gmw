//gmw.h

#ifndef __GMW_H__BY_SGCHOI
#define __GMW_H__BY_SGCHOI

#include "../util/sha1.h"

// undefine this to disuse private channel 
//#define IMPL_PRIVIATE_CHANNEL	
#ifdef IMPL_PRIVIATE_CHANNEL	
#define PKEY_SHA_INDEX			-1
#endif

#define RETRY_CONNECT			20
#define CONNECT_TIMEO_MILISEC	10000

const char GATE_INITIALIZED			= 0x10; 
const char GATE_ONGOING				= 0x20;
const char GATE_VALUE_NOTASSIGNED	= 0x30;
#define IsValueAssigned(x)		(!(x & GATE_VALUE_NOTASSIGNED))	


#define NUM_EXECS_NAOR_PINKAS	80
#define NUM_NP_BYTES			10

#define SHA1_BYTES				20
#define SHA1_BITS				160

#define OT_WINDOW_SIZE			(SHA1_BITS*128)
#define OT_WINDOW_SIZE_BYTES	(SHA1_BYTES*128)
 
struct SHA_BUFFER
{
	BYTE data[20];
	operator BYTE* (){ return data; }
};

struct SHA_BUF_MATRIX
{
	SHA_BUFFER	buf[NUM_EXECS_NAOR_PINKAS][4];
};

const BYTE MASK_BIT[8] = 
	{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

const BYTE CMASK_BIT[8] =  
	{0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

const BYTE MASK_SET_BIT[2][8] = 
	{{0,0,0,0,0,0,0,0},{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1}};

const BYTE MASK_SET_BIT_C[2][8] = 
	{{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1},{0,0,0,0,0,0,0,0}};

const BYTE MASK_2BITS[8] = 
	{0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3, 0x1}; 

const BYTE CMASK_2BITS[8] =  
	{0x3f, 0x9f, 0xcf, 0xe7, 0xf3, 0xf9, 0xfc, 0xfe};

const BYTE MASK_SET_2BITS[4][8] = 
	{{0,0,0,0,0,0,0,0},	
	{0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1, 0},
	{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1},
	{0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3, 0x1}};

const BYTE G_TRUTH_TABLE[3][2][2] = 
{
	{0,0,0,0},
	{0,0,0,1},		// and
	{0,1,1,0},		// xor
};


#define GETBIT(buf,i)	 !!((buf)[(i)>>3] & MASK_BIT[(i) & 0x7]) 


class CBitVector
{
public:
	CBitVector(){ m_pBits = NULL; m_nSize = 0; m_nRand = 0;}
	~CBitVector(){ if(m_pBits) delete [] m_pBits; }
	
	void FillRand(int bits, BYTE* seed, int& pid, int& cnt)
	{
		int size = (bits-1)/SHA1_BYTES + 1;
		if( size <= m_nRand ) return;

		sha1_context sha;
		for(int i=m_nRand; i<size; i++)
		{
			sha1_starts(&sha);
			sha1_update(&sha, seed, 20);
			sha1_update(&sha, (BYTE*) &pid, sizeof(int));
			sha1_update(&sha, (BYTE*) &cnt, sizeof(int));
			sha1_finish(&sha, m_pBits + i*SHA1_BYTES); 
			cnt++;
		}
		m_nRand = size;
	}

	void Create(int bits, BYTE* seed, int& pid, int& cnt)
	{
		if( m_pBits ) delete [] m_pBits;

		int size = (bits-1)/SHA1_BYTES + 1;
		m_nSize = size*SHA1_BYTES;
		m_pBits = new BYTE[m_nSize];
		sha1_context sha;
		
		for(int i=0; i<size; i++)
		{
			sha1_starts(&sha);
			sha1_update(&sha, seed, 20);
			sha1_update(&sha, (BYTE*) &pid, sizeof(int));
			sha1_update(&sha, (BYTE*) &cnt, sizeof(int));
			sha1_finish(&sha, m_pBits + i*SHA1_BYTES); 
			cnt++;
		}
	}

	
	void CreateinBytes(int bytes)
	{
		if( m_pBits ) delete [] m_pBits;
		m_pBits = new BYTE[bytes];
	}

	void Create(int bits, sha1_context& sha0, int& cnt )
	{
		if( m_pBits ) delete [] m_pBits;

		int size = (bits-1)/SHA1_BYTES + 1;
		m_nSize = size*SHA1_BYTES;
		m_pBits = new BYTE[m_nSize];
		
		sha1_context sha;
		for(int i=0; i<size; i++)
		{
			sha = sha0;
			sha1_update(&sha, (BYTE*) &cnt, sizeof(int));
			sha1_finish(&sha, m_pBits + i*SHA1_BYTES); 
			cnt++;
		}
	}

	void Create(int bits)
	{
		if( m_pBits ) delete [] m_pBits;
	
		int size = (bits-1)/SHA1_BYTES + 1;
		m_nSize = size*SHA1_BYTES;
		m_pBits = new BYTE[m_nSize];
	}

	BYTE GetBit(int idx)
	{
		return !!(m_pBits[idx>>3] & MASK_BIT[idx & 0x7]);
	}
	void SetBit(int idx, BYTE b)
	{
		m_pBits[idx>>3] = (m_pBits[idx>>3] & CMASK_BIT[idx & 0x7]) | MASK_SET_BIT_C[!b][idx & 0x7];
 	}

	void XORBit(int idx, BYTE b)
	{	
		m_pBits[idx>>3] ^= MASK_SET_BIT_C[!b][idx & 0x7];
	}

	void XOR(BYTE* p, int len)
	{
		for(int i=0; i<len; i++)
			m_pBits[i] ^= p[i];
	}

	BYTE Get2Bits(int idx)
	{
		idx <<= 1;  // times 2   
		return (m_pBits[idx>>3] & MASK_2BITS[idx & 0x7]) >> (6 - (idx & 0x7));
	}
	void Set2Bits(int idx, BYTE b)
	{
		idx <<= 1; // times 2   
		m_pBits[idx>>3] = (m_pBits[idx>>3] & CMASK_2BITS[idx & 0x7]) | MASK_SET_2BITS[b & 0x3][idx & 0x7];   
	}

	
	BYTE GetByte(int idx)
	{
		return m_pBits[idx];
	}
	BYTE* GetArr(){ return m_pBits;}

	void AttachBuf(BYTE* p, int size=-1){ m_pBits = p; m_nSize = size;}
	void DetachBuf(){ m_pBits = NULL; m_nSize = 0;}
	
	void Reset()
	{
		memset(m_pBits, 0, m_nSize);
	}

	int GetSize(){ return m_nSize; }

private:
	BYTE*		m_pBits;
	int			m_nSize;
	int			m_nRand;
};



class CBitNPMatrix 
{
public:
	CBitNPMatrix(){ m_pBits = NULL;}
	~CBitNPMatrix(){if(m_pBits) delete[] m_pBits;}
 
	void Create(int rows)		 
	{
		m_pBits = new BYTE[rows*NUM_NP_BYTES];
	}
	 
	BYTE GetBit(int row, int col)
	{
		return m_pBits[row*NUM_NP_BYTES + (col>>3)] & MASK_BIT[col & 0x7];
	}

	void SetBit(int row, int col, int b)
	{
		int idx = row*NUM_NP_BYTES + (col>>3);
		m_pBits[idx] = (m_pBits[idx] & CMASK_BIT[col & 0x7]) | MASK_SET_BIT_C[!b][col & 0x7];
	}
	
	BYTE* GetRow(int row)
	{
		return m_pBits + (row*NUM_NP_BYTES);
	}

	void XORRow(int row, BYTE* in, BYTE* out)
	{
		BYTE* in2 = m_pBits + row*NUM_NP_BYTES;
		for(int i=0; i<NUM_NP_BYTES; i++)
			out[i] = in[i] ^ in2[i];
	}
 	 
private:
	BYTE*		m_pBits;
};




#endif // __GMW_H__BY_SGCHOI

