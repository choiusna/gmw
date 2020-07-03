#include "server.h"
#include "../circuit/circuit.h"
#include "../util/config.h"
#include "../util/sha1.h"
#include "yao.h"
#include <cmath>

#ifdef _DEBUG
#include <cassert>
#endif

BOOL CServer::Init()
{
	// circuit
	CConfig* cf = CConfig::GetInstance();
	int nInputs = cf->GetNumInputs();
		
	if( !cf->GetCircFileName().empty() )
	{
		m_pCircuit = LOAD_CIRCUIT_BIN(cf->GetCircFileName().c_str());

		if(!m_pCircuit)
		{
			cout << "failure in loading circuit " << cf->GetCircFileName() << endl;
			return FALSE;
		}
	}
	else
	{
		m_pCircuit = CREATE_CIRCUIT(cf->GetNumParties(), cf->GetCircCreateName(), cf->GetCircCreateParams() );
	}

	m_pGates = m_pCircuit->Gates();
	m_nNumGates = m_pCircuit->GetNumGates();

	// bind the constant
	m_pGates[0].val = 0;
	m_pGates[1].val = 1;
	
	// bind the input
	int nBits = m_pCircuit->GetNumVBits(ID_SERVER);
	int nStart = m_pCircuit->GetInputStart(ID_SERVER);
	int nEnd = m_pCircuit->GetInputEnd(ID_SERVER);

	vector<int> vIn;
	CConfig::GetInstance()->GetInput(vIn);	
	//cout << "s=" << nStart << " e= " << nEnd << endl;
	
	int j=nStart;
	for( int i=0; i<nInputs && j <=nEnd; i++ ) 
	{
		for(int k=0; k<nBits && j<=nEnd; k++ )
		{
			int mask = (1 << k );
			m_pGates[j++].val = !!(vIn[i] & mask);
		}
	}
	 
	#ifdef _DEBUG
	
	if( nInputs == 10 )
	{
		j = nStart;
		for( int i=0; i<nInputs; i++ )
		{
			cout << "id[" << i << "]: ";
	
			for(int k=0; k<nBits; k++ )
			{
				cout << (int) m_pGates[j].val;
				j++;
			}
			cout << endl;
		}
		assert(j == nEnd+1);


		int i= m_pCircuit->GetInputStart(ID_CLIENT); 
		m_pGates[i++].val = 0;
		m_pGates[i++].val = 0;
		m_pGates[i++].val = 1;
		m_pGates[i++].val = 0;
		m_pGates[i++].val = 0;
		m_pGates[i++].val = 1;
		m_pGates[i++].val = 1;
		m_pGates[i++].val = 0;
		m_pGates[i++].val = 0;
		m_pGates[i++].val = 0;
 		m_pCircuit->Evaluate();
	}
	#endif

	// yao
	m_pYaoWires = new YAO_WIRE[m_nNumGates];
	m_pYaoGates  = new YAO_GARBLED_GATE[m_nNumGates - m_pCircuit->GetGateStart()];
	m_vOutGates.reserve(1024);
	m_nGatesDone = -1;
	m_nWiresDone = -1;

	// randomness
	sha1_context sha;
	sha1_starts(&sha);
	sha1_update(&sha, (BYTE*) cf->GetSeed().c_str(), cf->GetSeed().length());
	sha1_finish(&sha, m_aSeed);
	m_nCounter = 1;
	
	// batch parameters
 	m_nGateBatch = NUM_GATE_BATCH;
	m_nKeyBatch = NUM_KEY_BATCH; 
	 

	// IKNP
	m_S.Create(NUM_EXECS_NAOR_PINKAS, m_aSeed, m_nCounter );

	m_bStop = FALSE;

	return TRUE;
}

void CServer::Run()
{
	Init();

	CSocket sock;
	if(!sock.Socket()) return;
	if(!sock.Bind( CConfig::GetInstance()->GetPortPID(ID_SERVER), CConfig::GetInstance()->GetAddrPID(ID_SERVER)) ) return;
	
	cout << "listening.." << endl;
	sock.Listen();

	
	CWireThread* pThreadWire = new CWireThread();
	pThreadWire->Start();

	CGateThread* pThreadGate = new CGateThread();
	pThreadGate->Start();
	 
	if(!sock.Accept(m_sockMain)) return;
	if(!sock.Accept(m_sockOT)) return;
	sock.Close();

	cout << "accepted sockets!" << endl;

	COTThread* pThreadOT = new COTThread();

	pThreadOT->Start();
 
	RunMainThread();
	m_sockMain.Close();

	pThreadOT->Wait();
	m_sockOT.Close();

	pThreadGate->Wait();
	pThreadWire->Wait();

	Cleanup();

	delete pThreadOT;
	delete pThreadGate;
	delete pThreadWire;
	
} 


void CServer::Cleanup()
{
	delete m_pCircuit;
	delete [] m_pYaoWires;
	delete [] m_pYaoGates;
}



void CServer::RunWireThread()
{
	cout << "\nwire thread started \n" << flush;
	int wireid;
	YAO_WIRE* wire;
	sha1_context sha;


#ifdef USE_XOR_TECH
	int left, right, type;
	YAO_WIRE *wl, *wr;  
	KEY *w, *w1, *l, *r;
	
	YAO_WIRE tmp_wire;
	YAO_WIRE* tmp = &tmp_wire;
	RandomWire(tmp, m_aSeed, m_nCounter, &sha);
	KEY* R = tmp->keys + 1;
	 
	for( ; m_nWiresDone < m_nNumGates-1; ++m_nWiresDone )
	{ 
		wireid = m_nWiresDone+1;
		wire = m_pYaoWires + wireid; 
		type = m_pGates[wireid].type;
	 	
		// random wire for the output of this gate
		if( type == G_XOR )
		{
			left = m_pGates[wireid].left;
			right = m_pGates[wireid].right;

			#ifdef _DEBUG
			assert( wireid > left && wireid > right );
			#endif

			wl = m_pYaoWires + left;
			wr = m_pYaoWires + right;

			wire->b = wl->b ^ wr->b;
			w = wire->keys + wire->b;
			l = wl->keys + wl->b;
			r = wr->keys + wr->b;  
			XOR_KEYP3(w, l, r);

			w1 = wire->keys + (wire->b^1);
			XOR_KEYP3(w1, w, R);
		}
		else 
		{
			RandomWireXOR(wire, R, m_aSeed, m_nCounter, &sha);
		}
	}
#else
	for( ; m_nWiresDone < m_nNumGates-1; ++	m_nWiresDone )
	{ 
		wireid = m_nWiresDone+1;
		wire = m_pYaoWires + wireid; 
	 	
		// random wire for the output of this gate
		RandomWire( wire, m_aSeed, m_nCounter, &sha);
	}
#endif
	cout << "\nwire thread ended \n" << flush;
}


void CServer::RunGateThread()
{
	cout <<  "\ngate thread started \n" << flush;
	int nGateStart = m_pCircuit->GetGateStart();
	m_nGatesDone = nGateStart-1;

	USE_ENCRYPTION();
	KEY *c, *p, *l, *r; 
	KEY* table;
	GATE* gate;
	
	BYTE type;
	YAO_WIRE *leftwire, *rightwire, *outwire;
	int gateid, left, right;
	YAO_WIRE clean_wire = {0,0,0,0,0,0,0,0,0,0,0};
	KEY clean_key = {0,0,0,0,0};

	int a0, a1, b0, b1, c0;

	for( ; m_nGatesDone < m_nNumGates-1; ++m_nGatesDone )
	{ 
		gateid = m_nGatesDone+1;
		gate = m_pGates+gateid;
		type = gate->type;
  
		#ifdef USE_XOR_TECH 
		if( type == G_XOR ) 
			continue;
		#endif
		

		outwire = m_pYaoWires + gateid;
		left = gate->left;
		leftwire = m_pYaoWires + left;
		right = gate->right;
		rightwire = m_pYaoWires + right; 
 
		while( gateid > m_nWiresDone || left > m_nWiresDone || right > m_nWiresDone )
		{
			SleepMiliSec(100);
		}
		
		table = m_pYaoGates[gateid - nGateStart].table;
		a0 = leftwire->b;
		a1 = a0^1;
		b0 = rightwire->b;
		b1 = b0^1;
		c0 = outwire->b;
		
		c = table;
		p = outwire->keys + (TRUTH_TABLE(type, a0, b0)^c0); 
		l = leftwire->keys;
		r = rightwire->keys;
		Encrypt(c, p, l, r, gateid);
			
			
		c++; 
		p = outwire->keys + (TRUTH_TABLE(type, a0, b1)^c0); 
		l = leftwire->keys; 
		r = rightwire->keys + 1; 
		Encrypt(c, p, l, r, gateid);
		
		c++; 
		p = outwire->keys + (TRUTH_TABLE(type, a1, b0)^c0);  
		l = leftwire->keys + 1; 
		r = rightwire->keys; 
		Encrypt(c, p, l, r, gateid);
		
		c++; 
		p = outwire->keys + (TRUTH_TABLE(type, a1, b1)^c0);  
		l = leftwire->keys + 1; 
		r = rightwire->keys + 1; 
		Encrypt(c, p, l, r, gateid);
	}

	#ifdef _DEBUG
	TestEvaluate();
	#endif

	cout << "\ngate thread ended \n" << flush;
}
 
void CServer::RunMainThread()
{ 

	KEY* pKeys = new KEY[m_nKeyBatch];
	KEY* key1 = pKeys;
	KEY* key2;

	int j=0;
	
	// send constants
	cout << "\nsending server inputs...\n" << flush;
	while( m_nWiresDone < 2) {
		SleepMiliSec(100);
	}
	YAO_WIRE* wire = m_pYaoWires;	// constant 0
	key2 = wire->keys + wire->b;  
	COPY_KEYP(key1, key2);
	key1++;

	wire = m_pYaoWires+1;	// constant 1
	key2 = wire->keys + (wire->b ^ 1);  
	COPY_KEYP(key1, key2); 
	key1++;
		
	// send servers input
	GATE* gate;
	int nInputStart = m_pCircuit->GetInputStart(ID_SERVER);
	int nInputEnd = m_pCircuit->GetInputEnd(ID_SERVER);
	j=2;
	for(int i=nInputStart; !m_bStop && i<=nInputEnd; i++)
	{
		while( m_nWiresDone < i)
		{
			cout <<"z";
			SleepMiliSec(100);
		}

		gate = m_pGates + i; 
		wire = m_pYaoWires + i;  
		key2 = wire->keys + (gate->val ^ wire->b); 
	
		#ifdef _DEBUG
		assert( wire->b == 0 || wire->b == 1);
		#endif
	
		COPY_KEYP(key1, key2);
 
		j++;
		key1++;
		if( j >= m_nKeyBatch )
		{
			cout <<"?";
			if(m_sockMain.Send( pKeys,  sizeof(KEY)*j) == 0 )
				return;
			cout <<"s";
			key1 = pKeys;
			j = 0;
		}
	}

	if(j) 
	{
		if( m_sockMain.Send( pKeys,  sizeof(KEY)*j) == 0 )
			return;
	}
 	
	
	// send garbled gates
	cout << "\nsending garbled gates...\n" << flush;
	int nStart = m_pCircuit->GetGateStart();
	YAO_GARBLED_GATE* pGateIdx = m_pYaoGates;
	
#ifdef USE_XOR_TECH
	YAO_GARBLED_GATE* pGateBuf  = new YAO_GARBLED_GATE[m_nGateBatch];  
	j=0;
	int total = 0;
	for(int i= nStart; !m_bStop && i<m_nNumGates; i++, pGateIdx++)
	{
		while( m_nGatesDone < i )
		{
			cout << "z";
			SleepMiliSec(100);
		}

		if( m_pGates[i].type == G_XOR ) 
			continue;
	
		pGateBuf[j] = *pGateIdx;  
		j++;
		total++;

		if( j >= m_nGateBatch )
		{
			cout <<"?";
			if( m_sockMain.Send( pGateBuf,  sizeof(YAO_GARBLED_GATE)*j ) == 0 )
				return;
			cout <<"g";
			j=0;  
		}
	}

	if( j )
	{
		if(	m_sockMain.Send( pGateBuf,  sizeof(YAO_GARBLED_GATE)*j ) == 0 )
			return;
	}
	delete [] pGateBuf;

	cout << "\nsent " << total << " gates" <<endl;

#else
	j=0;
	for(int i= nStart; !m_bStop && i<m_nNumGates; i++ )
	{
		while( m_nGatesDone < i )
		{
			cout << "z";
			SleepMiliSec(100);
		}

		j++;
		if( j >= m_nGateBatch )
		{
			cout <<"?";
			if( m_sockMain.Send( pGateIdx,  sizeof(YAO_GARBLED_GATE)*j ) == 0 )
				return;
			cout <<"g";
			pGateIdx += j;
			j = 0;
		}
	}
	
	if( j )
	{
		if(	m_sockMain.Send( pGateIdx ,  sizeof(YAO_GARBLED_GATE)*j ) == 0 )
			return;
	}
#endif
	
	cout << "\nsending output wire keys...\n" << flush;
	
	// send output wires
	int o_start = m_pCircuit->GetOutputStart(ID_CLIENT);
	int o_end = m_pCircuit->GetOutputEnd(ID_CLIENT);
	int o_size = o_end - o_start + 1;
	if( o_size <= 0 ) return;

	 
	// send 2 keys for each output wire
	KEY zero = {0,0,0,0,0};
	KEY one = {0,0,0,0,1};

	j=0;
	KEY *c, *p, *l, *r;
	r = &zero;

	USE_ENCRYPTION();
	for( int i=0, w=o_start; !m_bStop && i<o_size; i++, w++)
	{
		wire = m_pYaoWires + w;
		c = pKeys+j;
		p = &zero;
		l = wire->keys + wire->b;
		Encrypt(c, p, l, r, i);  
		j++;

		c = pKeys+j;
		p = &one;
		l = wire->keys + (wire->b ^ 1);
		Encrypt(c, p, l, r, i);
		j++;

		if( j >= m_nKeyBatch )
		{
			m_sockMain.Send(pKeys, sizeof(KEY)*j);
			j=0;
		}
	}

	m_sockMain.Send(pKeys, sizeof(KEY)*j);
	
 
	// send output wires
	o_start = m_pCircuit->GetOutputStart(ID_SERVER);
	o_end = m_pCircuit->GetOutputEnd(ID_SERVER);
	o_size = o_end - o_start + 1;
	
	if( o_size > 0 )
	{
		m_vOutput.resize(o_size);
 		cout << "\nreceiving output wire keys.. \n" << flush;
	
		for(int i=0, j=o_start; i<o_size; i++, j++)
		{
			m_sockMain.Receive(pKeys, sizeof(KEY));
			if( !memcmp( pKeys, m_pYaoWires[j].keys, sizeof(KEY) ) )  
				m_vOutput[i] = m_pYaoWires[j].b;
			else
				m_vOutput[i] = 1 - m_pYaoWires[j].b;
		}
	}

	delete [] pKeys;


	cout << "\nmain thread ended\n" << flush;
} 



void CServer::RunOTThread()
{
	cout << "\not thread started\n" << flush;

	// IKNP-first step: receiver of Naor-Pinkas  
	ZZ& p = CConfig::GetInstance()->GetPrime();
	ZZ  q = p/2 - 1;
	ZZ& g = CConfig::GetInstance()->GetGenerator();
	

	// NP receiver: receive Cs
	int nBufSize = NUM_EXECS_NAOR_PINKAS * FIELD_SIZE_IN_BYTES;
	BYTE* pBuf = new BYTE[nBufSize];
	m_sockOT.Receive(pBuf, nBufSize);
	
	ZZ* pC = new ZZ[NUM_EXECS_NAOR_PINKAS];
	BYTE* pBufIdx = pBuf;
	for(int i=0, idx=0; i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		ZZFromBytes(pC[i], pBufIdx, FIELD_SIZE_IN_BYTES);
		pBufIdx += FIELD_SIZE_IN_BYTES;

		#ifdef _DEBUG
		cout << "pC[" << i <<"]: " << pC[i] << endl;
		#endif
		
	}


	// compute pk0, pk1
	CBitVector rnd;
	rnd.Create(NUM_EXECS_NAOR_PINKAS*FIELD_SIZE_IN_BITS, m_aSeed, m_nCounter);
	BYTE* pBufRnd = rnd.GetArr();
	ZZ* pK = new ZZ[NUM_EXECS_NAOR_PINKAS];
	ZZ ztmp;
	for(int i=0, idx=0; !m_bStop && i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		ZZFromBytes(ztmp, pBufRnd, FIELD_SIZE_IN_BYTES);
		pBufRnd += FIELD_SIZE_IN_BYTES;
		rem(pK[i], ztmp, q);
	}

	pBufIdx = pBuf;
	ZZ pk0, pk1;

	for(int i=0, idx=0; !m_bStop && i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		// compute pk0, pk1
		if( !m_S.GetBit(i) )
		{
			PowerMod(pk0, g, pK[i], p);
		}
		else
		{
			PowerMod(pk1, g, pK[i], p);

			//pk0 = pC[i]/pk1;
			InvMod(ztmp, pk1, p);
			MulMod(pk0, pC[i], ztmp, p);
		}

		#ifdef _DEBUG
		cout << "pk0[" << i << "]: " << pk0 << endl;
		#endif
		
		// put pk0
		BytesFromZZ(pBufIdx, pk0, FIELD_SIZE_IN_BYTES);
		pBufIdx += FIELD_SIZE_IN_BYTES;

	}


	m_sockOT.Send(pBuf, nBufSize);
	delete [] pC;
	delete [] pBuf;
	
	if( m_bStop ) return;

	// NP receiver: get the g^r0, Enc(M0), g^r2, Enc(M1) 
	int nInputStart = m_pCircuit->GetInputStart(ID_CLIENT);
	int nInputEnd = m_pCircuit->GetInputEnd(ID_CLIENT);

	int nMsgSize = (nInputEnd-nInputStart)/SHA1_BITS + 1;		// in sha1 scale
	int nMsginOT = FIELD_SIZE_IN_BYTES + nMsgSize*SHA1_BYTES;  
	int nBufSize2 = NUM_EXECS_NAOR_PINKAS * nMsginOT * 2;   
	BYTE* pBuf2 = new BYTE[nBufSize2];
	m_sockOT.Receive(pBuf2, nBufSize2);
	
	ZZ w;
	ZZ key;
	BYTE tmp[FIELD_SIZE_IN_BYTES];

	sha1_context sha;
	SHA_BUFFER buf_key;
	
	BYTE** ppMat = new BYTE*[NUM_EXECS_NAOR_PINKAS];
	BYTE* pBufToRead;
	BYTE* pBufMatIdx;

	pBufIdx = pBuf2;
	for(int i=0, idx=0; !m_bStop && i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		ppMat[i] = new BYTE[nMsgSize*SHA1_BYTES];
		
		if( !m_S.GetBit(i))
		{
			pBufToRead = pBufIdx;
			pBufIdx +=  nMsginOT + nMsginOT;
		}
		else
		{
			pBufIdx += nMsginOT;  
			pBufToRead = pBufIdx;
			pBufIdx += nMsginOT; 
		}

		ZZFromBytes(w, pBufToRead, FIELD_SIZE_IN_BYTES);
		pBufToRead += FIELD_SIZE_IN_BYTES;
		PowerMod(key, w, pK[i], p);
		BytesFromZZ(tmp, key, FIELD_SIZE_IN_BYTES);
	 	 
		sha1_starts(&sha);
		sha1_update(&sha, tmp, FIELD_SIZE_IN_BYTES);
		sha1_finish(&sha, (BYTE*) &buf_key);	

		pBufMatIdx=ppMat[i];
		for(int j=0; j<nMsgSize; j++)
		{
			sha1_starts(&sha);
			sha1_update(&sha, (BYTE*) &buf_key, sizeof(buf_key));
			sha1_update(&sha, (BYTE*) &j, sizeof(int)); 
			sha1_finish(&sha, tmp);
			 
			for(int x=0; x<SHA1_BYTES; x++, pBufMatIdx++, pBufToRead++ )
			{
 				*(pBufMatIdx) = *(pBufToRead) ^ tmp[x];
		 	}
		}
	} 
	delete [] pK;
	
	if( m_bStop ) return;


	// IKNP-second step: send the keys for client inputs
	int nInputSize = nInputEnd - nInputStart + 1;
	KEY* pKeys = new KEY[nInputSize*2];
	YAO_WIRE* wire;
	KEY* wirekey;
	
	CBitVector qj;
	qj.Create(NUM_EXECS_NAOR_PINKAS); 

	int j=0; // 0-starting index
	KEY* pKeyIdx = pKeys; 
	
	for(int i=nInputStart; !m_bStop && i<=nInputEnd; i++,j++)
	{
		while( m_nGatesDone < i ) {
			SleepMiliSec(100);
		}

		// compute qj
		for(int r=0; r<NUM_EXECS_NAOR_PINKAS; r++)
		{
			qj.SetBit( r, ppMat[r][j/8] & bitmask[j & 0x7] );
		}
 
		// compute hash
		sha1_starts(&sha);
		sha1_update(&sha,  qj.GetArr(), NUM_EXECS_NAOR_PINKAS/8);
		sha1_update(&sha, (BYTE*)&j, sizeof(int));
		sha1_finish(&sha, (BYTE*)&buf_key);
		
		// y0
		wire = m_pYaoWires+i;
		wirekey = wire->keys + wire->b;
		XOR_KEYP3( pKeyIdx, (&buf_key), wirekey );
		pKeyIdx++;

		// compute qj xor s
		for(int x=0; x<NUM_EXECS_NAOR_PINKAS/8; x++ )
			qj.GetArr()[x] ^=  m_S.GetByte(x);
		
		/*
		#ifdef _DEBUG
		cout << "qj xor s = "; 
		for(int z=0; z<NUM_EXECS_NAOR_PINKAS; z++)
			cout << (int) qj.GetBit(z);
		cout << endl; 
		#endif
		*/

		// y1
		sha1_starts(&sha);
		sha1_update(&sha,  qj.GetArr(), NUM_EXECS_NAOR_PINKAS/8);
		sha1_update(&sha, (BYTE*)&j, sizeof(int));
		sha1_finish(&sha, (BYTE*)&buf_key);
	 
		wirekey = wire->keys + (wire->b^1);
		XOR_KEYP3( pKeyIdx, (&buf_key), wirekey );
		pKeyIdx++;
	}
	m_sockOT.Send( pKeys, nInputSize*sizeof(KEY)*2);

	// clean-up
	
	delete [] pBuf2; 
	for(int i=0; i<NUM_EXECS_NAOR_PINKAS; i++)
	{
		delete [] ppMat[i];
	}
	delete [] ppMat;  
	delete [] pKeys;  
	
	cout << "\not thread ended \n" << flush;
}


void CServer::TestEvaluate()
{
	KEY bogus;
	KEY* p = &bogus;

	for( int i=0; i<m_pCircuit->GetGateStart(); i++ )
	{
		cout << "key[" << i <<"]: ";
		LOG_KEY(m_pYaoWires[i].keys[ m_pGates[i].val] );
	}

	USE_ENCDEC();
	for( int i=m_pCircuit->GetGateStart(), j=0; i<m_nNumGates; i++,j++)
	{
		
		GATE* g = m_pGates + i;
		YAO_WIRE* w = m_pYaoWires + i;
		int left = g->left;
		int right = g->right;

		cout << endl << "id: " << i << " type: " << (int) g->type  << " val: " << g->val << endl;
		cout << "key0: ";
		LOG_KEYP(w->keys);
		cout << "key1: ";
		LOG_KEYP(w->keys+1);
	
		GATE* gl = m_pGates + left;
		GATE* gr = m_pGates + right;  

		YAO_WIRE* wl = m_pYaoWires + left;
		YAO_WIRE* wr = m_pYaoWires + right;

		KEY* l = wl->keys + (gl->val ^ wl->b);
		KEY* r = wr->keys + (gr->val ^ wr->b);  
		KEY* x = w->keys + (g->val ^ w->b); 

		#ifdef USE_XOR_TECH
		if( g->type == G_XOR )
		{
			XOR_KEYP3(p, l, r);
		}
		else
		#endif
		{
			cout << "table=";
			LOG_KEYP(m_pYaoGates[j].table);

			int offset = ( l->val[4] & 1 )*2 + (r->val[4] & 1);
			KEY* c = m_pYaoGates[j].table + offset;
		
			Decrypt(p, c, l, r, i);
		}

		cout << "decrypt = ";
		LOG_KEYP(p);
			
		cout << "goal= "; 
		LOG_KEYP(x);

		#ifdef _DEBUG
		assert( memcmp(p, x, sizeof(KEY)) == 0); 
		#endif
	}
 }

 

