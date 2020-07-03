// cloud.cpp

#include "cloud.h"
#include <cassert>
#include <cmath>
using namespace std;

BOOL CCloudCircuit::Create(int nParties, const vector<int>& vParams)
{
	m_nNumParties = nParties;

	// parse params
	// params: width, depth, rep
	if( vParams.size() < (unsigned) 3 )
	{
		cout << "Error! This circuit needs " << 3  
			<< "parameters: items-per-server is-higest-quality bits-in-integer"
			 << endl;
		return FALSE;
	}

	m_nItems = vParams[0];
	m_bHQ = vParams[1];
	m_nRep = vParams[2];
	m_vNumVBits.resize(m_nNumParties, m_nRep);

	m_nTotItems = m_nItems * (nParties-1);
	m_nIdxRep = (int) ceil(log((double)m_nTotItems+1)/log(2.0)); 

	// gates for inputs:  
	m_vInputStart.resize(nParties);
	m_vInputEnd.resize(nParties);

	m_nFrontier = 2;
	for(int i=0; i<nParties-1; i++)
	{
		m_vInputStart[i] = m_nFrontier;
		m_nFrontier += m_nRep*m_nItems*2;
		m_vInputEnd[i] =  m_nFrontier - 1;
	}

	m_vInputStart[nParties-1] = m_nFrontier;
	m_nFrontier += m_nRep*2;
	m_vInputEnd[nParties-1] =  m_nFrontier - 1;
	  
	//===========================================================================
	// computes roughly the number of gates beforehand --- need this for allocating space
 	int gates_ge =  4*m_nRep+1;
	int gates_mux = 3*m_nRep;
	int gates_idx_mux = 3*m_nIdxRep;
	int gates_elm = gates_mux + 2*gates_ge + 1;
	int gates_tournament =  gates_ge + gates_mux + gates_idx_mux;
	int gates_noninput = m_nTotItems*( m_nIdxRep + gates_elm + gates_tournament );
	 	
	m_othStartGate = m_nFrontier;
	m_nNumGates = m_nFrontier + gates_noninput;
	m_pGates = new GATE[m_nNumGates];
	m_nNumXORs = 0;

	//============================================================================
	// Now construct the circuit 
	SetupInputGates();
	PutLayerI();
	PutLayerB();
	PutOutputLayer();
	PatchParentFields();

	return TRUE;

}

void CCloudCircuit::SetupInputGates()
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



void CCloudCircuit::PutLayerI()
{
	m_vIdxs.resize(m_nTotItems);
	for(int i=0; i<m_nTotItems; i++)
	{
		m_vIdxs[i] = PutIdxGate(i+1);
	}

	m_vELMs.resize(m_nTotItems);
	for(int i=0; i<m_nTotItems; i++)
	{
		m_vELMs[i] = PutELMGate(i);
	}
	//cout << "m_bHQ=" << m_bHQ << " m_nFrontier=" << m_nFrontier << endl;
}


int CCloudCircuit::PutIdxGate(int r)
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


int CCloudCircuit::PutELMGate(int r)
{
	int qr = 2+m_nRep*2*r;
	int pr = qr+m_nRep;
	int q = m_vInputStart[m_nNumParties-1];
	int p = q+m_nRep;

	int compq = PutGEGate(qr, q, m_nRep);
	int compp = PutGEGate(p, pr, m_nRep);
	int and1 = PutANDGate(compp, compq);

	int out;
	if( m_bHQ )
	{
		out = PutELM0Gate(qr, and1, m_nRep); 
	}
	else
	{
		out = PutELM1Gate(pr, and1, m_nRep);
	}

	return out;
}
   
void CCloudCircuit::PutLayerB()
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
				if( m_bHQ )
					cmp = PutGEGate(m_vELMs[i], m_vELMs[i+1], m_nRep);	// elm[i] >= elm[i+1] ?
				else
					cmp = PutGEGate(m_vELMs[i+1], m_vELMs[i], m_nRep);	// elm[i] <= elm[i+1] ?

				m_vELMs[j] = PutMUXGate(m_vELMs[i], m_vELMs[i+1], cmp, m_nRep);
				m_vIdxs[j] = PutMUXGate(m_vIdxs[i], m_vIdxs[i+1], cmp, m_nIdxRep);
				 
				i+=2;
				j++;
			} 	 
		}
		m_vELMs.resize(j);
		m_vIdxs.resize(j);
	}
}
 
 
void CCloudCircuit::PutOutputLayer()
{
	m_vOutputStart.resize(m_nNumParties, 1);
	m_vOutputEnd.resize(m_nNumParties, 0);
	 
	m_vOutputStart[m_nNumParties-1] = m_vIdxs[0];
	m_vOutputEnd[m_nNumParties-1] = m_vIdxs[0]+m_nIdxRep-1;

	m_nNumGates = m_nFrontier;
}

