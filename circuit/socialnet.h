// socialnet.h
#ifndef __SOCIAL_NET__H__BY_SGCHOI_
#define __SOCIAL_NET__H__BY_SGCHOI_
 
#include "circuit.h"
#include <cassert>

class CSNetCircuit : public CCircuit
{
public:
	BOOL Create(int nParties, const vector<int>& vParams);
 
public:
	void CreateFindAllCloseMatches();
	void PutOutputLayer();
	void PutLayerI();
	void SetupIndices();
	void PutLayerB();
	int  PutELMGate(int r);
	int	 PutIdxGate(int r);
	int	 PutDSTGate(int r);
	int	 PutCAPGate(int r);
	int	 PutCNTGate(int r);
	int	 PutINGate(int r);
	int	 PutCNTGate(const vector<int>&, int rep);
	void SetupInputGates();

private:
	enum ProtType
	{
		e_AllCloseMatches = 0,
		e_ClosetMatch = 1,
		e_BestResource = 2,
	}; 

	int		m_nType;
	int		m_nTotItems;
	int		m_nRep;
	int		m_nIdxRep;
	int		m_nIdxStart;
	int		m_nCntRep;
	int		m_nCntRange;


	vector<int>	m_vELMs;
	vector<int>	m_vIdxs;
};

#endif //__SOCIAL_NET__H__BY_SGCHOI_
