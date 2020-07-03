#ifndef __SI_CIRCUIT_H__BY_SGCHOI_
#define __SI_CIRCUIT_H__BY_SGCHOI_

#include "circuit.h"
#include <cassert>

class CSICircuit : public CCircuit
{
public:
	BOOL Create(int nParties, const vector<int>& vParams);


private:
	void AddEqualityBlock(int Pi, int Pj);
	void ConstructOutput(int i);

	int INDEX(int Pi, int Pj)
	{ 
		return Pi*m_nNumParties + Pj;
	}
	
	int ITEM(int Pi, int Pj, int i, int j)
	{ 
		return i*m_vSizes[Pj] + j; 
	}

private:
	int						m_nNumVBits;
	vector<int>				m_vSizes;
	vector<int>				m_vAggSizes;
	vector<BOOL>			m_vNeedOutput;
	vector<int>				m_vMatchIdxStart;
};

#endif //__SI_CIRCUIT_H__BY_SGCHOI_