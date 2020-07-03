// cloud.h
#ifndef __CLOUD__H__BY_SGCHOI_
#define __CLOUD__H__BY_SGCHOI_
 
#include "circuit.h"
#include <cassert>

class CCloudCircuit : public CCircuit
{
public:
	BOOL Create(int nParties, const vector<int>& vParams);
 
private:
	void PutOutputLayer();
	void PutLayerI();
	void SetupIndices();
	void PutLayerB();
	int  PutELMGate(int r);
	int	 PutIdxGate(int r);
	void SetupInputGates();

private:
	int		m_nItems;
	int		m_nTotItems;
	BOOL	m_bHQ;
	int		m_nRep;
	int		m_nIdxRep;
	int		m_nIdxStart;

	vector<int>	m_vELMs;
	vector<int>	m_vIdxs;
};

#endif //__CLOUD__H__BY_SGCHOI_