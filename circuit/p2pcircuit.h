
#ifndef __P2P_CIRCUIT_H__BY_SGCHOI_
#define __P2P_CIRCUIT_H__BY_SGCHOI_

#include "circuit.h"

class CP2PCircuit : public CCircuit
{

public:
	CP2PCircuit(){}
	~CP2PCircuit(){}

// public interface
public:
	BOOL Create(int nParties, const vector<int>& vParams); // for multi-party computation
 	
private:
	// constants
	enum
	{
		MUX_GATES = 3,  // # of gates for MUX for 1 bit
		CMP_GATES = 4,  // # of gates for Comparison for 1 bit
	};

private:
	int		GetNumBits(int decimal);
	int		GetNumGates(int tot_items, int num_bits);
	int		GetGatesSkip(int tot_items, int num_vbits, int layer_no, 
				int num_vs_bottom, int num_vs_middle, int* vs_no);
	int		IsVSLeft(int tot_items, int layer_no, int* vs_no);
	int		CreateInputLeft(int id, int is_false, int num_bits);
	int		CreateInputRight(int id, int is_false, int num_bits);
	int		CreateANDLeft(int id, int is_final, int num_vbits, int* vs_no);
	int		CreateANDRight(int id, int is_final, int num_vbits, int* vs_no, int is_alone);
	int		CreateCMP(int id, int is_false, int num_bits, int num_vbits);
	int		CreateMUXValue(int id, int is_final, int num_vbits, int num_bits, int is_left, int gates_skip, int is_moreMUXvalue);
	int		CreateMUXIndex(int id, int is_final, int num_bits, int num_vbits,
				int is_left, int layer_no, int gates_skip, int is_moreMUXvalue, int* vs_no, int is_alone, int nParties);
	double	Log2(double);
	void	Int2Bits(int dec, int* buf);
	
	int		GetInputStartC(int);
	int		GetInputEndC(int);

private:
	int			m_serStartGate;		// gate id for server's first input
	int			m_cliStartGate;		// gate id for client's first input
	int			m_nNumItems;		// # of items
	int			m_nNumServers;

};

#endif // __P2P_CIRCUIT_H__BY_SGCHOI_

