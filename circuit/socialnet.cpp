// socialnet.cpp

#include "socialnet.h"
#include <cassert>
#include <cmath>
using namespace std;

BOOL CSNetCircuit::Create(int nParties, const vector<int>& vParams)
{
	m_nNumParties = nParties;

	// parse params
	// params: width, depth, rep
	if( vParams.size() < (unsigned) 2 )
	{
		cout << "Error! This circuit needs " << 2  
		  	 << "parameter: type bits-in-integer" << endl
			 << "type=0 for find-all-close-matches" << endl
			 << "type=1 for find-closest-match" <<endl
			 << "type=2 for find-best-resource" << endl;
		return FALSE;
	}

	m_nType = vParams[0];
	m_nRep = vParams[1];
	m_vNumVBits.resize(m_nNumParties, m_nRep);

	m_nTotItems = (nParties-1);
	m_nIdxRep = (int) ceil(log((double)m_nTotItems+1)/log(2.0)); 

	m_nCntRep = (int) ceil(log(double(m_nRep))/log(2.0));
	m_nCntRange = 1 << m_nCntRep;

	// gates for inputs:  
	m_vInputStart.resize(nParties);
	m_vInputEnd.resize(nParties);

	m_nFrontier = 2;
	for(int i=0; i<nParties-1; i++)
	{
		m_vInputStart[i] = m_nFrontier;
		m_nFrontier += m_nRep*3;
		m_vInputEnd[i] =  m_nFrontier - 1;
	}

	m_vInputStart[nParties-1] = m_nFrontier;
	m_nFrontier += m_nRep*4;
	m_vInputEnd[nParties-1] =  m_nFrontier - 1;
	  
	//===========================================================================
	// computes roughly the number of gates beforehand --- need this for allocating space
 	int gates_ge =  4*m_nRep+1;
	int gates_mux = 3*m_nRep;
	int gates_add = 6*m_nRep;
	int gates_sub = 6*m_nRep;
	int gates_dst = 2*gates_ge + 4*gates_mux + 2*m_nRep + 2*gates_sub + gates_add;
	
	int gates_cnt = 2*gates_add;
	int gates_cap = m_nRep;
	int gates_in = m_nRep;
	int gates_elm = 3*m_nRep + gates_cap + gates_cnt + gates_in;

	int gates_idx_mux = 3*m_nIdxRep;
	int gates_tournament =  gates_ge + gates_mux + gates_idx_mux;
	int gates_noninput = m_nTotItems*( m_nIdxRep + gates_dst + gates_elm + gates_tournament );
	 	
	m_othStartGate = m_nFrontier;
	m_nNumGates = m_nFrontier + gates_noninput;
	m_pGates = new GATE[m_nNumGates];
	m_nNumXORs = 0;

	//============================================================================
	// Now construct the circuit
	SetupInputGates();

	if( m_nType ==  e_AllCloseMatches  )
	{
		CreateFindAllCloseMatches();
	}
	else
	{
		m_vELMs.resize(m_nTotItems);
		for(int i=0; i<m_nTotItems; i++)
		{
			m_vELMs[i] = PutELMGate(i);

			cout << "elm " << i << "=" << m_vELMs[i] << endl;
		}
		
		m_vIdxs.resize(m_nTotItems);
		for(int i=0; i<m_nTotItems; i++)
		{
			m_vIdxs[i] = PutIdxGate(i+1);

			cout << "idx " << i << "=" << m_vIdxs[i] << endl;
		}
		PutLayerB();
		PutOutputLayer();
	}
	PatchParentFields();

	return TRUE;
}


void CSNetCircuit::CreateFindAllCloseMatches()
{

	vector<int> comp(m_nTotItems), in(m_nTotItems);
	int delta = m_vInputStart[m_nNumParties-1] + 3*m_nRep;

	//cout << "delta=" << delta << endl;


	int dst;
	for(int i=0; i<m_nTotItems; i++)
	{
		dst = PutDSTGate(i);	
		comp[i] = PutGEGate(delta, dst, m_nRep);
		in[i] = PutINGate(i);

		/*
		cout << " i=" << i
			 << " dst=" << dst
			 << " comp=" << comp[i]
			 << " in=" << in[i]
			 << endl;
		*/
	}

	int out = m_nFrontier;
	for(int i=0; i<m_nTotItems; i++)
	{
		PutANDGate(comp[i], in[i]);
	}

	m_vOutputStart.resize(m_nNumParties, 1);
	m_vOutputEnd.resize(m_nNumParties, 0);
	 
	m_vOutputStart[m_nNumParties-1] = out;
	m_vOutputEnd[m_nNumParties-1] = m_nFrontier-1;

	m_nNumGates = m_nFrontier;
}

void CSNetCircuit::SetupInputGates()
{
	GATE* gate;
 	 
	for(int i=0; i<m_othStartGate; i++)
	{
		gate = m_pGates + i;
		gate->p_ids = 0;
		gate->p_num = 0;
		gate->left = -1;
		gate->right = -1;
		gate->type = 0;
		gate->val = 0;
	}

	// constant 1
	m_pGates[1].val = 1;
}	 

 
int CSNetCircuit::PutIdxGate(int r)
{
	int start = m_nFrontier;
	int digit;
  	for(int j=0; j<m_nIdxRep; j++)
	{
		digit = (r >> j) & 1;
		PutXORGate(digit,0);
	}
	return start;
}


int CSNetCircuit::PutDSTGate(int r)
{
	int lr1 = 2+m_nRep*3*r;
	int lr2 = lr1+m_nRep;
	
	int l1 = m_vInputStart[m_nNumParties-1];
	int l2 = l1 + m_nRep;

	int c1 = PutGEGate(lr1, l1, m_nRep);
	int c2 = PutGEGate(lr2, l2, m_nRep);
	
	int a1 = PutMUXGate(lr1, l1, c1, m_nRep);
	int b1 = PutMUXGate(lr1, l1, PutXORGate(1,c1), m_nRep); 
	
	int a2 = PutMUXGate(lr2, l2, c2, m_nRep);
	int b2 = PutMUXGate(lr2, l2, PutXORGate(1,c2), m_nRep);
	
	int s1 = PutSubGate(a1, b1, m_nRep);
	int s2 = PutSubGate(a2, b2, m_nRep);
	
	int dst = PutAddGate(s1,s2, m_nRep);
	 
	
	cout << "DST(" << r << ")=" << dst << endl;
	cout << "lr1=" << lr1 << endl;
	cout << "lr2=" << lr2 << endl;
	cout << "l1=" << l1 << endl;
	cout << "l2=" << l2 << endl;
	cout << ": GE(lr1,l1)=" << c1 << endl;
	cout << ": GE(lr2,l2)=" << c2 << endl;
	cout << ": MUX(lr1,l1,c1)=" << a1 << endl;
	cout << ": MUX(lr2,l2,c2)=" << a2 << endl;
	cout << ": MUX(lr1,l1,!c1)=" << b1 << endl;
	cout << ": MUX(lr2,l2,!c2)=" << b2 << endl;
	cout << ": SUB(a1,b1)=" << s1 << endl;
	cout << ": SUB(a2,b2)=" << s2 << endl;
	cout << endl;
	
	return dst;
}
  
int	CSNetCircuit::PutCAPGate(int r)
{
	int hr = 2 + 3*m_nRep*r + 2*m_nRep;
	int h = m_vInputStart[m_nNumParties-1] + 2*m_nRep;

	int out = m_nFrontier;
	for(int i=0; i<m_nRep; i++)
		PutANDGate(hr+i, h+i);
	
	return out;
}



int	CSNetCircuit::PutCNTGate(int a)
{
	vector<int> ins(m_nCntRange);
	for(int i=0; i<m_nRep; i++)
		ins[i] = a+i;

	for(int i=m_nRep; i<m_nCntRange; i++)
		ins[i] = 0;

	int out = PutCNTGate(ins, 1);

	return out;
}


int CSNetCircuit::PutCNTGate(const vector<int>& ins, int rep)
{
	if( ins.size() == 1 ) return ins[0];

	assert( ins.size() %2 == 0 );

	vector<int> ins2((ins.size()+1)/2);

	for(unsigned i=0, j=0; i<ins.size(); )
	{
		ins2[j++] = PutAddGate(ins[i], ins[i+1], rep, TRUE);
		i+=2;
	}

	return PutCNTGate(ins2, rep+1);
}

int	CSNetCircuit::PutINGate(int r)
{
	int h = m_vInputStart[m_nNumParties-1] + 2*m_nRep;
	int hr = 2 + 3*m_nRep*r + 2*m_nRep;

	
	int b1, aNb1;
	vector<int> s(m_nRep);
	for(int i=0; i<m_nRep; i++)
	{
		b1 = PutXORGate(hr+i, 1);
		aNb1 = PutANDGate(h+i, b1);
		s[i] = PutXORGate(1, aNb1);
	}
	
	return PutWideGate(G_AND, s);
}



void CSNetCircuit::PutLayerB()
{
	// build a balanced binary tree
	int cmp;
	while( m_vELMs.size() > 1 )
	{
		unsigned j=0;
		for(unsigned i=0; i<m_vELMs.size(); )
		{
			if( i+1 >=  m_vELMs.size() )
			{
				m_vELMs[j] = m_vELMs[i];
				m_vIdxs[j] = m_vIdxs[i];
				i++;
				j++;
			}
			else
			{
				if( m_nType == e_ClosetMatch )
				{
					cmp = PutGEGate(m_vELMs[i+1], m_vELMs[i], m_nRep);	// elm[i] <= elm[i+1] ?
					m_vELMs[j] = PutMUXGate(m_vELMs[i], m_vELMs[i+1], cmp, m_nRep);
					m_vIdxs[j] = PutMUXGate(m_vIdxs[i], m_vIdxs[i+1], cmp, m_nIdxRep);
				}
				else	// best resource
				{
					cmp = PutGEGate(m_vELMs[i], m_vELMs[i+1], m_nCntRep);	// elm[i] >= elm[i+1] ?
					m_vELMs[j] = PutMUXGate(m_vELMs[i], m_vELMs[i+1], cmp, m_nCntRep);
					m_vIdxs[j] = PutMUXGate(m_vIdxs[i], m_vIdxs[i+1], cmp, m_nIdxRep);
				}
				i+=2;
				j++;
			} 	 
		}
		m_vELMs.resize(j);
		m_vIdxs.resize(j);
	}
}
 
 
void CSNetCircuit::PutOutputLayer()
{
	m_vOutputStart.resize(m_nNumParties, 1);
	m_vOutputEnd.resize(m_nNumParties, 0);
	 
	m_vOutputStart[m_nNumParties-1] = m_vIdxs[0];
	m_vOutputEnd[m_nNumParties-1] = m_vIdxs[0]+m_nIdxRep-1;

	m_nNumGates = m_nFrontier;
}

int CSNetCircuit::PutELMGate(int r)
{
	if( m_nType == e_ClosetMatch  )
	{
		// find-closest-match
		int dst = PutDSTGate(r);
		int in = PutINGate(r);
		int elm = PutELM1Gate(dst,in,m_nRep);

		cout << "r=" << r << " dst=" << dst <<" in=" << in << " elm=" << elm << endl;

		return elm;
	}
	else
	{
		// find-best-resource
		int dst = PutDSTGate(r);
		int delta = m_vInputStart[m_nNumParties-1] + 3*m_nRep;
		int d = PutGEGate(delta, dst, m_nRep);
		int cap = PutCAPGate(r);
		int cnt = PutCNTGate(cap);
		int elm = PutELM0Gate(cnt, d, m_nCntRep);

		cout	<< "r=" << r << " dst=" << dst << " delta=" << delta 
				<< " ge=" << d << " cap=" << cap 
				<<" cnt=" << cnt << " elm=" << elm << endl;
		return elm;
	}
}

