// yao.h

#ifndef __YAO_H__BY_SGCHOI
#define __YAO_H__BY_SGCHOI

#include "../util/typedefs.h"
#include "../circuit/circuit.h"
#include "../util/sha1.h"


#define			ID_SERVER	0
#define			ID_CLIENT	1


//============================================================
// OPTIONS
// option1: size of key
//#define USE_160_BIT_KEY
#define USE_80_BIT_KEY

// option2: use xor tech?
#define USE_XOR_TECH
//#define NO_XOR_TECH
//============================================================

#ifdef USE_160_BIT_KEY

struct KEY
{
	ULONG	val[5];
};

struct SHA_BUFFER
{
	ULONG	val[5];
};

#define RandomWire(w, seed, counter, sha)	\
\
	sha1_starts(sha);									\
	sha1_update(sha, seed, 20);							\
	sha1_update(sha, (BYTE*) &counter, sizeof(int));	\
	sha1_finish(sha, (BYTE*) w->keys);					\
	counter++;											\
	\
	sha1_starts(sha);									\
	sha1_update(sha, seed, 20);							\
	sha1_update(sha, (BYTE*) &counter, sizeof(int));	\
	sha1_finish(sha, (BYTE*) (w->keys+1));				\
	counter++;											\
	\
	w->b = w->keys[0].val[4] & 1;						\
	w->keys[0].val[4] &= 0xfffffffe;						\
	w->keys[1].val[4] |= 0x00000001;


#define RandomWireXOR(w, R, seed, counter, sha)	\
\
	sha1_starts(sha);									\
	sha1_update(sha, seed, 20);							\
	sha1_update(sha, (BYTE*) &counter, sizeof(int));	\
	sha1_finish(sha, (BYTE*) w->keys);					\
	counter++;											\
	\
	w->b = w->keys[0].val[4] & 1;						\
	w->keys[0].val[4] &= 0xfffffffe;						\
	XOR_KEYP3(w->keys+1, w->keys, R);													
	

#else  // USE_160_BIT_KEY
 
struct KEY
{
	USHORT  val[5];
};

struct SHA_BUFFER
{
	USHORT val[10];
};

#define RandomWire(w, seed, counter, sha)	\
\
	sha1_starts(sha);									\
	sha1_update(sha, seed, 20);							\
	sha1_update(sha, (BYTE*) &counter, sizeof(int));	\
	sha1_finish(sha, (BYTE*) w->keys);					\
	counter++;											\
 	\
	w->b = w->keys[0].val[4] & 1;						\
	w->keys[0].val[4] &= 0xfffe;							\
	w->keys[1].val[4] |= 0x0001;	


#define RandomWireXOR(w, R, seed, counter, sha)	\
\
	sha1_starts(sha);									\
	sha1_update(sha, seed, 20);							\
	sha1_update(sha, (BYTE*) &counter, sizeof(int));	\
	sha1_finish(sha, (BYTE*) w->keys);					\
	counter++;											\
	\
	w->b = w->keys[0].val[4] & 1;						\
	w->keys[0].val[4] &= 0xfffe;							\
	XOR_KEYP3(w->keys+1, w->keys, R)	;												

#endif

 
#define USE_ENCRYPTION()	SHA_BUFFER mask1; 	sha1_context sha;
#define USE_ENCDEC()		SHA_BUFFER mask1; 	sha1_context sha;

#define Encrypt(c, p, l, r, id)   \
	sha1_starts(&sha);	\
	sha1_update(&sha, (BYTE*) l, sizeof(KEY));	\
	sha1_update(&sha, (BYTE*) r, sizeof(KEY));	\
	sha1_update(&sha, (BYTE*) &id, sizeof(int)); \
	sha1_finish(&sha, (BYTE*) &mask1);	\
	\
	c->val[0] = p->val[0]^mask1.val[0];	\
	c->val[1] = p->val[1]^mask1.val[1];	\
	c->val[2] = p->val[2]^mask1.val[2];	\
	c->val[3] = p->val[3]^mask1.val[3];	\
	c->val[4] = p->val[4]^mask1.val[4];	

#define Decrypt(p, c, l, r, id)	\
\
	sha1_starts(&sha);						\
	sha1_update(&sha, (BYTE*) l, sizeof(KEY));	\
	sha1_update(&sha, (BYTE*) r, sizeof(KEY));	\
	sha1_update(&sha, (BYTE*) &id, sizeof(int)); \
	sha1_finish(&sha, (BYTE*) &mask1);			\
	\
	p->val[0] = c->val[0]^mask1.val[0];	\
	p->val[1] = c->val[1]^mask1.val[1];	\
	p->val[2] = c->val[2]^mask1.val[2];	\
	p->val[3] = c->val[3]^mask1.val[3];	\
	p->val[4] = c->val[4]^mask1.val[4];	 

#define COPY_KEY(x,y)	(x) = (y) 
#define COPY_KEYP(x,y)	*(x) = *(y) 

#define XOR_KEY(x,y) \
{ x.val[0]^=y.val[0]; x.val[1]^=y.val[1]; x.val[2]^=y.val[2]; \
  x.val[3]^=y.val[3]; x.val[4]^=y.val[4];}

#define XOR_KEYP(x,y) \
{ x->val[0]^=y->val[0]; x->val[1]^=y->val[1]; x->val[2]^=y->val[2]; \
  x->val[3]^=y->val[3]; x->val[4]^=y->val[4];}

#define XOR_KEY3(x,y,z) \
{	\
	x.val[0] = y.val[0]^z.val[0]; \
	x.val[1] = y.val[1]^z.val[1]; \
	x.val[2] = y.val[2]^z.val[2]; \
	x.val[3] = y.val[3]^z.val[3]; \
	x.val[4] = y.val[4]^z.val[4]; \
}

#define XOR_KEYP3(x,y,z) \
{	\
	(x)->val[0] = ((y)->val[0]) ^ ((z)->val[0]); \
	(x)->val[1] = ((y)->val[1]) ^ ((z)->val[1]); \
	(x)->val[2] = ((y)->val[2]) ^ ((z)->val[2]); \
	(x)->val[3] = ((y)->val[3]) ^ ((z)->val[3]); \
	(x)->val[4] = ((y)->val[4]) ^ ((z)->val[4]); \
}

#define LOG_KEY(x)	\
{ cout << (x).val[1] << (x).val[2] << (x).val[3] << (x).val[4] << endl;}

#define LOG_KEYP(x) \
{ cout << (x)->val[1] << (x)->val[2] << (x)->val[3] << (x)->val[4] << endl;}


struct YAO_WIRE
{
	KEY			keys[2];		// left-input-wire keys
	BYTE			b;
};

struct YAO_GARBLED_GATE
{
	KEY			table[4];
};

struct KEY_PAIR
{
	KEY			keys[2]; 
};
 
//===================================================================================
// OT
#define NUM_EXECS_NAOR_PINKAS		80
#define FIELD_SIZE_IN_BITS			512
#define FIELD_SIZE_IN_BYTES			64
#define SHA1_BITS					160
#define SHA1_BYTES					20
#define NUM_GATE_BATCH				(0x100000/ sizeof(YAO_GARBLED_GATE))	// 1MB
#define NUM_KEY_BATCH				(0x100000/ sizeof(KEY))


//=======================================================================================
// truth table
const BYTE g_TruthTable[7][2][2] = 
{
	{0,0,0,0},
	{0,0,0,1},		// and
	{0,1,1,1},		// or
	{0,1,1,0},		// xor
	{1,1,1,0},		// nand
	{1,0,0,0},		// nor
	{1,0,1,0},		// xnor 
};

const BYTE bitmask[8] = {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
const BYTE cbitmask[8] =  {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe}; 
const BYTE bitmask_set[2][8] = {{0,0,0,0,0,0,0,0},{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1}};
const BYTE bitmask_setc[2][8] = {{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1},{0,0,0,0,0,0,0,0}};


#define TRUTH_TABLE(t,l,r)  (g_TruthTable[t][l][r])

class CBitVector
{
public:
	CBitVector(){ m_pBits = NULL;}
	~CBitVector(){ if(m_pBits) delete [] m_pBits; }
	
	void Create(int bits, BYTE* seed, int& cnt)
	{
		if( m_pBits ) delete [] m_pBits;

		int size = (bits-1)/SHA1_BYTES + 1;
		m_pBits = new BYTE[size*SHA1_BYTES];
		sha1_context sha;

		for(int i=0; i<size; i++)
		{
			sha1_starts(&sha);
			sha1_update(&sha, seed, 20);
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

	void Create(int bits)
	{
		if( m_pBits ) delete [] m_pBits;
	
		int size = (bits-1)/SHA1_BYTES + 1;
		m_pBits = new BYTE[size*SHA1_BYTES];
	}

	BYTE GetByte(int idx)
	{
		return m_pBits[idx];
	}

	BYTE GetBit(int idx)
	{
		return !!(m_pBits[idx/8] & bitmask[idx & 0x7]);
	}
	void SetBit(int idx, BYTE b)
	{
		m_pBits[idx/8] = (m_pBits[idx/8] & cbitmask[idx & 0x7]) | bitmask_setc[!b][idx & 0x7];
	}
  	BYTE* GetArr(){ return m_pBits;}

private:
	BYTE*		m_pBits;
};


#endif //__YAO_H__BY_SGCHOI


