// sicircuit.cpp

#include "sicircuit.h"
#include <cassert>
using namespace std;

BOOL CSICircuit::Create(int nParties, const vector<int>& vParams)
{
	m_nNumParties = nParties;

	// n = nParties
	// params: s0, ..., sn-1, b0, ..., bn-1, rep
	// si: set-size for Pi
	// bi: does Pi get output? 0/1
	// rep:  
	if( vParams.size() < (unsigned) 2*nParties + 1 )
	{
		cout << "Error! This circuit needs " << 2*nParties +1 
			 << "parameters: s_0, ..., s_{n-1}, b_0, ..., b_{n-1}, rep" 
			 << endl
			 << "s_i is a set-size for Pi; "
			 << "b_i is non-zero if Pi wants output; "
			 << "rep is #bits needed for each value."
			 << endl;

		return FALSE;
	}
	
	m_vSizes.resize(m_nNumParties);
	m_vNeedOutput.resize(m_nNumParties);

	// parse params
	for(int i=0; i<nParties; i++)
	{
		m_vSizes[i] = vParams[i];
		m_vNeedOutput[i] = vParams[i+nParties];
	}
	m_nNumVBits = vParams[2*nParties];
	m_vNumVBits.resize(nParties, m_nNumVBits);
	
	// gates for inputs
	m_vInputStart.resize(nParties);
	m_vInputEnd.resize(nParties);

	m_nFrontier = 2;
	for(int i=0; i<nParties; i++)
	{
		m_vInputStart[i] = m_nFrontier;
		m_nFrontier += m_vSizes[i]*m_vNumVBits[i];
		m_vInputEnd[i] =  m_nFrontier - 1;
	} 

	//===========================================================================
	// computes roughly the number of gates beforehand --- need this for allocating space
	// gates for each equality gate:
	// bitwise xor, bitwise negation, AND of each bitwise outputs
	int gates_equality = 3*m_nNumVBits ; 
	int gates_noninput = 0;
	int output_parties = nParties;
	for(int i=0; i<nParties; i++)
	{
		if( !m_vNeedOutput[i] )
			continue;
	  
		for(int j=0; j<nParties; j++)
		{
			if( i < j || !m_vNeedOutput[j] )
			{	
				// equality gates for pairs
				gates_noninput += m_vSizes[i] * m_vSizes[j] * gates_equality;
			}

			// MATCH[i] xoring the output of equality gates
			gates_noninput += m_vSizes[i] * m_vSizes[j];
		}

		// output: AND of all MATCHes
		gates_noninput += m_vSizes[i] * nParties;
	}
	 	
	m_othStartGate = m_nFrontier;
	m_nNumGates = m_nFrontier + gates_noninput;
	m_pGates = new GATE[m_nNumGates];
	m_nNumXORs = 0;


	//================================================================================
	// Initialize the input gates: make room for parent info
	GATE* gate;
	for(int i=0; i<m_othStartGate; i++)
	{
		gate = m_pGates + i;
		gate->type = 0;
		gate->left = gate->right = -1;
		gate->p_num = 0;
		gate->p_ids = 0;
		gate->val = 0;
	}
	m_pGates[1].val = 1;

	
	//================================================================================
	// comparison pairs
	int outsize = (nParties * nParties);
	m_vMatchIdxStart.resize(outsize);
	
	for(int i=0; i<nParties; i++)
	{
		// Party j
		for(int j=i+1; j<nParties; j++)
		{
			if( m_vNeedOutput[i] || m_vNeedOutput[j] )
			{
				AddEqualityBlock(i,j);
			}
		}
	}
		

	//================================================================================
	// output: AND of all MATCHes  
	m_vOutputStart.resize(nParties);
	m_vOutputEnd.resize(nParties);
	 
	for(int i=0; i<nParties; i++)
	{
		ConstructOutput(i);
	}
	
	m_nNumGates = m_nFrontier;
	PatchParentFields();
	return TRUE;
}


void CSICircuit::AddEqualityBlock(int Pi, int Pj)
{
	int sizei = m_vSizes[Pi];
	int sizej = m_vSizes[Pj];
	int sz = sizei*sizej;
	vector<int> vIdxs(sz);
	// i: element of Pi
	int idx=0;
	for(int i=0; i<sizei; i++)
	{
		// j: element of Pj
		for(int j=0; j <sizej; j++)
		{
			 int left = GetInputStart(Pi) + i*m_nNumVBits;
			 int right = GetInputStart(Pj) + j*m_nNumVBits;
			 vIdxs[idx++] = PutEQGate(left, right, m_nNumVBits);
		}
	}
	
	m_vMatchIdxStart[ INDEX(Pi, Pj) ] = m_nFrontier;
	for(int i=0; i<sz; i++)
	{
		PutXORGate(vIdxs[i], 0);
	}

}
 

void CSICircuit::ConstructOutput(int Pi)
{
	if( !m_vNeedOutput[Pi] )
	{
		m_vOutputStart[Pi] = 1;
		m_vOutputEnd[Pi] = 0;
		return;
	}
	
	int in = GetInputStart(Pi);
	int sizei = m_vSizes[Pi];
	
	vector<int> outs(sizei);
	int idx, sizej;
	vector<int> comps;
	vector<int> match;
	
	match.reserve(m_nNumParties);

	for(int i=0; i<sizei; i++)
	{
		match.clear();
			
		// compute match[Pj]
		// match[Pj] = 1 iff i-th elem belongs to Pj's set
		for(int Pj=0; Pj<m_nNumParties; Pj++)
		{
			if( Pj == Pi ) continue;

			// match[Pj] = 1 if at least one comps[j] is 1
			sizej = m_vSizes[Pj];
			comps.reserve(sizej);
			comps.clear();
			
			if( Pi < Pj )
			{
				idx = INDEX(Pi,Pj);
				for(int j=0; j<sizej; j++)
					comps.push_back( m_vMatchIdxStart[idx] + ITEM(Pi,Pj,i,j) );
			 }
			 else
			 {
				idx = INDEX(Pj,Pi);
				for(int j=0; j<sizej; j++)
					comps.push_back( m_vMatchIdxStart[idx] + ITEM(Pj,Pi,j,i) );
			 }

			// match[Pj] = 1 if at least one comps[j] is 1
			match.push_back( PutWideGate(G_XOR, comps) );
		}

		// AND of all matches[Pj]s
		outs[i] = PutWideGate(G_AND, match);
	}

	// gates for outputs
	m_vOutputStart[Pi] = m_nFrontier;
	// XOR(out_i, 0)
	for(int i=0; i<sizei; i++)
	{
		PutXORGate(outs[i], 0);
	}
	m_vOutputEnd[Pi] = m_nFrontier-1;

}

