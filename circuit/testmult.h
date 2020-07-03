// testmult.h
#ifndef __TEST_MULT_H__BY_SGCHOI_
#define __TEST_MULT_H__BY_SGCHOI_
 
#include "circuit.h"
#include <cassert>

class CTestMultCircuit : public CCircuit
{
public:
	BOOL Create(int nParties, const vector<int>& vParams);
 
public:
	void PutMultLayer();
	void PutOutputLayer();

private:
	int m_nWidth;
	int	m_nDepth;
	int	m_nRep;
	vector<int>	m_vLayerInputs;
	vector<int> m_vLayerOutputs;
};

#endif //__TEST_MULT_H__BY_SGCHOI_