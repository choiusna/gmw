// client.cpp
#include "client.h"
#include "../circuit/circuit.h"
#include "../util/config.h"
#include "../util/sha1.h"
#include "yao.h"

#ifdef _DEBUG
using namespace std;
#include <cassert>
#endif

void CClient::Init()
{
	// circuit
	CConfig* cf = CConfig::GetInstance();
	
	// random
	sha1_context sha;
	sha1_starts(&sha);
	sha1_update(&sha, (BYTE*) cf->GetSeed().c_str(), cf->GetSeed().length());
	sha1_finish(&sha, m_aSeed);
	m_nCounter = 1;
	

	if( !cf->GetCircFileName().empty() )
	{
		m_pCircuit = LOAD_CIRCUIT_BIN(cf->GetCircFileName().c_str());

		if(!m_pCircuit)
		{
			cout << "failure in loading circuit " << cf->GetCircFileName() << endl;
			return;
		}
	}
	else
	{
		m_pCircuit = CREATE_CIRCUIT(cf->GetNumParties(), cf->GetCircCreateName(), cf->GetCircCreateParams() );
	}

	m_pGates = m_pCircuit->Gates();
	m_nNumGates = m_pCircuit->GetNumGates();
	m_nGateStart = m_pCircuit->GetGateStart();
	 
	// bind the constant
	m_pGates[0].val = 0;
	m_pGates[1].val = 1;
	
	// bind the input
	int nBits = m_pCircuit->GetNumVBits(ID_CLIENT);
	int nStart = m_pCircuit->GetInputStart(ID_CLIENT);
	int nEnd = m_pCircuit->GetInputEnd(ID_CLIENT);
	vector<int> vIn;
	CConfig::GetInstance()->GetInput(vIn);	
	int nInputs = vIn.size();	
 	
	int j=nStart;
	for( int i=0; i<nInputs && j <=nEnd; i++ ) 
	{
		for(int k=0; k<nBits && j<=nEnd; k++ )
		{
			int mask = (1 << k );
			m_pGates[j++].val = !!(vIn[i] & mask);
		}
	}
	 
	// yao
	m_pYaoKeys = new KEY[m_nNumGates];
	m_pYaoGates  = new YAO_GARBLED_GATE[m_nNumGates-m_nGateStart+1];
	m_nGatesDone = -1;
	m_bOTDone = FALSE;
	m_bOutKeysReady = FALSE;

	// batch
	m_nGateBatch = NUM_GATE_BATCH; 
	m_nKeyBatch = NUM_KEY_BATCH; 
	 
	// IKNP
	int nNumInputBits = m_pCircuit->GetInputEnd(ID_CLIENT) - m_pCircuit->GetInputStart(ID_CLIENT) + 1;
	m_T.resize(NUM_EXECS_NAOR_PINKAS);
	for(int i=0; i<NUM_EXECS_NAOR_PINKAS; i++)
		m_T[i].Create(nNumInputBits, m_aSeed, m_nCounter );
}

void CClient::Run()
{
	Init();

	if(!m_sockMain.Socket()) return;
	if(!m_sockMain.Bind() )  return;

	if(!m_sockOT.Socket()) return;
	if(!m_sockOT.Bind() )  return;

	CConfig* pConfig = CConfig::GetInstance();
	if( !m_sockMain.Connect( pConfig->GetAddrPID(ID_SERVER), pConfig->GetPortPID(ID_SERVER) ) )
		return;

	if( !m_sockOT.Connect( pConfig->GetAddrPID(ID_SERVER), pConfig->GetPortPID(ID_SERVER) )  )
		return;
	 
	m_pThreadOT = new COTThread();
	m_pRecvThread = new CRecvThread();
	
	m_pThreadOT->Start();
	m_pRecvThread->Start();

	RunMainThread();

	m_pThreadOT->Wait();
	m_pRecvThread->Wait();
	
	delete m_pThreadOT; 
	delete m_pRecvThread; 

	Cleanup();
} 
		
void CClient::Cleanup()
{
	delete m_pCircuit; 
	// yao
	delete [] m_pYaoKeys;
	delete [] m_pYaoGates; 
}


void CClient::RunMainThread()
{
	// check constant & check server input
	int nServerEnd = m_pCircuit->GetInputEnd(ID_SERVER);
	while( m_nGatesDone < nServerEnd ) {
		SleepMiliSec(100);
	}
	
	// check client input
	while( !m_bOTDone ) {
		SleepMiliSec(100);
	}
		

	// Do evaluation
	int i = m_nGateStart; 
	BOOL bRun = TRUE;
	
	
	GATE* g;  
	YAO_GARBLED_GATE* yg;
	KEY *p, *c, *l, *r; 
	int offset;

	USE_ENCDEC();

	YAO_WIRE clean_wire = {0,0,0,0,0,0,0,0,0,0,0};
	KEY clean_key = {0,0,0,0,0};
	KEY temp_key;
	int type;
	for( ;  i< m_nNumGates; i++ )
	{
		while( m_nGatesDone < i ) {
			SleepMiliSec(100);		
		} 
		g = m_pGates + i;
		yg = m_pYaoGates + (i-m_nGateStart);
		p = m_pYaoKeys + i;
		type =g->type;
		
		 // random wire for the output of this gate
		GATE* g = m_pGates + i;
		l = m_pYaoKeys + g->left;
		r = m_pYaoKeys + g->right;

		#ifdef USE_XOR_TECH
		if( type == G_XOR )
		{
			XOR_KEYP3(p, l, r);
			continue;
		}
		#endif

		offset = ( l->val[4] & 1 )*2 + (r->val[4] & 1);
		c = yg->table + offset;
		Decrypt(p, c, l, r, i);
	
		#ifdef _DEBUG
		cout << "key[" << i  << "]: ";
		LOG_KEYP(p);
		cout << "left(" << g->left << ")";
		LOG_KEYP(l);
		cout << "right(" << g->right << ")";
		LOG_KEYP(r);
		cout << endl;
		#endif
	}

	// Output
	while( !m_bOutKeysReady )
	{
		SleepMiliSec(100);
	}

	int o_start = m_pCircuit->GetOutputStart(ID_CLIENT);
	int o_end = m_pCircuit->GetOutputEnd(ID_CLIENT);
	int o_size = o_end - o_start + 1;
	if( o_size > 0 )
	{ 
		m_vOutput.resize(o_size);

		for( int i=0, j=o_start; i<o_size; i++, j++)
		{
			#ifdef _DEBUG
			cout << endl << "id =" << j << endl; 
			cout << "okey0 ";
			LOG_KEY(m_vOutKeyPairs[i].keys[0]);
			cout << "okey1 ";
			LOG_KEY(m_vOutKeyPairs[i].keys[1]);
			cout << "gate-key ";
			LOG_KEY(m_pYaoKeys[j]);
			cout << endl;
			#endif

			p = &temp_key;
			c = m_vOutKeyPairs[i].keys;
			l = m_pYaoKeys + j;
			r = &clean_key;

			Decrypt(p, c, l, r, i);

			if( p->val[4] == 0 )
			{
				 m_vOutput[i] = 0;
				 continue;
			}

			c = m_vOutKeyPairs[i].keys + 1;
			Decrypt(p, c, l, r, i);

			if( p->val[4] == 1 )
			{
				m_vOutput[i] = 1;
			}
			else
			{ 
				// error..
				m_vOutput.push_back(2);
				cout << "error in the output gate-id = " << j << endl;
			}
		}
	}

	// send output wires
	o_start = m_pCircuit->GetOutputStart(ID_SERVER);
	o_end = m_pCircuit->GetOutputEnd(ID_SERVER);
	o_size = o_end - o_start + 1;
	
	if( o_size > 0 )
	{
		m_sockMain.Send( m_pYaoKeys + o_start, o_size * sizeof(KEY));
	}

}
 
void CClient::RunRecvThread()
{
	m_sockMain.Receive( m_pYaoKeys, sizeof(KEY) );	// constant 0;
	m_sockMain.Receive( (m_pYaoKeys+1), sizeof(KEY) );	// constant 1;

	// servers input
	int nInputStart = m_pCircuit->GetInputStart(ID_SERVER);
	int nInputEnd = m_pCircuit->GetInputEnd(ID_SERVER);

	KEY* pStart = m_pYaoKeys + nInputStart;
	int	 nSize = nInputEnd - nInputStart + 1;
	m_sockMain.Receive(pStart,  sizeof(KEY)*nSize);
 	m_nGatesDone = nInputEnd; 
	 
#ifdef _DEBUG
	cout << "recvt: server input received" << endl;
#endif

	// gates
	int nStart = m_pCircuit->GetGateStart();
	m_nGatesDone = nStart-1;
	int rcv;

#ifdef USE_XOR_TECH
	YAO_GARBLED_GATE* pBuf = new YAO_GARBLED_GATE[m_nGateBatch];
	nSize = m_pCircuit->GetNumANDs();

	#ifdef _DEBUG
	cout << "nSize = " << nSize << endl;
	#endif

	while( nSize > 0 )
	{
		if( nSize > m_nGateBatch ) rcv = m_nGateBatch;
		else rcv = nSize;
	 
		m_sockMain.Receive(pBuf, sizeof(YAO_GARBLED_GATE)*rcv);
		for(int i=0; i<rcv; i++)
		{
			while( m_pGates[m_nGatesDone+1].type == G_XOR ) m_nGatesDone++;
			m_pYaoGates[m_nGatesDone+1-nStart] = pBuf[i];

			#ifdef _DEBUG
			cout << "YAO_GATE[" << m_nGatesDone+1 << "]: ";
			LOG_KEYP(m_pYaoGates[m_nGatesDone+1-nStart].table);
			#endif

			m_nGatesDone++;
		}
		nSize -= rcv;
	}
	delete [] pBuf;
	while( m_pGates[m_nGatesDone+1].type == G_XOR ) m_nGatesDone++;
	
#else  
	nSize = m_nNumGates - nStart;
 	while( nSize > 0 )
	{
		if( nSize > m_nGateBatch ) rcv = m_nGateBatch;
		else rcv = nSize;
	
 		m_sockMain.Receive(m_pYaoGates + m_nGatesDone - nStart + 1, sizeof(YAO_GARBLED_GATE)*rcv);
		m_nGatesDone += rcv; 
		nSize -= rcv;
 	}
 #endif

#ifdef _DEBUG
	assert(m_nGatesDone  == m_nNumGates-1);
	cout << "recvt: gates received" << endl;
#endif
	
	// output gate
	int nOutputSize = m_pCircuit->GetOutputEnd(ID_CLIENT) - m_pCircuit->GetOutputStart(ID_CLIENT)+1;

	#ifdef _DEBUG
	cout << "output size = " << nOutputSize << endl;
	#endif
	 
	if( nOutputSize < 0 || nOutputSize > m_nNumGates )
	{
		cout << "error: the circuit has output size " << nOutputSize << endl;
	}
	else if( nOutputSize > 0 )
	{
		m_vOutKeyPairs.resize(nOutputSize);
		m_sockMain.Receive( &m_vOutKeyPairs[0], sizeof(KEY_PAIR)*nOutputSize);
	}
	m_bOutKeysReady = TRUE;

#ifdef _DEBUG
	cout << "recvt: output keys received" << endl;
#endif

} 


void CClient::RunOTThread()
{

	// generate input vector
	int nInputStart = m_pCircuit->GetInputStart(ID_CLIENT);
	int nInputEnd = m_pCircuit->GetInputEnd(ID_CLIENT);
	int nInputSize = nInputEnd-nInputStart+1; 
	m_r.Create(nInputSize);

 
	for(int i=nInputStart; i<=nInputEnd; i++)
	{
		m_r.SetBit(i-nInputStart, m_pGates[i].val);
	}
	 
	// IKNP-first step: sender of Naor-Pinkas
	ZZ& p = CConfig::GetInstance()->GetPrime();
	ZZ& g = CConfig::GetInstance()->GetGenerator();
	ZZ  q = p/2 - 1;
	
	int nBufSize = NUM_EXECS_NAOR_PINKAS * FIELD_SIZE_IN_BYTES;
	BYTE* pBuf = new BYTE[nBufSize];
	
	// generate and send c
	CBitVector rnd;
	rnd.Create( NUM_EXECS_NAOR_PINKAS*FIELD_SIZE_IN_BITS, m_aSeed, m_nCounter); 
	ZZ* pC = new ZZ[NUM_EXECS_NAOR_PINKAS ];
	BYTE* pBufIdx = pBuf;
	BYTE* pBufIn = rnd.GetArr();
	ZZ ztmp, ztmp2;

	for(int i=0; i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		ZZFromBytes(ztmp, pBufIn, FIELD_SIZE_IN_BYTES);
		rem(ztmp2, ztmp, p);
		SqrMod(pC[i], ztmp2, p);
		BytesFromZZ(pBufIdx, pC[i], FIELD_SIZE_IN_BYTES);
	
		pBufIn += FIELD_SIZE_IN_BYTES;
		pBufIdx += FIELD_SIZE_IN_BYTES;
	}
	 
	m_sockOT.Send(pBuf, NUM_EXECS_NAOR_PINKAS * FIELD_SIZE_IN_BYTES);
	 
	// receive pk0
	m_sockOT.Receive(pBuf, nBufSize);
	ZZ* pPK0 = new ZZ[NUM_EXECS_NAOR_PINKAS];
	ZZ* pPK1 = new ZZ[NUM_EXECS_NAOR_PINKAS];
	
	pBufIdx = pBuf;
	for(int i=0; i<NUM_EXECS_NAOR_PINKAS; i++ )
	{
		ZZFromBytes(pPK0[i], pBufIdx, FIELD_SIZE_IN_BYTES);
		pBufIdx += FIELD_SIZE_IN_BYTES;

		// pPK[i] = pC[i]/pPK0[i]
		InvMod(ztmp, pPK0[i], p);
		MulMod(pPK1[i], pC[i], ztmp, p);
 	} 
	delete [] pBuf;
	 
	// send <g^r1, Enc(M0)> and <g^r2, Enc(M1)>
	int nMsgSize = (nInputEnd-nInputStart)/SHA1_BITS + 1;		// in sha1 scale
	int nMsginOT = FIELD_SIZE_IN_BYTES + nMsgSize*SHA1_BYTES;  
	int nBufSize2 = NUM_EXECS_NAOR_PINKAS * nMsginOT * 2;   
	BYTE* pBuf2 = new BYTE[nBufSize2];
	 
	// to do
	ZZ* pR0 = new ZZ[NUM_EXECS_NAOR_PINKAS];
	ZZ* pR1 = new ZZ[NUM_EXECS_NAOR_PINKAS];

	rnd.Create( NUM_EXECS_NAOR_PINKAS*2*FIELD_SIZE_IN_BITS, m_aSeed, m_nCounter);
	pBufIdx = rnd.GetArr();
	for(int i=0; i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		ZZFromBytes(ztmp, pBufIdx, FIELD_SIZE_IN_BYTES);
		rem(pR0[i], ztmp, q);
		pBufIdx += FIELD_SIZE_IN_BYTES;

		ZZFromBytes(ztmp, pBufIdx, FIELD_SIZE_IN_BYTES);
		rem(pR1[i], ztmp, q);
		pBufIdx += FIELD_SIZE_IN_BYTES;
	}

	ZZ gr0, gr1, pkr0, pkr1;
		
	pBufIdx = pBuf2;
	sha1_context sha;
	BYTE tmp[FIELD_SIZE_IN_BYTES];
	SHA_BUFFER	buf_key;
	for(int i=0; i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		// put g^r0
		PowerMod(gr0, g, pR0[i], p);
		BytesFromZZ(pBufIdx, gr0, FIELD_SIZE_IN_BYTES);
		pBufIdx += FIELD_SIZE_IN_BYTES;

		// compute the key for M0
		PowerMod(pkr0, pPK0[i], pR0[i], p);
		BytesFromZZ(tmp, pkr0, FIELD_SIZE_IN_BYTES);
		
 		sha1_starts(&sha);
		sha1_update(&sha, tmp, FIELD_SIZE_IN_BYTES);
		sha1_finish(&sha, (BYTE*) &buf_key);
		
		// put Enc(M0): M0 = t
 		for(int j=0, k=0; j<nMsgSize; j++)
		{
			sha1_starts(&sha);
			sha1_update(&sha, (BYTE*) &buf_key, sizeof(buf_key));
			sha1_update(&sha, (BYTE*) &j, sizeof(int));
			sha1_finish(&sha, pBufIdx);
			 
 			for(int x=0; x < SHA1_BYTES; x++, k++, pBufIdx++ )
			{
				*(pBufIdx) ^= m_T[i].GetByte(k);
 			}
		}
 	 
		// put g^r1
		PowerMod(gr1, g, pR1[i], p);
		BytesFromZZ(pBufIdx, gr1, FIELD_SIZE_IN_BYTES);
		pBufIdx += FIELD_SIZE_IN_BYTES;

		// compute the key for M1
		PowerMod(pkr1, pPK1[i], pR1[i], p);
		BytesFromZZ(tmp, pkr1, FIELD_SIZE_IN_BYTES);
		 
		sha1_starts(&sha);
		sha1_update(&sha, tmp, FIELD_SIZE_IN_BYTES);
		sha1_finish(&sha, (BYTE*) &buf_key);
		
		// put Enc(M1) : M1 = r xor t
	 	for(int j=0,k=0; j<nMsgSize; j++)
		{
			sha1_starts(&sha);
			sha1_update(&sha, (BYTE*) &buf_key, sizeof(buf_key));
			sha1_update(&sha, (BYTE*) &j, sizeof(int));
			sha1_finish(&sha, pBufIdx);
			 
			for(int x=0; x < SHA1_BYTES; x++, pBufIdx++, k++ )
			{
				*pBufIdx ^= m_T[i].GetByte(k) ^ m_r.GetByte(k);
	 		}
		}

	}
	m_sockOT.Send(pBuf2, nBufSize2);

	delete [] pBuf2;
	delete [] pR0;
	delete [] pR1; 
	 
	// IKNP: recv the keys for client inputs
	KEY* pKeys = new KEY[nInputSize*2];
	m_sockOT.Receive(pKeys, nInputSize*sizeof(KEY)*2);
	KEY* pKeyIdx = pKeys; 
	KEY* pYaoKeyIdx = m_pYaoKeys + nInputStart;
	CBitVector tj;
	tj.Create(NUM_EXECS_NAOR_PINKAS);

	for(int i=nInputStart, j=0; i<nInputEnd+1; i++, j++)
	{
		for(int x=0; x<NUM_EXECS_NAOR_PINKAS; x++)
			tj.SetBit(x, m_T[x].GetBit(j));

		sha1_starts(&sha);
		sha1_update(&sha, tj.GetArr(), NUM_EXECS_NAOR_PINKAS/8);
		sha1_update(&sha, (BYTE*)&j, sizeof(int));
		sha1_finish(&sha, (BYTE*)&buf_key);
		
		/*
		#ifdef _DEBUG
		cout << "H(tj, j)=";
		LOG_KEY(*pYaoKeyIdx);
		cout <<endl;

		cout << "gate-val=" << (int) m_pGates[i].val << endl;
		cout << "key0=";
		LOG_KEY(*pKeyIdx);
		cout << "key1=";
		LOG_KEY(*(pKeyIdx+1));
		#endif
		*/
		 
		if( !m_pGates[i].val )
		{
			XOR_KEYP3(pYaoKeyIdx, (&buf_key), pKeyIdx);
			pKeyIdx++;
			pKeyIdx++;
		}
		else
		{
			pKeyIdx++;
			XOR_KEYP3(pYaoKeyIdx, (&buf_key), pKeyIdx);
			pKeyIdx++;
		}

		/*
		#ifdef _DEBUG
		cout << "gateid: " << i << " ";
		LOG_KEY(*pYaoKeyIdx);
		cout << endl;
		#endif
		*/

		pYaoKeyIdx++;
		
	}

	// clean-up
	delete [] pKeys; 
	m_bOTDone = TRUE;
}

