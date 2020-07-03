// p2pcircuit.cpp
#include "p2pcircuit.h"
#include <cassert>
#include <cmath>


///=================================================================================
// creates the binary circuit that consits of only gate infomation
// input: the number of items of each server,	
//        the number of servers,
//        the number of bits for each item "value" (for multiparty computation)
// output: void
BOOL CP2PCircuit::Create(int nParties, const vector<int>& vParams)
{
	int num_servers = nParties - 1;
	
	int num_items  = vParams[0];
	int input_range = vParams[1];

	if( vParams.size() < 2 )
	{
		cout << "Error! This circuit needs two parameters: #items and input_range." << endl;
		return FALSE;
	}

	int tot_items;		// # of items of all servers
	int num_vbits;		// # of bits to represent each value	
	int num_bits;		// # of bits to identify [0, num_items]
	int num_gates;		// # of gates required to create the circuit
	int num_layers;		// # of layers for the tournament
	int num_andgates;	// # of AND gates
	int num_mux_val;	// # of MUX circuit for values
	int num_mux_idx;	// # of MUX circuit for indexes
	int num_cmp;		// # of comparison (circuit)
	int num_in;			// # of IN gates
	int num_out;		// # of OUT gates
	int num_vs_bottom;	// # of gates for each vs at the bottom of tournament 
	int num_vs_middle;	// # of gates for each vs between the bottom and top of the tournament
	int num_vs_top;		// # of gates for each vs at top of the tournament
	//vector<int> outputs = vector<int>();	// final outputs that will be given to OUT gates
	
	tot_items = num_items * num_servers;

	// collects necessary info first
	num_bits = GetNumBits(tot_items);
	num_vbits = input_range >= 0? GetNumBits(input_range) : num_bits;
	//num_vbits = -1;
	
	num_layers = (int)ceil(Log2(tot_items));
	//num_andgates = tot_items * num_bits;
	num_andgates = tot_items * num_vbits;
	num_cmp = tot_items - 1;
	num_mux_val = num_cmp - 1;
	num_mux_idx = num_cmp;
	//num_in = 2+ tot_items*num_bits + tot_items;
	num_in = 2+ tot_items*num_vbits + tot_items;
	num_out = num_bits;
	//num_gates = num_andgates + (num_mux_val+num_mux_idx)*MUX_GATES*num_bits 
	//	+ num_cmp*CMP_GATES*num_bits + num_in +num_out;
	num_gates = num_andgates + num_mux_val*MUX_GATES*num_vbits
		+ num_mux_idx*MUX_GATES*num_bits + num_cmp*CMP_GATES*num_vbits + num_in +num_out;
	//num_gates = GetNumGates(tot_items, num_bits);

	//num_vs_bottom = num_bits*2 + CMP_GATES*num_bits + 2*MUX_GATES*num_bits;
	num_vs_bottom = num_vbits*2 + CMP_GATES*num_vbits + MUX_GATES*(num_bits+num_vbits);
	//num_vs_middle = num_vs_bottom - 2*num_bits;
	num_vs_middle = num_vs_bottom - 2*num_vbits;
	num_vs_top = num_vs_middle - MUX_GATES*num_bits;
	num_vs_top = num_vs_middle - MUX_GATES*num_vbits;

	// circuit initialization
	cout << " Total # of gates:" << num_gates << endl;
	cout << " Total # of gates for bottom vs:" << num_vs_bottom << endl;
	cout << " Total # of gates for middle vs:" << num_vs_middle << endl;
	cout << " Total # of gates for top vs:" << num_vs_top << endl;
	cout << " Total # of IN gates:" << num_in << endl;

	m_nNumGates = num_gates;
	//m_nNumBits = num_bits;
	m_nNumItems = num_items; 
	//m_nNumVBits = input_range >= 0 ? GetNumBits(input_range) : num_bits;
	m_nNumServers = num_servers;
	m_nNumXORs = 0;
	m_vNumVBits.resize(nParties,num_vbits);
	m_vNumVBits[nParties-1] = 1;

	m_pGates = new GATE[num_gates];  
	if (m_pGates == NULL){
		puts("CP2PCircuit::Create(const int): memory error");
		exit(-1);
	}
	for (int i=0;i<num_gates;i++){
		(m_pGates+i)->left = -1;
		(m_pGates+i)->right = -1;
		(m_pGates+i)->p_num = 0;
	}
	 

	// starts creating the circuit	
	int id = 0;				// gate id
	int is_left = 1;		// left - 1, right - 0	
	int is_final = 0;		// final comparison: yes - 1, no - 0
	int is_bottompair = 0;	// indicates whether the comparison is a pair at the bottom of tournament
	int is_bottomsingle = 0;// indicates whether the comparison is a single at the bottom of tournament
	int is_middle = 0;		// indicates whether the comparison is in the middle of tournament
	int is_alone = 0;		// whether the vs is a pair or single
	int is_moreMUXvalue = 0;// whether the higher layer's vs uses MUX_value
	int num_remainders;		// # of items that remain on the tournament
	int gates_skip;			// # of gates that should be skipped
	int layer_no=0;			// layer number from the bottom of the tournament starting from 0
	int* vs_no;				// # of completed vs from the leftmost (on the current layer) of the tournament starting from 0
	int* layer_waiting;	// # of waitings at each layer
	int* is_left_before;	// whether last vs was left or right
	
	num_remainders = tot_items;
	
	layer_waiting = new int[num_layers+1];
	vs_no = new int[num_layers+1];
	is_left_before = new int[num_layers+1];
	for (int i=0;i<num_layers;i++){
		layer_waiting[i] = 0;
		vs_no[i]=0;
		is_left_before[i] = -1;
	}

	// IN gates for const inputs 0 and 1
	(m_pGates+id)->type = G_XOR;
	(m_pGates+id)->right = 0;
	id++;
	(m_pGates+id)->type = G_XOR;
	(m_pGates+id)->right = 0;
	id++;
	
	// creates gates for server inputs
	int tmp_num_bits;
	if (num_vbits <= 0){
		tmp_num_bits = num_bits;	// for two party computation
	}
	else{
		tmp_num_bits = num_vbits;	// for multiparty computation
	}
	m_serStartGate = id;
	for (int i=0;i<num_servers;i++){
		m_vInputStart.push_back(id);
		for (int j=0;j<num_items;j++){
			for (int k=0;k<tmp_num_bits;k++,id++){
				(m_pGates+id)->type = G_XOR;
				(m_pGates+id)->right = 0;
			}
		}
		m_vInputEnd.push_back(id-1);
	}
	
	// creates gates for client inputs
	m_cliStartGate = id;
	m_vInputStart.push_back(id);
	for(int i=0;i<tot_items;i++,id++){
		(m_pGates+id)->type = G_XOR;
		(m_pGates+id)->right = 0;
	}
	m_vInputEnd.push_back(id-1);

	// creates gates for output of each party
	// servers
	for (int i=0;i<nParties-1;i++){
		// no output for servers
		m_vOutputStart.push_back(0);
		m_vOutputEnd.push_back(-1);
	}

	// output for client
	m_vOutputStart.push_back(num_gates-num_out);
	m_vOutputEnd.push_back(num_gates-1);
	for(int i=0;i<num_out;i++){
		(m_pGates+m_vOutputStart[num_servers]+i)->type = G_XOR;
		(m_pGates+m_vOutputStart[num_servers]+i)->p_num = 0;
		(m_pGates+m_vOutputStart[num_servers]+i)->right = 0;
	}

	// creates gates other than server and client's input gates
	m_othStartGate = id;
	for (int i=0;i<num_cmp;i++){
		if (i== num_cmp-1){
			is_final = 1;	
		}

		int tmp=0;
		int layer_no_tmp=-1;
		// compares with two same layers (bottom layer not included)
		assert(layer_no >= 0);
		if (layer_waiting[layer_no] == 2){
			is_alone = 0;
			//is_left = 1;///////////////////////////////
			if (is_left_before[layer_no] == 1){
				is_left = 0;
				is_left_before[layer_no] = is_left;
			}
			else if (is_left_before[layer_no] == -1){
				is_left = 1;
				is_left_before[layer_no] = is_left;
			}
			else if (is_left_before[layer_no] == 0){
				is_left = IsVSLeft(tot_items,layer_no, vs_no);
				//tmp = tot_items - 2*vs_no[layer_no];
				is_left_before[layer_no] = is_left;
			}
			
			if (is_left){
				if (layer_no >= num_layers-2){
					is_moreMUXvalue = 0;
				}
				else{
					is_moreMUXvalue = 1;
				}
			}
			else {// && num_remainders>0){
				is_moreMUXvalue = 0;
				for (int j=layer_no+1;j<num_layers-1;j++){

					#ifdef _DEBUG
					assert(j >= 0);
					#endif
					
					if (layer_waiting[j] == 1){
						is_moreMUXvalue = 1;
						break;
					}
				}
			}
			
			if (is_left){
				gates_skip = GetGatesSkip(tot_items, num_vbits, layer_no, num_vs_bottom, num_vs_middle, vs_no);
			}

			id = CreateCMP(id, is_final, num_bits, num_vbits);
			if (!is_final){
				id = CreateMUXValue(id, is_final, num_bits, num_vbits,is_left, gates_skip,is_moreMUXvalue);
			}
			id = CreateMUXIndex(id, is_final, num_bits, num_vbits, is_left, layer_no, gates_skip,is_moreMUXvalue, vs_no, is_alone, nParties);
			
			vs_no[layer_no]++;
			layer_waiting[layer_no]=0;
			
			#ifdef _DEBUG
			assert( layer_no >= 0);
			#endif
			
			layer_no++;
			layer_waiting[layer_no]++;

			#ifdef _DEBUG
			assert( layer_no <= num_layers);
			#endif
			
		}
		// compares with different layers (bottom layer not included)
		else if (num_remainders == 0){
			is_alone= 0;
			is_left = 0;

			for (int j=layer_no+1;j<num_layers;j++){

				#ifdef _DEBUG
				assert( j >= 0 );
				#endif
			
				if (layer_waiting[j] == 1){
					layer_waiting[j] = 0;
					vs_no[j]++;
					layer_no_tmp = j+1;
					break;
				}
			}
			if (layer_no_tmp == -1){
				puts("CP2PCircuit::Create(const int): layer_no_tmp is -1");		
				exit(-1);
			}

			//if (layer_no_tmp == num_layers-1){	// higher layer (one above) is the top layer
			//	is_moreMUXvalue=0;
			//}
			//else{
			//	is_moreMUXvalue=1;
			//}
			
			is_moreMUXvalue = 0;
			for (int j=layer_no_tmp;j<num_layers-1;j++){

				#ifdef _DEBUG
				assert( j >= 0 );
				#endif
		

				if (layer_waiting[j] == 1){
					is_moreMUXvalue = 1;
					break;
				}
			}

			id = CreateCMP(id, is_final, num_bits, num_vbits);
			if (!is_final){
				id = CreateMUXValue(id, is_final, num_bits, num_vbits,is_left, gates_skip,is_moreMUXvalue);
			}
			id = CreateMUXIndex(id, is_final, num_bits, num_vbits, is_left, layer_no, gates_skip,is_moreMUXvalue, vs_no, is_alone, nParties);
			
			layer_waiting[layer_no]=0;

			#ifdef _DEBUG
			assert( layer_no >= 0 && layer_no < num_layers);
			#endif
		

			layer_no = layer_no_tmp;
			layer_waiting[layer_no]++;

			//#ifdef _DEBUG
			//assert( layer_no >= 0 && layer_no < num_layers);
			//#endif
		
		}
		// compares with different layers (only one layer is bottom)
		else if (num_remainders == 1){
			layer_no = 0;
			is_alone = 1;
			is_left = 0;
			
			for (int j=layer_no+1;j<num_layers;j++){

				#ifdef _DEBUG
				assert( j >= 0 );  
				#endif
		
				if (layer_waiting[j] == 1){
					layer_waiting[j] = 0;
					vs_no[j]++;
					layer_no_tmp = j+1;
					break;
				}
			}
			if (layer_no_tmp == -1){
				puts("CP2PCircuit::Create(const int): layer_no_tmp is -1");		
				exit(-1);
			}
			/*
			if (layer_no_tmp == num_layers){	// higher layer (one above) is the top layer
				is_moreMUXvalue=0;
			}
			else{
				is_moreMUXvalue=1;
			}
			*/
			is_moreMUXvalue = 0;
			for (int j=layer_no_tmp;j<num_layers-1;j++){

				#ifdef _DEBUG
				assert( j >= 0 );  
				#endif
		
				if (layer_waiting[j] == 1){
					is_moreMUXvalue = 1;
					break;
				}
			}
			
			id = CreateANDRight(id, is_final, num_vbits, vs_no, 1);
			id = CreateCMP(id, is_final, num_bits, num_vbits);
			if (!is_final){
				id = CreateMUXValue(id, is_final, num_bits, num_vbits, is_left, gates_skip,is_moreMUXvalue);
			}
			id = CreateMUXIndex(id, is_final, num_bits, num_vbits, is_left, layer_no, gates_skip,is_moreMUXvalue, vs_no, is_alone, nParties);

			layer_no = layer_no_tmp;
			layer_waiting[layer_no]++;
			num_remainders--;

			#ifdef _DEBUG
			assert( layer_no >= 0 && layer_no < num_layers);
			#endif
		
		}
		// compares with two bottom layers
		else if (num_remainders >= 2){
			layer_no = 0;
			is_alone = 0;
			if (is_left_before[layer_no] == 1){
				is_left = 0;
				is_left_before[layer_no] = is_left;
			}
			else if (is_left_before[layer_no] == -1){
				is_left = 1;
				is_left_before[layer_no] = is_left;
			}
			else if (is_left_before[layer_no] == 0){
				is_left = IsVSLeft(tot_items,layer_no, vs_no);
				//tmp = tot_items - 2*vs_no[layer_no];
				is_left_before[layer_no] = is_left;
			}
			
			if (num_layers<=2){
				is_moreMUXvalue = 0;
			}
			else if(is_left){
				is_moreMUXvalue = 1;
			}
			else {
				is_moreMUXvalue = 0;
				for (int j=layer_no+1;j<num_layers-1;j++){

					#ifdef _DEBUG
					assert( j >= 0 );
					#endif
		
					if (layer_waiting[j] == 1){
						is_moreMUXvalue = 1;
						break;
					}
				}
			}
			if (is_left){						
				gates_skip = GetGatesSkip(tot_items, num_vbits, layer_no, num_vs_bottom, num_vs_middle, vs_no);
			}
			//id = CreateInputLeft(id, is_final, num_bits);
			//id = CreateInputRight(id, is_final, num_bits);
			id = CreateANDLeft(id, is_final, num_vbits, vs_no);
			id = CreateANDRight(id, is_final, num_vbits, vs_no, 0);
			id = CreateCMP(id, is_final, num_bits, num_vbits);
			if (!is_final){
				id = CreateMUXValue(id, is_final, num_bits, num_vbits, is_left, gates_skip, is_moreMUXvalue);
			}
			id = CreateMUXIndex(id, is_final, num_bits, num_vbits, is_left, layer_no, gates_skip,is_moreMUXvalue, vs_no, is_alone, nParties);
			
			vs_no[layer_no]++;
			layer_no = 1;
			layer_waiting[layer_no]++;
			num_remainders -= 2;

			#ifdef _DEBUG
			assert( layer_no >= 0 && layer_no < num_layers);
			#endif
		
		}
	}

	#ifdef _DEBUG
	cout << "final gate id: " << id-1+num_out << endl;
	#endif

	delete [] layer_waiting;
	delete [] vs_no;
	delete [] is_left_before; 

	m_nNumParties = m_nNumServers + 1;
	m_vInputStart.resize(m_nNumParties);
	m_vInputEnd.resize(m_nNumParties);
	 
	/*
	cout << "input start gate id1: " << m_vInputStart[0] << endl;
	cout << "input end gate id1: " << m_vInputEnd[0] << endl;
	cout << "input start gate id2: " << m_vInputStart[1] << endl;
	cout << "input end gate id2: " << m_vInputEnd[1] << endl;
	cout << "input start gate id3: " << m_vInputStart[2] << endl;
	cout << "input end gate id3: " << m_vInputEnd[2] << endl;
	cout << "output start gate id1: " << m_vOutputStart[0] << endl;
	cout << "output end gate id1: " << m_vOutputEnd[0] << endl;
	cout << "output start gate id2: " << m_vOutputStart[1] << endl;
	cout << "output end gate id2: " << m_vOutputEnd[1] << endl;
	cout << "output start gate id3: " << m_vOutputStart[2] << endl;
	cout << "output end gate id3: " << m_vOutputEnd[2] << endl;
	*/

	return TRUE;
}

// gets # of bits for a decimal number
// input: decimal number
// output: # of bits for the input
// note: input should not be a negative int
int CP2PCircuit::GetNumBits(int decimal)
{
	int num_bits = 1;
	int tmp; 
	
	if (decimal < 0){
		puts("CP2PCircuit::GetNumBits(int): negative input");
		exit(-1);
	}
	else if(decimal == 0){
		return 0;
	}
	else if(decimal == 1){
		return 1;
	}

	tmp = decimal;
	while(1==1){
		num_bits++;
		tmp = tmp/2;
		if (tmp <= 1){
			break;
		}
	}

	return num_bits;
}

// gets # of gates required to create the circuit
// input: # of items, # of bits for each 
// output: # of gates required
// note: input should be a positive int
int CP2PCircuit::GetNumGates(int tot_items, int num_bits)
{
	if (tot_items <= 0 || num_bits <= 0){
		puts("CP2PCircuit::GetNumGates(int, int): needs positive values for inputs");
		exit(-1);
	}

	int num_gates;
	int num_layers;	// # of layers for the tournament
	int num_andgates;	// # of AND gates
	int num_mux;	// # of MUX
	int num_cmp;	// # of comparison
	
	num_layers = (int)ceil(Log2(tot_items));
	num_andgates = tot_items * num_bits;
	num_cmp = tot_items - 1;
	num_mux = (num_cmp - 1)*2 + 1;
	num_gates = num_andgates + num_mux*MUX_GATES*num_bits + num_cmp*CMP_GATES*num_bits;

	return num_gates;
}

// computes # of gates that have to be skipped
int CP2PCircuit::GetGatesSkip(int tot_items, int num_vbits, int layer_no, int num_vs_bottom, int num_vs_middle, int* vs_no)
{
	int remainder = tot_items - (vs_no[layer_no+1]*2+1)* (int)pow(2.0,layer_no+1);
	if (remainder > (int)pow(2.0, layer_no+1)){
		remainder = (int)pow(2.0, layer_no+1);
	}
	//int gates_skip;
	//if (remainder == 1){
	//	gates_skip = num_bits;
	//}
	//else{
		int gates_skip = (int)floor(remainder/2.0)*num_vs_bottom + (remainder%2)*num_vbits 
			+ ((int)ceil(remainder/2.0)-1)*num_vs_middle;
			//+ ((int)ceil(Log2(remainder))-1)*num_vs_middle;
	//}
	
	return gates_skip;
}

// figures out if vs # is left or right
int CP2PCircuit::IsVSLeft(int tot_items, int layer_no, int* vs_no)
{
	int remainder = tot_items - (vs_no[layer_no+1]*2+1)* (int)pow(2.0,layer_no+1);
	if (remainder>0){
		return 1;
	}
	else{
		return 0;
	}
}

// creates input gates for provider and customer
// left side
// input: start gate id, if or not the vs is final, # of bits
// output: final gate id
int CP2PCircuit::CreateInputLeft(int id, int is_final, int num_bits)
{
	for (int k=0;k<num_bits;k++,id++){
		if (!is_final){
			(m_pGates+id)->p_num = 3;
			(m_pGates+id)->p_ids = New(3); 
			(m_pGates+id)->p_ids[0] = id + 2*(2*num_bits-k) +k;
			(m_pGates+id)->p_ids[1] = id + 2*(2*num_bits-k) +k +(2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*num_bits + MUX_GATES*k ;
			(m_pGates+id)->p_ids[2] = id + 2*(2*num_bits-k) +k +(2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*num_bits + MUX_GATES*k + 2;
			id++;
			(m_pGates+id)->p_num = 3;
			(m_pGates+id)->p_ids = New(3);
			(m_pGates+id)->p_ids[0] = id + 2*(2*num_bits-k) +k -1;
			(m_pGates+id)->p_ids[1] = id + 2*(2*num_bits-k) +k -1 +(2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*num_bits + MUX_GATES*k ;
			(m_pGates+id)->p_ids[2] = id + 2*(2*num_bits-k) +k -1 +(2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*num_bits + MUX_GATES*k + 2;
		}
		else{
			(m_pGates+id)->p_ids = New(3);
			(m_pGates+id)->p_ids[0] = id + 2*(2*num_bits-k) +k;
			(m_pGates+id)->p_ids[1] = id + 2*(2*num_bits-k) +k + (2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*k;
			(m_pGates+id)->p_ids[2] = id + 2*(2*num_bits-k) +k + (2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*k + 2;
			id++;
			(m_pGates+id)->p_num = 3;
			(m_pGates+id)->p_ids = New(3);
			(m_pGates+id)->p_ids[0] = id + 2*(2*num_bits-k) +k -1;
			(m_pGates+id)->p_ids[1] = id + 2*(2*num_bits-k) +k -1 + (2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*k;
			(m_pGates+id)->p_ids[2] = id + 2*(2*num_bits-k) +k -1 + (2*num_bits-k) + num_bits*CMP_GATES+1 + MUX_GATES*k + 2;
		}
	}
	return id;
}

// creates input gates for provider and customer
// right side
// input: start gate id, if or not the vs is final, # of bits
// output: final gate id
int CP2PCircuit::CreateInputRight(int id, int is_final, int num_bits)
{
	for (int k=0;k<num_bits;k++,id++){
		if (!is_final){			
			(m_pGates+id)->p_num = 2;
			(m_pGates+id)->p_ids = New(2);
			(m_pGates+id)->p_ids[0] = id + 2*(num_bits-k) +k+num_bits;
			(m_pGates+id)->p_ids[1] = id + 2*(num_bits-k) +k+num_bits + (num_bits-k) + num_bits*CMP_GATES + MUX_GATES*num_bits + MUX_GATES*k ;
			id++;
			(m_pGates+id)->p_num = 2;
			(m_pGates+id)->p_ids = New(2);
			(m_pGates+id)->p_ids[0] = id + 2*(num_bits-k) +k+num_bits -1;
			(m_pGates+id)->p_ids[1] = id + 2*(num_bits-k) +k+num_bits -1 + (num_bits-k) + num_bits*CMP_GATES + MUX_GATES*num_bits + MUX_GATES*k ;
		}
		else{
			(m_pGates+id)->p_num = 2;
			(m_pGates+id)->p_ids = New(2);
			(m_pGates+id)->p_ids[0] = id + 2*(num_bits-k) +k+num_bits;
			(m_pGates+id)->p_ids[1] = id + 2*(num_bits-k) +k+num_bits+ (num_bits-k) + num_bits*CMP_GATES + MUX_GATES*k;
			id++;
			(m_pGates+id)->p_num = 2;
			(m_pGates+id)->p_ids = New(2);
			(m_pGates+id)->p_ids[0] = id + 2*(num_bits-k) +k+num_bits -1;
			(m_pGates+id)->p_ids[1] = id + 2*(num_bits-k) +k+num_bits -1 + (num_bits-k) + num_bits*CMP_GATES + MUX_GATES*k;
		}
	}
	return id;
}

// creates AND gates for provider and customer
// left side
// input: start gate id, if or not the vs is final, # of bits, 
//			vs number from the leftmost (on the current layer) of the tournament starting from 0
// output: final gate id
int CP2PCircuit::CreateANDLeft(int id, int is_final, int num_vbits, int* vs_no)
{
	int tmpid=-1;
	int s_inputid=-1;
	int c_inputid=-1;
	
	// finds corresponding IN gates for client
	c_inputid = m_cliStartGate + 2*vs_no[0];
	(m_pGates+c_inputid)->p_num = num_vbits;
	(m_pGates+c_inputid)->p_ids = New(num_vbits);
	for (int k=0;k<num_vbits;k++,id++){
		(m_pGates+c_inputid)->p_ids[k]= id;

		// finds corresponding IN gates for server
		s_inputid = m_serStartGate + 2*vs_no[0]*num_vbits +k;
		(m_pGates+s_inputid)->p_num = 1;
		(m_pGates+s_inputid)->p_ids = New(1);
		(m_pGates+s_inputid)->p_ids[0]= id;

		// AND gates
		(m_pGates+id)->type = G_AND;
		if (!is_final){
			(m_pGates+id)->p_num = 3;
			(m_pGates+id)->p_ids = New(3);
			tmpid = id+ (2*num_vbits-k) + CMP_GATES*k ;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->right = id;
			tmpid = id+ (2*num_vbits-k) + num_vbits*CMP_GATES + MUX_GATES*k;
			(m_pGates+id)->p_ids[1] = tmpid;
			(m_pGates+tmpid)->left = id;
			tmpid = id+ (2*num_vbits-k) + num_vbits*CMP_GATES + MUX_GATES*k + 2;
			(m_pGates+id)->p_ids[2] = tmpid;
			(m_pGates+tmpid)->left = id;
		}
		else{
			(m_pGates+id)->p_num = 1;
			(m_pGates+id)->p_ids = New(1);
			tmpid = id+ (2*num_vbits-k) + CMP_GATES*k;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->right = id;
			//pid[1] = id+ (2*num_bits-k) + num_bits*CMP_GATES + MUX_GATES*k;
		}
		(m_pGates+id)->left = s_inputid;
		(m_pGates+id)->right = c_inputid;
	}
	return id;
}

// creates AND gates for provider and customer
// right side
// input: start gate id, if or not the vs is final, # of bits,
//			vs number from the leftmost (on the current layer) of the tournament starting from 0,
//			whether the right AND is with or without left AND
// output: final gate id
int CP2PCircuit::CreateANDRight(int id, int is_final, int num_vbits, int* vs_no, int is_alone)
{
	int tmpid=-1;
	int s_inputid=-1;
	int c_inputid=-1;
	
	// finds corresponding IN gates for client
	if (!is_alone){
		c_inputid = m_cliStartGate + 2*vs_no[0] +1;
	}
	else{
		c_inputid = m_cliStartGate + 2*vs_no[0];
	}
	(m_pGates+c_inputid)->p_num = num_vbits;
	(m_pGates+c_inputid)->p_ids = New(num_vbits);
	
	for (int k=0;k<num_vbits;k++,id++){
		(m_pGates+c_inputid)->p_ids[k]= id;

		// finds corresponding IN gates for server
		if (!is_alone){
			s_inputid = m_serStartGate + 2*vs_no[0]*num_vbits +k +num_vbits;
		}
		else{
			s_inputid = m_serStartGate + 2*vs_no[0]*num_vbits +k;
		}
		(m_pGates+s_inputid)->p_num = 1;
		(m_pGates+s_inputid)->p_ids = New(1);
		(m_pGates+s_inputid)->p_ids[0]= id;

		// AND gates
		(m_pGates+id)->type = G_AND;
		if (!is_final){
			(m_pGates+id)->p_num = 3;
			(m_pGates+id)->p_ids = New(3);
			tmpid = id+ (num_vbits-k) + CMP_GATES*k +1;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->right = id;
			tmpid = id+ (num_vbits-k) + CMP_GATES*k +1 +2;
			(m_pGates+id)->p_ids[1] = tmpid;
			(m_pGates+tmpid)->left = id;
			tmpid = id+ (num_vbits-k) + num_vbits*CMP_GATES + MUX_GATES*k;
			(m_pGates+id)->p_ids[2] = tmpid;
			(m_pGates+tmpid)->right = id;
		}
		else{
			(m_pGates+id)->p_num = 2;
			(m_pGates+id)->p_ids = New(2);
			tmpid = id+ (num_vbits-k) + CMP_GATES*k +1;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->right = id;
			tmpid = id+ (num_vbits-k) + CMP_GATES*k +1 +2;
			(m_pGates+id)->p_ids[1] = tmpid;
			(m_pGates+tmpid)->left = id;
		}
		(m_pGates+id)->left = s_inputid;
		(m_pGates+id)->right = c_inputid;
	}
	return id;
}

// creates comparison circuit
// input: start gate id, if or not the vs is final, # of bits
// output: final gate id
// NOTE: switched all num_bits to num_vbits
int CP2PCircuit::CreateCMP(int id, int is_final, int num_bits, int num_vbits)
{
	int tmpid=-1;
	for (int k=0;k<num_vbits;k++,id++){
		(m_pGates+id)->type = G_XOR;
		m_nNumXORs++;
		if(k==0)(m_pGates+id)->left = 0;
		(m_pGates+id)->p_num = 1;
		(m_pGates+id)->p_ids = New(1);
		tmpid = id + 2;
		(m_pGates+id)->p_ids[0] = tmpid;
		(m_pGates+tmpid)->left = id;
		id++;
		(m_pGates+id)->type = G_XOR;
		m_nNumXORs++;  
		if(k==0)(m_pGates+id)->left = 0;
		(m_pGates+id)->p_num = 1;
		(m_pGates+id)->p_ids = New(1);
		tmpid = id + 1;
		(m_pGates+id)->p_ids[0] = tmpid;
		(m_pGates+tmpid)->right = id;
		id++;
		(m_pGates+id)->type = G_AND;
		(m_pGates+id)->p_num = 1;
		(m_pGates+id)->p_ids = New(1);
		tmpid = id + 1;
		(m_pGates+id)->p_ids[0] = tmpid;
		(m_pGates+tmpid)->right = id;
		id++;
		(m_pGates+id)->type = G_XOR;
		m_nNumXORs++;
		if (k == num_vbits-1){ // final gate of CMP
			if (!is_final){
				(m_pGates+id)->p_num = num_vbits+num_bits;
				(m_pGates+id)->p_ids = New(num_vbits+num_bits); 
				
				for (int m=0;m<num_vbits;m++){
					tmpid = id + 2 + m*MUX_GATES;
					(m_pGates+id)->p_ids[m] = tmpid;
					(m_pGates+tmpid)->left = id;
				}
				for (int m=0;m<num_bits;m++){
					tmpid = id + 2 + num_vbits*MUX_GATES + m*MUX_GATES;
					(m_pGates+id)->p_ids[num_vbits+m] = tmpid;
					(m_pGates+tmpid)->left = id;
				}

				//tmpid = id + 2;
				//(m_pGates+id)->p_ids[0] = tmpid;
				//(m_pGates+tmpid)->left = id;
				//tmpid = id + 2 + num_bits*MUX_GATES;
				//(m_pGates+id)->p_ids[1] = tmpid;
				//(m_pGates+tmpid)->left = id;
			}
			else{ // CMP on top level
				(m_pGates+id)->p_num = num_bits;
				(m_pGates+id)->p_ids = New(num_bits);
				for (int m=0;m<num_bits;m++){
					tmpid = id + 2 + m*MUX_GATES;
					(m_pGates+id)->p_ids[m] = tmpid;
					(m_pGates+tmpid)->left = id;
				}
				
			}
		}
		else{
			(m_pGates+id)->p_num = 2;
			(m_pGates+id)->p_ids = New(2);
			tmpid = id + 1;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->left = id;
			tmpid = id + 2;
			(m_pGates+id)->p_ids[1] = tmpid;
			(m_pGates+tmpid)->left = id;
		}
	}
	return id;
}


// MUX circuit (for values)
// input: start gate id, if or not the vs is final, # of bits, if or not vs is left ,# of gates to skip,
//			whether or not the higher layer's vs uses MUX_value circuit
// output: final gate id
int CP2PCircuit::CreateMUXValue(int id, int is_final, int num_bits, int num_vbits, int is_left, int gates_skip, int is_moreMUXvalue)
{
	if (!is_final){
		int tmpid = -1;
		for (int k=0;k<num_vbits;k++,id++){
			(m_pGates+id)->type = G_XOR;
			m_nNumXORs++;
			(m_pGates+id)->p_num = 1;
			(m_pGates+id)->p_ids = New(1);
			tmpid = id + 1;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->right = id;
			id++;
			(m_pGates+id)->type = G_AND;
			(m_pGates+id)->p_num = 1;
			(m_pGates+id)->p_ids = New(1);
			tmpid = id + 1;
			(m_pGates+id)->p_ids[0] = tmpid;
			(m_pGates+tmpid)->right = id;
			id++;
			(m_pGates+id)->type = G_XOR;
			m_nNumXORs++;
			
			if (is_left){
				if (is_moreMUXvalue){
					(m_pGates+id)->p_num = 3;
					(m_pGates+id)->p_ids = New(3);
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + CMP_GATES*k + gates_skip;	
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->right = id;
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + num_vbits*CMP_GATES + MUX_GATES*k + gates_skip;
					(m_pGates+id)->p_ids[1] = tmpid;
					(m_pGates+tmpid)->left = id;
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + num_vbits*CMP_GATES + MUX_GATES*k +2 + gates_skip;
					(m_pGates+id)->p_ids[2] = tmpid;
					(m_pGates+tmpid)->left = id;
				}
				else{
					(m_pGates+id)->p_num = 1;
					(m_pGates+id)->p_ids = New(1);
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + CMP_GATES*k + gates_skip;	
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->right = id;
				}
			}
			else{
				if (is_moreMUXvalue){
					(m_pGates+id)->p_num = 3;
					(m_pGates+id)->p_ids = New(3);
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + CMP_GATES*k+1;	
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->right = id;
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + CMP_GATES*k+1 +2;	
					(m_pGates+id)->p_ids[1] = tmpid;
					(m_pGates+tmpid)->left = id;
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + num_vbits*CMP_GATES + MUX_GATES*k;
					(m_pGates+id)->p_ids[2] = tmpid;
					(m_pGates+tmpid)->right = id;
				}
				else{
					(m_pGates+id)->p_num = 2;
					(m_pGates+id)->p_ids = New(2);
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + CMP_GATES*k+1;	
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->right = id;
					tmpid = id + (num_vbits-k-1)*MUX_GATES+1 + num_bits*MUX_GATES + CMP_GATES*k+1 +2;	
					(m_pGates+id)->p_ids[1] = tmpid;
					(m_pGates+tmpid)->left = id;
				}
			}
		}
	}
	else{
		puts("CP2PCircuit::CreateMUXValue(int,int,int,int): the function was called despite the final vs");
		exit(-1);
	}

	return id;
}

// MUX circuit (for indexes)
// input: start gate id, if or not the vs is final, # of bits, if or not vs is left, current layer#, # of gates to skip,
//			whether or not the higher layer's vs uses MUX_value circuit,
//			vs number from the leftmost (on the current layer) of the tournament starting from 0,
//			whether vs is a pair or a single, number of parties
// output: final gate id
int CP2PCircuit::CreateMUXIndex(int id, int is_final, int num_bits, int num_vbits, int is_left, int layer_no, int gates_skip, int is_moreMUXvalue, int* vs_no, int is_alone, int nParties)
{
	int idx1=0;	// Note: actual index starts from 1
	int idx2=0;
	int* idx1_bits = new int[num_bits];
	int* idx2_bits = new int[num_bits];
	
	int tmpid = -1;
	int num_outgates = 0;
	// gets each input index's binary bits
	if (layer_no==0){
		for (int i=0;i<num_bits;i++){
			idx1_bits[i] = 0;
		}
		idx1 = vs_no[0]*2+1;
		Int2Bits(idx1, idx1_bits);
		if(!is_alone){
			for (int i=0;i<num_bits;i++){
				idx2_bits[i] = 0;
			}
			idx2 = vs_no[0]*2+2;	
			Int2Bits(idx2, idx2_bits);
		}
	}
	// another MUX circuit (for index)
	for (int k=0;k<num_bits;k++){
		(m_pGates+id)->type = G_XOR;
		m_nNumXORs++;
		(m_pGates+id)->p_num = 1;
		(m_pGates+id)->p_ids = New(1);
		tmpid = id + 1;
		(m_pGates+id)->p_ids[0] = tmpid;
		(m_pGates+tmpid)->right = id;
		
		// for provider and customer IN gates
		if (layer_no == 0){
			if(!is_alone){
				(m_pGates+id)->left = idx1_bits[k];
				(m_pGates+id)->right = idx2_bits[k];
			}
			else{
				(m_pGates+id)->right = idx1_bits[k];
			}
		}

		id++;
		(m_pGates+id)->type = G_AND;
		(m_pGates+id)->p_num = 1;
		(m_pGates+id)->p_ids = New(1);
		tmpid = id + 1;
		(m_pGates+id)->p_ids[0] = tmpid;
		(m_pGates+tmpid)->right = id;
		id++;
		(m_pGates+id)->type = G_XOR;
		m_nNumXORs++;
		// for provider and customer IN gates
		if (layer_no == 0 && !is_alone){
			(m_pGates+id)->left = idx1_bits[k];
		}
		if (!is_final){
			if (is_left){
				if(is_moreMUXvalue){
					(m_pGates+id)->p_num = 2;
					(m_pGates+id)->p_ids = New(2);
					tmpid = id + (num_bits-k-1)*MUX_GATES+1	+ num_vbits*CMP_GATES + num_vbits*MUX_GATES + MUX_GATES*k + gates_skip;
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->left = id;
					tmpid = id + (num_bits-k-1)*MUX_GATES+1 + num_vbits*CMP_GATES + num_vbits*MUX_GATES + MUX_GATES*k +2 + gates_skip;
					(m_pGates+id)->p_ids[1] = tmpid;
					(m_pGates+tmpid)->left = id;
				}
				else{
					(m_pGates+id)->p_num = 2;
					(m_pGates+id)->p_ids = New(2);
					tmpid = id + (num_bits-k-1)*MUX_GATES+1	+ num_vbits*CMP_GATES + MUX_GATES*k + gates_skip;
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->left = id;
					
					#ifdef _DEBUG
					cout << "last left index goes to:"<< tmpid << endl;
					#endif
					tmpid = id + (num_bits-k-1)*MUX_GATES+1 + num_vbits*CMP_GATES + MUX_GATES*k +2 + gates_skip;
					(m_pGates+id)->p_ids[1] = tmpid;
					(m_pGates+tmpid)->left = id;
				}
			}
			else{
				if(is_moreMUXvalue){
					(m_pGates+id)->p_num = 1;
					(m_pGates+id)->p_ids = New(1);
					tmpid = id + (num_bits-k-1)*MUX_GATES+1	+ num_vbits*CMP_GATES + num_vbits*MUX_GATES + MUX_GATES*k;
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->right = id;
				}
				else{
					(m_pGates+id)->p_num = 1;
					(m_pGates+id)->p_ids = New(1);
					tmpid = id + (num_bits-k-1)*MUX_GATES+1	+ num_vbits*CMP_GATES + MUX_GATES*k;
					(m_pGates+id)->p_ids[0] = tmpid;
					(m_pGates+tmpid)->right = id;

					#ifdef _DEBUG
					cout << "last right index goes to:"<< tmpid << endl;
					#endif
				}
			}
		}
		else{
			// Final Output!!!!
			(m_pGates+id)->p_num = 1;
			(m_pGates+id)->p_ids = New(1);
			tmpid = m_vOutputStart[nParties-1]+num_outgates;
			(m_pGates+id)->p_ids[0] = tmpid;
			//cout << "Output Gate id:" << id << endl;
			//cout << "Final Output Gate id:" << tmpid << endl;
			(m_pGates+tmpid)->left = id;
			num_outgates++;
		}
		id++;
	} 
	return id;
}

// changes integer into binary bits
// input: integer decimal, array for binary bits result
void CP2PCircuit::Int2Bits(int dec, int* buf){
	for(int i=0; dec ;i++){
        buf[i] = dec%2;
        dec /= 2;
    }
}
 
// Calculates log2 of number.  
double CP2PCircuit::Log2(double n)  
{  
    // log(n)/log(2) is log2.  
    return log(n) / log(2.0);  
}



int	CP2PCircuit::GetInputStartC(int nPlayerID)
{
	if( nPlayerID < m_nNumServers )
	{
		return m_serStartGate+(nPlayerID*m_nNumItems*m_vNumVBits[nPlayerID]);
	}
	else
	{
		//client
		return m_cliStartGate;
	}
}

int CP2PCircuit::GetInputEndC(int nPlayerID)
{
	if( nPlayerID < m_nNumServers - 1 )
	{
		return GetInputStartC(nPlayerID+1)-1; 
	}
	else if (nPlayerID == m_nNumServers - 1)
	{
		return m_cliStartGate-1;
	}
	else
	{
		//client
		return m_othStartGate-1;
	}
}