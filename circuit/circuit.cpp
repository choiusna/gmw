#include "circuit.h"
#include "p2pcircuit.h"
#include "sicircuit.h"
#include "testadd.h"
#include "testmult.h"
#include "socialnet.h"
#include "cloud.h"
#include "../util/config.h"

#include <cmath>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;


//**** You can change the circuit part here if you want a different circuit 
//*************************************************************************//

CCircuit* CREATE_CIRCUIT(int nParties, const string& name, const vector<int>& params)
{
	CCircuit* pCircuit = NULL;

	// change the class if you want a different circuit
	if( name == "p2p" )
	{
		pCircuit = new CP2PCircuit();		
	}
	else if( name == "set-intersection" )
	{
		pCircuit = new CSICircuit();
	}
	else if ( name == "test-add" )
	{
		pCircuit = new CTestAddCircuit();
	}
	else if ( name == "test-mult" )
	{
		pCircuit = new CTestMultCircuit();
	}
	else if ( name == "cloud" )
	{
		pCircuit = new CCloudCircuit();
	}
	else if ( name == "social-net" )
	{
		pCircuit = new CSNetCircuit();
	}
 
	if(pCircuit)  
	{
		BOOL b = pCircuit->Create(nParties, params);
		if( !b )
		{
			delete pCircuit;
			return NULL;
		}
	}

	cout << "#AND=" << pCircuit->GetNumANDs() << endl;
	return pCircuit;
}
//*************************************************************************//




CCircuit::CCircuit()
{
	m_vBufs.reserve( ALLOC_SIZE );
	m_vBufs.push_back( new int [ALLOC_SIZE] );
	m_nBufIdx = 0;
	m_pGates = NULL;

}

int* CCircuit::New(int size)
{
#ifdef _DEBUG
	assert(size <= ALLOC_SIZE);
#endif

	int nToBe = m_nBufIdx + size;
	int *p;
	if( nToBe < ALLOC_SIZE )
	{
		p = m_vBufs.back() + m_nBufIdx;
		m_nBufIdx = nToBe;
	}
	else
	{
		//int alloc_size = max(ALLOC_SIZE, size);
		int alloc_size = ALLOC_SIZE;
		if ( size > alloc_size ) alloc_size = size;
		p = new int [alloc_size];
		m_vBufs.push_back(p);
		m_nBufIdx = size;
	}

	return p;
}
	


void CCircuit::Clearup()
{
	if( m_pGates ) delete [] m_pGates;
	for(UINT i=0; i<m_vBufs.size(); i++ )
	{
		delete [] m_vBufs[i];
	}
}

 
void CCircuit::Evaluate()
{
	// other gates
	int id = m_othStartGate;
	int ch = -1;
	char inleft; 
	char inright; 
	int left, right, type;
	while (id<m_nNumGates)
	{
		type = m_pGates[id].type;

		left = m_pGates[id].left;
		right = m_pGates[id].right;

		if( left < 0 || left >= m_nNumGates || right < 0 || right >= m_nNumGates )
		{
			cout << "error!" << endl;
			cout << id << "(" << type << "): ";
			cout << " left(" << m_pGates[id].left;
			cout << " right(" << m_pGates[id].right;
			cout << endl;
			return;
		}
			
		inleft= m_pGates[left].val; 
		inright = m_pGates[right].val;
		
		if( (inleft & 0xfe) != 0 || (inright & 0xfe ) != 0 )
		{
			cout << "error!" << endl;
			cout << id << "(" << type << "): ";
			cout << " left(" << m_pGates[id].left << ").val=" << (int) inleft;
			cout << " right(" << m_pGates[id].right << ").val=" << (int) inright;
			cout << endl;
			return;
		}
	
		(m_pGates+id)->val = ComputeGate(inleft,inright,type);
		id++;
	}
}

int CCircuit::ComputeGate(int i1, int i2, BYTE type)
{
	int o=0;
	
	// AND gate
	if (type  == G_AND){
		if (i1==1 && i2==1){
			o = 1;
		}
	}

	// XOR gate
	else if (type  == G_XOR){
		if (i1!=i2){
			o = 1;
		}
	}

	return o;
}

const char T_NUMPARTIES = 'n';
const char T_INPUTDESC = 'i';
const char T_OUTPUTDESC = 'o';
const char T_GATESDESC = 'd';
const char T_GATE = 'g';
const char T_VBITS = 'v';
const char T_COMMENT = '%';

void CCircuit::Save(const char* filename, BOOL bVal)
{
	ofstream f(filename);
	 
	int nNumParties = GetNumParties();
	f << T_NUMPARTIES << " " << nNumParties << endl;
	
	int nGateStart = GetGateStart() ;
	int nNumGates = GetNumGates(); 
	int nXORs = GetNumXORs();
	f << T_GATESDESC  << " " << nNumGates << " " << nGateStart << " " << nXORs << endl;
	for(int i=0; i<nNumParties; i++)
	{
		f << T_INPUTDESC << " " << i << " " << GetInputStart(i) << " " << GetInputEnd(i) << endl;
	}
	for(int i=0; i<nNumParties; i++)
	{
		f << T_OUTPUTDESC << " " << i << " " << GetOutputStart(i) << " " << GetOutputEnd(i) << endl;
	}
	for(int i=0; i<nNumParties; i++)
	{
		f << T_VBITS << " " << i << " " << GetNumVBits(i) << endl;
	}
 	
	GATE* g;
	for(int i=0; i<nNumGates; i++)
	{
		g = m_pGates + i;

		f << T_GATE << " " << i 
					<< " " << (int) g->type 
					<< " " <<  g->left 
					<< " " << g->right;

		if( i >= 2 )
		{
			f << " " << g->p_num;
		
			for( int j=0; j<g->p_num; j++ )
			{
				f << " " << g->p_ids[j];
			}
		}
		else
		{
			f << " 0";
		}

		if( bVal ) f << " " << (int) g->val;
		f << endl;
	}
}
  
void CCircuit::SaveBin(const char* filename)
{
	ofstream f(filename, ios::out|ios::binary);
	
	f.write((const char*)&m_nNumParties, sizeof(int));
	f.write((const char*)&m_othStartGate, sizeof(int));
	f.write((const char*)&m_nNumGates, sizeof(int));
	f.write((const char*)&m_nNumXORs, sizeof(int));
	for(int i=0; i<m_nNumParties; i++)
	{
		f.write((const char*)&m_vInputStart[i], sizeof(int));
		f.write((const char*)&m_vInputEnd[i], sizeof(int));
	}
	for(int i=0; i<m_nNumParties; i++)
	{
		f.write((const char*)&m_vOutputStart[i], sizeof(int));
		f.write((const char*)&m_vOutputEnd[i], sizeof(int));
	}
	for(int i=0; i<m_nNumParties; i++)
	{
		f.write((const char*)&m_vNumVBits[i], sizeof(int));
	}

	GATE* g;

	int zero=0;
	for(int i=0; i<1; i++)
	{
		f.write((const char*) &zero, sizeof(int));
	}
	
	for(int i=2; i<m_othStartGate; i++)
	{
		g = m_pGates + i;
		f.write((const char*) &g->p_num, sizeof(int));
		if( g->p_num > 0 )
			f.write((const char*) g->p_ids, sizeof(int)*g->p_num); 
	}

	for(int i=m_othStartGate; i<m_nNumGates; i++)
	{
		g = m_pGates + i;
		f.write((const char*)g, sizeof(GATE)-sizeof(int));
		if( g->p_num > 0 )
			f.write((const char*) g->p_ids, sizeof(int)*g->p_num); 
	}
}

BOOL CCircuit::Load(const char* filename)
{
	ifstream f(filename, ios::in|ios::binary);
	if( !f.is_open() ) return FALSE;

	f.seekg (0, ios::end);
	int size = (int) f.tellg();
	f.seekg(0, ios::beg);
	
	char* pText = new char[size+1];
	f.read(pText, size);
	pText[size] = 0;

	istringstream is(pText);  
	string sLine;
	for( int nLineNum = 1; is.good(); nLineNum++ )
	{ 	
		getline(is, sLine);
		if( sLine.empty() ) continue;

		istringstream is(sLine);

		string sParam;
		is >> sParam;
		if( is.bad() )
		{
			cout  << nLineNum << ": " << sLine << endl;
			cout << " error in parsing the parameter" << endl;
			return FALSE;
		}
		 switch(sParam[0])
		{
 		case T_GATE: 
			{
				int id, type, left, right, num, par;
				is >> id >> type >> left >> right >> num;
				GATE* g = m_pGates + id;
				g->type = (BYTE) type;
				g->left  = left;
				g->right = right;
				g->p_num = num;
				if( num > 0 )
				{
					g->p_ids = New(num);
					for(int k=0; k<num; k++)
					{
						is >> par;
						g->p_ids[k] = par;
					}
				}
			}
			break;
		case T_NUMPARTIES: 
			{
				is >> m_nNumParties; 
				m_vInputStart.resize(m_nNumParties);
				m_vInputEnd.resize(m_nNumParties);
				m_vOutputStart.resize(m_nNumParties);
				m_vOutputEnd.resize(m_nNumParties);
				m_vNumVBits.resize(m_nNumParties);

			}
			break;
		case T_GATESDESC:
			{
				is >> m_nNumGates >> m_othStartGate >> m_nNumXORs;

				m_pGates = new GATE[m_nNumGates];
				for(int i=0; i<m_othStartGate; i++ )
				{
					m_pGates[i].left = m_pGates[i].right = -1;
					m_pGates[i].p_num = 0;
				}
			}
		break;
		case T_INPUTDESC:
			{
				int id, s, e;
				is >> id >> s >> e;
				m_vInputStart[id] = s;
				m_vInputEnd[id] = e;
			}
			break;
		case T_OUTPUTDESC:
			{
				int id, s, e;
				is >> id >> s >> e;
				m_vOutputStart[id] = s;
				m_vOutputEnd[id] = e;
			}
			break;
		case T_VBITS:
			{ 
				int id, v;
				is >> id >> v;
				m_vNumVBits[id] = v;
			}
			break;
		case T_COMMENT:
			break;
		default:
		 	cout << "unrecognized command in line (skip)" << nLineNum << ": " << sLine << endl;
		}
	
 	}

	delete[] pText;
	return true;
}

 

BOOL CCircuit::LoadBin(const char* filename)
{
	ifstream f(filename, ios::in|ios::binary);
	if(!f.is_open()) return FALSE;

	f.read((char*)&m_nNumParties, sizeof(int));
	f.read((char*)&m_othStartGate, sizeof(int));
	f.read((char*)&m_nNumGates, sizeof(int));
	f.read((char*)&m_nNumXORs, sizeof(int));
	 
	m_vInputStart.resize(m_nNumParties);
	m_vInputEnd.resize(m_nNumParties);
	m_vOutputStart.resize(m_nNumParties);
	m_vOutputEnd.resize(m_nNumParties);
	m_vNumVBits.resize(m_nNumParties);

	for(int i=0; i<m_nNumParties; i++)
	{
		f.read((char*)&m_vInputStart[i], sizeof(int));
		f.read((char*)&m_vInputEnd[i], sizeof(int));
	}
	for(int i=0; i<m_nNumParties; i++)
	{
		f.read((char*)&m_vOutputStart[i], sizeof(int));
		f.read((char*)&m_vOutputEnd[i], sizeof(int));
	}

	for(int i=0; i<m_nNumParties; i++)
	{
		f.read((char*)&m_vNumVBits[i], sizeof(int));
	}

	if( !m_pGates ) m_pGates = new GATE[m_nNumGates];
	GATE* g;
	for(int i=0; i<m_othStartGate; i++ )
	{
		g = m_pGates + i;
		g->left = g->right = -1;
		f.read((char*)&g->p_num, sizeof(int));
		if( g->p_num > 0 )
		{
			g->p_ids = New(g->p_num);
			f.read((char*) g->p_ids, sizeof(int)*g->p_num);
		}
	}
	
	for(int i=m_othStartGate; i<m_nNumGates; i++)
	{
		g = m_pGates + i;
		f.read((char*)g, sizeof(GATE)-sizeof(int));
		if( g->p_num > 0 )
		{
			g->p_ids = New(g->p_num);
			f.read((char*) g->p_ids, sizeof(int)*g->p_num);
		}
	}

	return TRUE;
}

BOOL CCircuit::LoadHeaderBin(const char* filename)
{
	ifstream f(filename, ios::in|ios::binary);
	if(!f.is_open()) return FALSE;

	f.read((char*)&m_nNumParties, sizeof(int));
	f.read((char*)&m_othStartGate, sizeof(int));
	f.read((char*)&m_nNumGates, sizeof(int));
	f.read((char*)&m_nNumXORs, sizeof(int));
	 
	m_vInputStart.resize(m_nNumParties);
	m_vInputEnd.resize(m_nNumParties);
	m_vOutputStart.resize(m_nNumParties);
	m_vOutputEnd.resize(m_nNumParties);
	m_vNumVBits.resize(m_nNumParties);

	for(int i=0; i<m_nNumParties; i++)
	{
		f.read((char*)&m_vInputStart[i], sizeof(int));
		f.read((char*)&m_vInputEnd[i], sizeof(int));
	}
	for(int i=0; i<m_nNumParties; i++)
	{
		f.read((char*)&m_vOutputStart[i], sizeof(int));
		f.read((char*)&m_vOutputEnd[i], sizeof(int));
	}
	for(int i=0; i<m_nNumParties; i++)
	{
		f.read((char*)&m_vNumVBits[i], sizeof(int));
	}
	if( !m_pGates ) m_pGates = new GATE[m_nNumGates];

	return TRUE;
}


BOOL CCircuit::IsEqual(CCircuit* c)
{
	if( GetNumGates() != c->GetNumGates() )
	{
		cout << "num_gates mismatch:" << GetNumGates() << ", " << c->GetNumGates() << endl;
		return FALSE;
	}
	
	if( GetNumParties() != c->GetNumParties() )
	{
		cout << "num_parties mismatch" << endl;
		return FALSE;
	}
	 
	for(int i=0; i<GetNumParties(); i++ )
	{
		if( GetInputStart(i) != c->GetInputStart(i) ) 
		{
			cout << "input start mismatch: " << i << endl;
			return FALSE;
		}
		
		if( GetInputEnd(i) != c->GetInputEnd(i) )
		{
			cout << "input end mismatch " << i << endl;
			return FALSE;
		}

		if( GetOutputStart(i) != c->GetOutputStart(i) ) 
		{
			cout << "output start mismatch: " << i << endl;
			return FALSE;
		}
		
		if( GetOutputEnd(i) != c->GetOutputEnd(i) )
		{
			cout << "output end mismatch " << i << endl;
			return FALSE;
		}

		if( GetNumVBits(i) != c->GetNumVBits(i) )
		{
			cout << "num_vbits mismatch " << i << endl;
			return FALSE;
		}

	}
	
	if( GetGateStart() != c->GetGateStart() ) 
	{
		cout << "gate start mismatch" << endl;
		return FALSE;
	}

	if( GetNumXORs() != c->GetNumXORs() )
	{
		cout << "num_xors mismatch" << endl;
		return FALSE;
	}


	int n = GetNumGates();

	GATE *a, *b;
	for(int i=0; i<n; i++)
	{
		a = m_pGates + i; 
		b = c->m_pGates + i;
		 
		if( a->type != b->type )
		{ 
			cout << "type mismatch: " << i << endl;
			return FALSE;
		}
		 
		if( a->left != b->left )
		{
			cout << "left mismatch: " << i << endl;
			return FALSE;
		}

		if( a->right != b->right )
		{
			cout << "right mismatch: " << i << endl;
			return FALSE;
		}

		if( a->p_num != b->p_num )
		{
			cout << "p_num mismatch: "<< i << endl;
			return FALSE;
		}

		for(int j=0; j < a->p_num; j++ )
		{
			if( a->p_ids[j] != b->p_ids[j] )
			{
				cout << "p_ids mismatch: " << i << " " << j << endl;
				return FALSE;
			}
		}
	}

	return TRUE;
}





class CLoadCircuit : public CCircuit
{
private:
	BOOL Create(int nParties, const vector<int>& params){ return FALSE;}
};

CCircuit* LOAD_CIRCUIT_TXT(const char* filename)
{
	CCircuit* pCircuit = new CLoadCircuit();
	pCircuit->Load(filename);
	return pCircuit;
}

CCircuit* LOAD_CIRCUIT_BIN(const char* filename)
{
	CCircuit* pCircuit = new CLoadCircuit();
	pCircuit->LoadBin(filename);
	return pCircuit;
}

CCircuit* LOAD_CIRCUIT_HEADER_BIN(const char* filename)
{
	CCircuit* pCircuit = new CLoadCircuit();
	if(!pCircuit->LoadHeaderBin(filename) )
	{
		delete pCircuit;
		return 0;
	}

	return pCircuit;
}



void TEST_CIRCUIT(const vector<const char*>& configs, BOOL bLog)
{
	unsigned size = (unsigned) configs.size();

	vector<CConfig*> cfs(size);

	for(unsigned i=0; i<size; i++)
	{
		cfs[i] = new CConfig();
		cfs[i]->Load( configs[i] );
	}

	CCircuit* circ;
	if( cfs[0]->GetCircFileName().empty() )
	{
		circ = CREATE_CIRCUIT( size, cfs[0]->GetCircCreateName(), cfs[0]->GetCircCreateParams() );
	}
	else
	{
		circ = LOAD_CIRCUIT_BIN(cfs[0]->GetCircFileName().c_str());
	}
		
	GATE* gates = circ->Gates();
	gates[0].val = 0;
	gates[1].val = 1;
	for(unsigned i=0; i<size; i++)
	{
		int bits = circ->GetNumVBits(i);
		int start = circ->GetInputStart(i);
		int end = circ->GetInputEnd(i);
		vector<int> in;
		cfs[i]->GetInput(in);

		int j=start;
		for( unsigned u=0; u<in.size() && j<= end; u++ ) 
		{
			for(int k=0; k<bits; k++ )
			{
				int mask = (1 << k );
				gates[j].val = (char) !!(in[u] & mask);
				j++;
			}
		}
	}

	circ->Evaluate();
	if( bLog )
		circ->Save("eval.txt", TRUE);
	circ->PrintOutputs();
}



static void PrintOutput(const vector<int>& vOutput)
{
	cout << "output:" << endl;
	cout << "(binary)";
	for( UINT i=0; i<vOutput.size(); i++ )
	{
		cout << " " << (int) vOutput[i];
	}
	cout << endl;

	ZZ out;
	out.SetSize((int)vOutput.size());
	
	cout << "(numeric:big-endian) ";

	int size = (int) vOutput.size();
	for( int i=0; i<size; i++ )
	{
		if( bit(out,i) != vOutput[i] )
			SwitchBit(out,i);
	}
	cout << out << endl;


	cout << "(numeric:little-endian) ";

	for( int i=0; i<size; i++ )
	{
		if( bit(out,i) != vOutput[size-i-1] )
			SwitchBit(out,i);
	}
	cout << out << endl;
}



void CCircuit::PrintOutputs()
{
	for(int i=0; i<m_nNumParties; i++)
	{
		cout << "Party " << i << endl;

		int o_start = GetOutputStart(i);
		int o_end = GetOutputEnd(i);
		int o_size = o_end - o_start + 1;
		if( o_end >= o_start )
		{
			vector<int> vOutput(o_size);
			for(int j=o_start, k=0; j<=o_end; j++, k++)
			{
				vOutput[k] = m_pGates[j].val;
			}
			
			PrintOutput(vOutput);
		}
	}
}


int CCircuit::PutANDGate(int a, int b)
{
	int out = m_nFrontier++;
	GATE* gate;
	
	gate = m_pGates + out;
	gate->type = G_AND;
	gate->p_num = 0;
	gate->p_ids = 0;

	gate->left = a;
	m_pGates[gate->left].p_num++;
	gate->right = b;
	m_pGates[gate->right].p_num++;
	
	return out;
}


int CCircuit::PutXORGate(int a, int b)
{
	int out = m_nFrontier++;
	GATE* gate;
	
	gate = m_pGates + out;
	gate->type = G_XOR;
	m_nNumXORs++;
	gate->p_num = 0;
	gate->p_ids = 0;

	gate->left = a;
	m_pGates[gate->left].p_num++;
	gate->right = b;
	m_pGates[gate->right].p_num++;
	
	return out;
}



int CCircuit::PutEQGate(int a, int b, int rep)
{
	// xor1 = XOR(left, right)
	int xor1 = m_nFrontier;
	for(int i=0; i<rep; i++)
	{
		PutXORGate(a+i, b+i);
	}

	// xor2 = XOR(xor1, 1)
	vector<int> xor2(rep);
	for(int i=0; i<rep; i++)
	{
		xor2[i] = PutXORGate(xor1+i, 1);
	}

	// AND of all xorid2's
	return PutWideGate(G_AND, xor2);
}



int CCircuit::PutGTGate(int a, int b, int rep)
{
	int ci=0, ci1, ac, bc, acNbc;
	for(int i=0; i<rep; i++, ci = ci1 )
	{
		ac = PutXORGate(a+i, ci);
		bc = PutXORGate(b+i, ci);
		acNbc = PutANDGate(ac, bc);
		ci1 = PutXORGate(a+i, acNbc);
	}

	return ci;
}

int CCircuit::PutGEGate(int a, int b, int rep)
{
	int cl1 = PutGTGate(b,a,rep);
	int out = PutXORGate(cl1, 1); 
	return out;
}


int CCircuit::PutELM0Gate(int val, int b, int rep)
{
	int out = m_nFrontier;
	for(int i=0; i<rep; i++)
	{
		PutANDGate(val+i, b);
	}
	return out;
}


int CCircuit::PutELM1Gate(int a, int s, int rep)
{
	// MUX GATE (a, b=1, s)
	return PutMUXGate(a, 1, s, rep, TRUE);
} 
 
int CCircuit::PutMUXGate(int a, int b, int s, int rep, BOOL bConstb)
{
	vector<int> sab(rep);
	int ab;

	for(int i=0; i<rep; i++)
	{
		
		if( bConstb) ab = PutXORGate(a+i, b);  
		else ab = PutXORGate(a+i, b+i);

		sab[i] = PutANDGate(s, ab);
	}

	int out = m_nFrontier;
	int mi;
	for(int i=0; i<rep; i++ )
	{
		if( bConstb )	mi = PutXORGate(b, sab[i]); 
		else mi = PutXORGate(b+i, sab[i]);
	}

	return out;
}

 
int CCircuit::PutAddGate(int a, int b, int rep, BOOL bCarry)
{
	// left + right mod (2^Rep)
	// Construct C[i] gates
	vector<int> C(rep);	 	
	int axc, bxc,  acNbc;
	
	C[0] = 0;
	
	int i=0;
	for( ; i<rep-1; i++)
	{
		//===================
		// New Gates
		// a[i] xor c[i]
		axc = PutXORGate(a+i, C[i]);

		// b[i] xor c[i]
		bxc = PutXORGate(b+i, C[i]);
		 
		// axc AND bxc
		acNbc = PutANDGate(axc, bxc);
		
		// C[i+1]
		C[i+1] = PutXORGate( C[i], acNbc );
	}

	if( bCarry )
	{
		axc = PutXORGate(a+i, C[i]);

		// b[i] xor c[i]
		bxc = PutXORGate(b+i, C[i]);
		 
		// axc AND bxc
		acNbc = PutANDGate(axc, bxc);
	}


	// Construct a[i] xor b[i] gates
	vector<int> AxB(rep);	
	for(int i=0; i<rep; i++)
	{
		// a[i] xor b[i]
		AxB[i] = PutXORGate(a+i, b+i);
	} 

	// Construct Output gates of Addition
	int out = m_nFrontier;
	for(int i=0; i<rep; i++)
	{
		PutXORGate(C[i], AxB[i]);
	}
	
	if( bCarry )
		PutXORGate( C[i], acNbc );
 
	return out;
}


int CCircuit::PutSubGate(int a, int b, int rep)
{
	vector<int> C(rep);	 	
	int axc, bxc,  acNbc;
	
	C[0] = 1;
	
	int i=0;
	for( ; i<rep-1; i++)
	{
		//===================
		// New Gates
		// a[i] xor c[i]
		axc = PutXORGate(a+i, C[i]);

		// b[i] xor c[i]
		bxc = PutXORGate(b+i, C[i]);
		 
		// axc AND bxc
		acNbc = PutANDGate(axc, bxc);
		
		// C[i+1]
		C[i+1] = PutXORGate( a+i, acNbc );
	}

 
	int axb = m_nFrontier;
	for(int i=0; i<rep; i++)
	{
		// a[i] xor b[i]
		PutXORGate(a+i, b+i);
	}

	int abc = m_nFrontier;
	for(int i=0; i<rep; i++)
	{
		PutXORGate(axb+i, C[i]);
	} 

	// Construct Output gates of Addition
	int out = m_nFrontier;
	for(int i=0; i<rep; i++)
	{
		PutXORGate(abc+i, 1);
	}
	 
	return out;
}


int CCircuit::PutMultGate(int a, int b, int rep)
{
	vector<int> vAdds(rep);
	for(int i=0; i<rep; i++)
	{
		// c[i] = (a << 2^i) AND b[i]
		vAdds[i] = m_nFrontier;	
		
		for(int j=0; j<rep; j++)
		{
			if( j < i )  PutXORGate(0,0);
			else PutANDGate(a+j-i, b+i); 
		}
	}

	int out = PutWideAddGate(vAdds, rep);
 	return out;
}


int CCircuit::PutWideAddGate(vector<int>& ins, int rep)
{
	// build a balanced binary tree
	vector<int>& survivors = ins;
	 
	while( survivors.size() > 1 )
	{
		unsigned j=0;
		for(unsigned i=0; i<survivors.size(); )
		{
			if( i+1 >=  survivors.size() )
			{
				survivors[j++] = survivors[i++];
			}
			else
			{
			 	survivors[j++] = PutAddGate( survivors[i], survivors[i+1], rep);
				i+=2;
			} 	 
		}
		survivors.resize(j);
	}

	return survivors[0];
}

int CCircuit::PutWideGate(BYTE type, vector<int>& ins)
{
	// build a balanced binary tree
	vector<int>& survivors = ins;
	 
	while( survivors.size() > 1 )
	{
		unsigned j=0;
		for(unsigned i=0; i<survivors.size(); )
		{
			if( i+1 >=  survivors.size() )
			{
				survivors[j++] = survivors[i++];
			}
			else
			{
				if( type == G_AND )
					survivors[j++] = PutANDGate( survivors[i], survivors[i+1]);
				else
					survivors[j++] = PutXORGate( survivors[i], survivors[i+1]);

				i+=2;
			} 	 
		}
		survivors.resize(j);
	}

	return survivors[0];
}


void CCircuit::PatchParentFields()
{
	vector<int> counter_map(m_nNumGates,0);
	GATE* gate;
	GATE* c;
	int child;
	
	for(int i=m_othStartGate; i<m_nNumGates; i++)
	{
		gate = m_pGates + i;

		child = gate->left;
	 
		if( child >= 2 )
		{
			c = m_pGates + child;
			if( counter_map[child] == 0 )
			{
				c->p_ids = New(c->p_num);
			}
			c->p_ids[ counter_map[child]++ ] = i;
		}

		child = gate->right;

		if( child >= 2 )
		{
			c = m_pGates + child;
			if( counter_map[child] == 0 )
			{
				c->p_ids = New(c->p_num);
			}
			c->p_ids[ counter_map[child]++ ] = i;
		}
	}
}
