// circuit.h
#ifndef __CIRCUIT_H__BY_SGCHOI_
#define __CIRCUIT_H__BY_SGCHOI_

#include "../util/typedefs.h"
#include <iostream>

const BYTE  G_AND		= 0x1;
const BYTE  G_XOR		= 0x2;
 
 
struct GATE
{
	BYTE		type;			// gate type
	char		val;			// for input/output gate: can store the actual value
	int			left;			// left gate
	int			right;			// for binary gate the right input
	int			p_num;			// parent gates
	int*		p_ids;			// parent gates
};



class CCircuit
{
public:
	CCircuit();
	virtual ~CCircuit(){ Clearup();}

public:
	// must be overridden
	virtual BOOL Create(int nParties, const vector<int>& params) = 0;
	 
public:
	void	Save(const char* filename, BOOL withValue = FALSE);
	BOOL	Load(const char* filename);
	void	SaveBin(const char* filename);
	BOOL	LoadBin(const char* filename);
	BOOL	LoadHeaderBin(const char* filename);

	GATE*	Gates(){ return m_pGates;}
	int		GetNumParties(){ return m_nNumParties;}
	int		GetNumGates(){ return m_nNumGates;}
	int		GetInputStart(int nPlayerID){ return m_vInputStart[nPlayerID]; }
	int		GetInputEnd(int nPlayerID){ return m_vInputEnd[nPlayerID]; }
	int		GetOutputStart(int nPlayerID){ return m_vOutputStart[nPlayerID]; }
	int		GetOutputEnd(int nPlayerID){ return m_vOutputEnd[nPlayerID]; }
	int		GetGateStart(){ return m_othStartGate;}
	int		GetNumXORs(){ return m_nNumXORs;}
	int		GetNumANDs(){ return m_nNumGates - m_othStartGate - m_nNumXORs;}
	int		GetNumVBits(int nPlayerID){ return m_vNumVBits[nPlayerID];}

	BOOL	IsEqual(CCircuit*);
	int		ComputeGate(int i1, int i2, BYTE type);
	void	Evaluate();
	void	PrintOutputs();

protected:
	int*	New(int size);
	void	Clearup();

	int  PutEQGate(int left, int right, int rep);
	int  PutGTGate(int left, int right, int rep);
	int  PutGEGate(int left, int right, int rep);
	int	 PutSubGate(int left, int right, int rep);
	int  PutAddGate(int left, int right, int rep, BOOL bCarry=FALSE);
	int  PutWideGate(BYTE type, vector<int>& ins);
	int  PutWideAddGate(vector<int>& ins, int rep);
	int	 PutMUXGate(int a, int b, int s, int rep, BOOL const_right=FALSE);
	int	 PutANDGate(int left, int right);
	int  PutXORGate(int left, int right);
	int  PutELM0Gate(int elm, int b, int rep);
	int  PutELM1Gate(int elm, int b, int rep);
	int  PutMultGate(int left, int right, int rep);
	void PatchParentFields();

protected:
	enum
	{		
		ALLOC_SIZE = 32768
	};

protected:
	GATE*		m_pGates;	

	int			m_nNumParties;
	int			m_nNumGates;		// total # of gates in circuit
	int			m_nNumXORs;
	vector<int>	m_vNumVBits;
	
	vector<int>	m_vInputStart;
	vector<int> m_vInputEnd;
	vector<int>	m_vOutputStart;
	vector<int> m_vOutputEnd;
	int			m_othStartGate;		// gate id for non-input gates

	
	vector<int*>	m_vBufs;
	int				m_nBufIdx;
	
	int			m_nFrontier;
};


CCircuit* CREATE_CIRCUIT(int nParties, const string& name, const vector<int>& params);
CCircuit* LOAD_CIRCUIT_TXT(const char* filename);
CCircuit* LOAD_CIRCUIT_BIN(const char* filename);
CCircuit* LOAD_CIRCUIT_HEADER_BIN(const char* filename);
void TEST_CIRCUIT(const vector<const char*>& configs, BOOL bLog); 

#endif // __CIRCUIT_H__BY_SGCHOI_
