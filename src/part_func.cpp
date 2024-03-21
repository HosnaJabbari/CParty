#include "part_func.hh"
#include "pf_globals.hh"
#include "h_externs.hh"

#include <string>
#include <iostream>
#include <stdio.h>
#include <math.h>

/*
 * If the global use_mfelike_energies flag is set, truncate doubles to int
 * values and cast back to double. This makes the energy parameters of the
 * partition (folding get_scaled_exp_params()) compatible with the mfe folding
 * parameters (get_scaled_exp_params()), e.g. for explicit partition function
 * computations.
 */
#define TRUNC_MAYBE(X) ((!pf_smooth) ? (double)((int)(X)) : (X))
/* Rescale Free energy contribution according to deviation of temperature from measurement conditions */
#define RESCALE_dG(dG, dH, dT)   ((dH) - ((dH) - (dG)) * dT)

/*
 * Rescale Free energy contribution according to deviation of temperature from measurement conditions
 * and convert it to Boltzmann Factor for specific kT
 */
#define RESCALE_BF(dG, dH, dT, kT)          ( \
    exp( \
      -TRUNC_MAYBE((double)RESCALE_dG((dG), (dH), (dT))) \
      * 10. \
      / kT \
      ) \
    )


W_final_pf::W_final_pf(std::string seq, bool pk_free, int dangle) : exp_params_(scale_pf_parameters())
{
    this->seq = seq;
    this->n = seq.length();
    this->pk_free = pk_free;

    make_pair_matrix();
    exp_params_->model_details.dangles = dangle;
    S_ = encode_sequence(seq.c_str(),0);
	S1_ = encode_sequence(seq.c_str(),1);

    index.resize(n+1);
    cand_pos_t total_length = ((n+1) *(n+2))/2;
    index[1] = 0;
    for (cand_pos_t i=2; i <= n; i++)
        index[i] = index[i-1]+(n+1)-i+1;
    // Allocate space
    V.resize(total_length,0);
    WM.resize(total_length,0);
    WMv.resize(total_length,0);
    WMp.resize(total_length,0);
    W.resize(n+1,1);

    // PK
    WI.resize(total_length,1);
    WIP.resize(total_length,0);
    VP.resize(total_length,0);
    VPL.resize(total_length,0);
    VPR.resize(total_length,0);
    WMB.resize(total_length,0);
    WMBP.resize(total_length,0);
    WMBW.resize(total_length,0);
    BE.resize(total_length,0);


    rescale_pk_globals();
}

W_final_pf::~W_final_pf(){

}

void W_final_pf::rescale_pk_globals(){
    double kT = exp_params_->model_details.betaScale * ( exp_params_->model_details.temperature + K0) * GASCONST; /* kT in cal/mol  */
    double TT = (exp_params_->model_details.temperature + K0) / (Tmeasure);
    int pf_smooth = exp_params_->model_details.pf_smooth;

    expPS_penalty = RESCALE_BF(PS_penalty,PS_penalty*3,TT,kT);
    expPSM_penalty = RESCALE_BF(PSM_penalty,PSM_penalty*3,TT,kT);
    expPSP_penalty = RESCALE_BF(PSP_penalty,PSP_penalty*3,TT,kT);		
    expPB_penalty = RESCALE_BF(PB_penalty,PB_penalty*3,TT,kT);
    expPUP_penalty = RESCALE_BF(PUP_penalty,PUP_penalty*3,TT,kT);
    expPPS_penalty= RESCALE_BF(PPS_penalty,PPS_penalty*3,TT,kT);

    expa_penalty = RESCALE_BF(a_penalty,ML_closingdH,TT,kT); 
    expb_penalty = RESCALE_BF(b_penalty,ML_interndH,TT,kT);
    expc_penalty = RESCALE_BF(c_penalty,ML_BASEdH,TT,kT);

    expap_penalty = RESCALE_BF(ap_penalty,ap_penalty*3,TT,kT);
    expbp_penalty = RESCALE_BF(bp_penalty,bp_penalty*3,TT,kT);
    expcp_penalty = RESCALE_BF(cp_penalty,cp_penalty*3,TT,kT);
}

double W_final_pf::hfold_pf(sparse_tree &tree){

    for (int i = n; i >=1; --i){	
			for (int j =i; j<=n; ++j){
                cand_pos_t ij = index[i]+j-i;

				const bool evaluate = tree.weakly_closed(i,j);
				const pair_type ptype_closing = pair[S_[i]][S_[j]];
				const bool restricted = tree.tree[i].pair == -1 || tree.tree[j].pair == -1;
				const bool paired = (tree.tree[i].pair == j && tree.tree[j].pair == i);

				if(ptype_closing> 0 && evaluate && !restricted)
				compute_energy_restricted (i,j,tree);

				if(!pk_free) compute_pk_energies(i,j,tree);

				compute_WMv_WMp(i,j,tree.tree);
				compute_energy_WM_restricted(i,j,tree);
		}

	}
    for (cand_pos_t j= TURN+1; j <= n; j++){
        pf_t contributions = 0;
		
		for (cand_pos_t k=1; k<=j-TURN-1; ++k){
			pf_t acc = (k>1) ? W[k-1]: 1; //keep as 0 or 1?
            pair_type tt  = pair[S_[k]][S_[j]];

			contributions += acc*get_energy(k,j)*exp_Extloop(k,j);//E_ext_Stem(V->get_energy(k,j),V->get_energy(k+1,j),V->get_energy(k,j-1),V->get_energy(k+1,j-1),S_,params_,k,j,n,tree.tree));
			if (k == 1 || (tree.weakly_closed(1,k-1) && tree.weakly_closed(k,j))) contributions += acc*get_energy_WMB(k,j)*expPS_penalty;
		}
        if(tree.tree[j].pair < 0) contributions += W[j-1];
        if(!tree.weakly_closed(1,j)) W[j] = 0;

		W[j] = contributions;
	}

    // std::cout << "cont:  " << W[n] << std::endl;

    double energy = (-log(W[n]))* exp_params_->kT / 1000.0;
    // std::cout << exp_params_->pf_scale << std::endl; = -1 maybe because we don't set a scale?


    return energy;
}

pf_t W_final_pf::exp_Extloop(cand_pos_t i, cand_pos_t j){
	pair_type tt  = pair[S_[i]][S_[j]];

	if(exp_params_->model_details.dangles == 1 || exp_params_->model_details.dangles == 2){
		base_type si1 = i>1 ? S_[i-1] : -1;
		base_type sj1 = j<n ? S_[j+1] : -1;
		return exp_E_ExtLoop(tt,si1,sj1,exp_params_);
	}
	else{
		return exp_E_ExtLoop(tt,-1,-1,exp_params_);
	} 
}

pf_t W_final_pf::exp_MLstem(cand_pos_t i, cand_pos_t j){
	pair_type tt  = pair[S_[i]][S_[j]];
	if(exp_params_->model_details.dangles == 1 || exp_params_->model_details.dangles == 2){
		base_type si1 = i>1 ? S_[i-1] : -1;
		base_type sj1 = j<n ? S_[j+1] : -1;
		return exp_E_MLstem(tt,si1,sj1,exp_params_);
	}
	else{
		return exp_E_MLstem(tt,-1,-1,exp_params_);
	} 
}

pf_t W_final_pf::exp_Mbloop(cand_pos_t i, cand_pos_t j){
	pair_type tt  = pair[S_[j]][S_[i]];
	if(exp_params_->model_details.dangles == 1 || exp_params_->model_details.dangles == 2){
		base_type si1 = i>1 ? S_[i-1] : -1;
		base_type sj1 = j<n ? S_[j+1] : -1;
		return exp_E_MLstem(tt,sj1,si1,exp_params_);
	}
	else{
		return exp_E_MLstem(tt,-1,-1,exp_params_);
	} 
}

pf_t W_final_pf::HairpinE(cand_pos_t i, cand_pos_t j){
    
    const int ptype_closing = pair[S_[i]][S_[j]];
    if (ptype_closing==0) return 0;
    return static_cast<pf_t>(exp_E_Hairpin(j-i-1,ptype_closing,S1_[i+1],S1_[j-1],&seq.c_str()[i-1],exp_params_));
}

pf_t W_final_pf::compute_internal_restricted(cand_pos_t i, cand_pos_t j, std::vector<int> &up){
    pf_t v_iloop = 0;
    cand_pos_t max_k = std::min(j-TURN-2,i+MAXLOOP+1);
	const int ptype_closing = pair[S_[i]][S_[j]];
	for ( cand_pos_t k=i+1; k<=max_k; ++k) {
		cand_pos_t min_l=std::max(k+TURN+1 + MAXLOOP+2, k+j-i) - MAXLOOP-2;
        if((up[k-1]>=(k-i-1))){
            for (cand_pos_t l=j-1; l>=min_l; --l) {
                if(up[j-1]>=(j-l-1)){
                    v_iloop += exp_E_IntLoop(k-i-1,j-l-1,ptype_closing,rtype[pair[S_[k]][S_[l]]],S1_[i+1],S1_[j-1],S1_[k-1],S1_[l+1],exp_params_)*get_energy(k,l);
                }
            }
        }
	}


    return v_iloop;
}

void W_final_pf::compute_WMv_WMp(cand_pos_t i, cand_pos_t j, std::vector<Node> &tree){
	if(j-i+1<4) return;
	cand_pos_t ij = index[(i)]+(j)-(i);
	cand_pos_t ijminus1 = index[(i)]+(j)-1-(i);

    pf_t WMv_contributions = 0;
    pf_t WMp_contributions = 0;


    WMv_contributions += (get_energy(i,j)*exp_MLstem(i,j));
	WMp_contributions += (get_energy_WMB(i,j)*expPSM_penalty*expb_penalty);
	if (tree[j].pair < 0)
	{
		WMv_contributions += (WMv[ijminus1]*exp_params_->expMLbase);
		WMp_contributions += (WMp[ijminus1]*exp_params_->expMLbase);
	}
    WMv[ij] = WMv_contributions;
    WMp[ij] = WMp_contributions;
}

void W_final_pf::compute_energy_WM_restricted (cand_pos_t i, cand_pos_t j, sparse_tree &tree){
    if(j-i+1<4) return;
    pf_t contributions = 0;
	cand_pos_t ij = index[(i)]+(j)-(i);
	cand_pos_t ijminus1 = index[(i)]+(j)-1-(i);

	for (cand_pos_t k=i; k < j -TURN-1; k++)
	{
		bool can_pair = tree.up[k-1] >= (k-i);
		if(can_pair) contributions += (static_cast<pf_t>(pow(exp_params_->expMLbase,(k-i)))*get_energy_WMv(k,j));
		if(can_pair) contributions += (static_cast<pf_t>(pow(exp_params_->expMLbase,(k-i)))*get_energy_WMp(k,j));
		contributions += (get_energy_WM(i,k-1)*get_energy_WMv(k,j));
		contributions += (get_energy_WM(i,k-1)*get_energy_WMp(k,j));
	}
	if (tree.tree[j].pair < 0) contributions += WM[ijminus1]*exp_params_->expMLbase;

    WM[ij] = contributions;
}

pf_t W_final_pf::compute_energy_VM_restricted (cand_pos_t i, cand_pos_t j, std::vector<Node> &tree){
    pf_t contributions = 0;
    for (cand_pos_t k = i+1; k <= j-3; ++k)
    {
        contributions += (get_energy_WM(i+1,k-1)*get_energy_WMv(k,j-1)*exp_Mbloop(i,j)*exp_params_->expMLclosing); // was sj1,si1
        contributions += (get_energy_WM(i+1,k-1)*get_energy_WMp(k,j-1)*exp_Mbloop(i,j)*exp_params_->expMLclosing);
        contributions += (static_cast<pf_t>(pow(exp_params_->expMLbase,(k-i)))*get_energy_WMp(k,j-1)*exp_Mbloop(i,j)*exp_params_->expMLclosing);
    }

    return contributions;
}

void W_final_pf::compute_energy_restricted(cand_pos_t i,cand_pos_t j,sparse_tree &tree){

    cand_pos_t ij = index[i]+j-i;

    const pair_type ptype_closing = pair[S_[i]][S_[j]];
    const bool unpaired = (tree.tree[i].pair<-1 && tree.tree[j].pair<-1);
	const bool paired = (tree.tree[i].pair == j && tree.tree[j].pair == i);

    pf_t contributions = 0;

    if (paired || unpaired)    // if i and j can pair
    {
        bool canH = !(tree.up[j-1]<(j-i-1));
        if(canH) contributions += HairpinE(i,j);

        contributions += compute_internal_restricted(i,j,tree.up);

        contributions += compute_energy_VM_restricted(i,j,tree.tree);
    }   

    V[ij] = contributions;
}

void W_final_pf::compute_pk_energies(cand_pos_t i,cand_pos_t j,sparse_tree &tree){

    cand_pos_t ij = index[i]+j-i;
	const pair_type ptype_closing = pair[S_[i]][S_[j]];
	bool weakly_closed_ij = tree.weakly_closed(i,j);

	if ((i == j || j-i<4 || weakly_closed_ij))	{
		VP[ij] = 0;
		VPL[ij] = 0;
		VPR[ij] = 0;
	}
	else{
		if(ptype_closing>0 && tree.tree[i].pair < -1 && tree.tree[j].pair < -1) compute_VP(i,j,tree);
		compute_VPL(i,j,tree);
		compute_VPR(i,j,tree);
	}

	if (!((j-i-1) <= TURN || (tree.tree[i].pair >= -1 && tree.tree[i].pair > j) || (tree.tree[j].pair >= -1 && tree.tree[j].pair < i) || (tree.tree[i].pair >= -1 && tree.tree[i].pair < i ) || (tree.tree[j].pair >= -1 && j < tree.tree[j].pair))){
		compute_WMBW(i,j,tree);
		compute_WMBP(i,j,tree);
		compute_WMB(i,j,tree);
	}

	if(!weakly_closed_ij){
		WI[ij] = 0;
		WIP[ij] = 0;
	}
	else{
		compute_WI(i,j,tree);
		compute_WIP(i,j,tree);
	}
	cand_pos_t ip = tree.tree[i].pair; // i's pair ip should be right side so ip = )
	cand_pos_t jp = tree.tree[j].pair; // j's pair jp should be left side so jp = (
	compute_BE(i,ip,jp,j,tree);
    cand_pos_t ijp = index[i]+jp-i;

}

void W_final_pf::compute_WI(cand_pos_t i,cand_pos_t j,sparse_tree &tree){

    cand_pos_t ij = index[i]+j-i;
    pf_t contributions = 0;
    if(i==j){
        WI[ij] = expPUP_penalty;
        return;
    }
    contributions += (get_energy(i,j)*expPPS_penalty);
    contributions += (get_energy_WMB(i,j)*expPSP_penalty*expPPS_penalty);

    for (cand_pos_t k = i+1; k < j-TURN-1; ++k){
        contributions += (get_energy_WI(i,k-1)*get_energy(k,j)*expPPS_penalty);
        contributions += (get_energy_WI(i,k-1)*get_energy_WMB(k,j)*expPSP_penalty*expPPS_penalty);
    }
    if (tree.tree[j].pair < 0) contributions +=  (get_energy_WI(i,j-1)*expPUP_penalty); 


    WI[ij] = contributions;
}

void W_final_pf::compute_WIP(cand_pos_t i,cand_pos_t j,sparse_tree &tree){

    cand_pos_t ij = index[i]+j-i;
    pf_t contributions = 0;
    contributions += get_energy(i,j)*expbp_penalty;
    contributions += get_energy_WMB(i,j)*expbp_penalty*expPSM_penalty;
    for (cand_pos_t k = i+1; k < j-TURN-1; ++k){
		bool can_pair = tree.up[k-1] >= (k-i);

        contributions += (get_energy_WIP(i,k-1)*get_energy(k,j)*expbp_penalty);
        contributions += (get_energy_WIP(i,k-1)*get_energy_WMB(k,j)*expb_penalty*expPSM_penalty);
        if(can_pair) contributions += (static_cast<pf_t>(pow(expcp_penalty,(k-i)))*get_energy(k,j)*expbp_penalty);
        if(can_pair) contributions += (static_cast<pf_t>(pow(expcp_penalty,(k-i)))*get_energy_WMB(k,j)*expbp_penalty*expPSM_penalty);

    }
    if (tree.tree[j].pair < 0) contributions += (get_energy_WIP(i,j-1)*expcp_penalty);
    WIP[ij] = contributions;

}

void W_final_pf::compute_VPL(cand_pos_t i, cand_pos_t j, sparse_tree &tree){

	cand_pos_t ij = index[i]+j-i;
	pf_t contributions = 0;

	cand_pos_t min_Bp_j = std::min((cand_pos_tu) tree.b(i,j), (cand_pos_tu) tree.Bp(i,j));
	for(cand_pos_t k = i+1; k<min_Bp_j; ++k){
		contributions += (static_cast<pf_t>(pow(expcp_penalty,(k-i)))*get_energy_VP(k,j));
	}


	VPL[ij] = contributions;

}

void W_final_pf::compute_VPR(cand_pos_t i, cand_pos_t j, sparse_tree &tree){

	cand_pos_t ij = index[i]+j-i;
	pf_t contributions = 0;

	cand_pos_t max_i_bp = std::max(tree.B(i,j),tree.bp(i,j));

	for(cand_pos_t k = max_i_bp+1; k<j; ++k){
		contributions += (get_energy_VP(i,k)*get_energy_WIP(k+1,j));
		contributions += (get_energy_VP(i,k)*static_cast<pf_t>(pow(expcp_penalty,(k-i))));
	}

	VPR[ij] = contributions;
}


void W_final_pf::compute_VP(cand_pos_t i, cand_pos_t j, sparse_tree &tree){
	cand_pos_t ij = index[i]+j-i;

	const pair_type ptype_closing = pair[S_[i]][S_[j]];	
    pf_t contributions = 0;
	
	// Borders -- added one to i and j to make it fit current bounds but also subtracted 1 from answer as the tree bounds are shifted as well
	cand_pos_t Bp_ij = tree.Bp(i,j);
	cand_pos_t B_ij = tree.B(i,j);
	cand_pos_t b_ij = tree.b(i,j);
	cand_pos_t bp_ij = tree.bp(i,j);
	
	if((tree.tree[i].parent->index) > 0 && (tree.tree[j].parent->index) < (tree.tree[i].parent->index) && Bp_ij >= 0 && B_ij >= 0 && bp_ij < 0){
        contributions += (get_energy_WI(i+1,Bp_ij-1)*get_energy_WI(B_ij+1,j-1));
	}

	if ((tree.tree[i].parent->index) < (tree.tree[j].parent->index) && (tree.tree[j].parent->index) > 0 && b_ij>= 0 && bp_ij >= 0 && Bp_ij < 0){
        contributions += (get_energy_WI(i+1,b_ij-1)*get_energy_WI(bp_ij+1,j-1));
	}

	if((tree.tree[i].parent->index) > 0 && (tree.tree[j].parent->index) > 0 && Bp_ij >= 0 && B_ij >= 0  && b_ij >= 0 && bp_ij>= 0){
        contributions += (get_energy_WI(i+1,Bp_ij-1)*get_energy_WI(B_ij+1,b_ij-1)*get_energy_WI(bp_ij+1,j-1));
	}

	pair_type ptype_closingip1jm1 = pair[S_[i+1]][S_[j-1]];
	if((tree.tree[i+1].pair) < -1 && (tree.tree[j-1].pair) < -1 && ptype_closingip1jm1>0){
        contributions += (get_e_stP(i,j)*get_energy_VP(i+1,j-1));
	}

	cand_pos_t min_borders = std::min((cand_pos_tu) Bp_ij, (cand_pos_tu) b_ij);
	cand_pos_t edge_i = std::min(i+MAXLOOP+1,j-TURN-1);
	min_borders = std::min(min_borders,edge_i);
	for (cand_pos_t k = i+1; k < min_borders; ++k){
		if (tree.tree[k].pair < -1 && (tree.up[(k)-1] >= ((k)-(i)-1))){
			cand_pos_t max_borders = std::max(bp_ij,B_ij)+1;
			cand_pos_t edge_j = k+j-i-MAXLOOP-2;
			max_borders = std::max(max_borders,edge_j);
			for (cand_pos_t l = j-1; l > max_borders ; --l){
				pair_type ptype_closingkj = pair[S_[k]][S_[l]];
                if(k==i+1 && l==j-1) continue; // I have to add or else it will add a stP version and an eintP version to the sum
				if (tree.tree[l].pair < -1 && ptype_closingkj>0 && (tree.up[(j)-1] >= ((j)-(l)-1))){
					contributions += (get_e_intP(i,k,l,j)*get_energy_VP(k,l));					
				}
			}
		}
	}

		cand_pos_t min_Bp_j = std::min((cand_pos_tu) tree.b(i,j), (cand_pos_tu) tree.Bp(i,j));
		cand_pos_t max_i_bp = std::max(tree.B(i,j),tree.bp(i,j));

		for(cand_pos_t k = i+1; k<min_Bp_j; ++k){
			contributions += (get_energy_WIP(i+1,k-1)*get_energy_VP(k,j-1)*expap_penalty*pow(expbp_penalty,2)); // doing two more bp's -- one for VP and one for closing base pair I think?
		}
		

		for(cand_pos_t k = max_i_bp+1; k<j; ++k){
			contributions += (get_energy_VP(i+1,k)*get_energy_WIP(k+1,j-1)*expap_penalty*pow(expbp_penalty,2));
		}

		for(cand_pos_t k = i+1; k<min_Bp_j; ++k){
			contributions += (get_energy_WIP(i+1,k-1)*get_energy_VPR(k,j-1)*expap_penalty*pow(expbp_penalty,2));
		}

		for(cand_pos_t k = max_i_bp+1; k<j; ++k){
			contributions += (get_energy_VPL(i+1,k)*get_energy_WIP(k+1,j-1)*expap_penalty*pow(expbp_penalty,2));
		}

	VP[ij] = contributions;

}

pf_t W_final_pf::compute_int(cand_pos_t i, cand_pos_t j, cand_pos_t k, cand_pos_t l){
	const pair_type ptype_closing = pair[S_[i]][S_[j]];
    return  exp_E_IntLoop(k-i-1,j-l-1,ptype_closing,rtype[pair[S_[k]][S_[l]]],S1_[i+1],S1_[j-1],S1_[k-1],S1_[l+1],exp_params_);
}

pf_t W_final_pf::get_e_stP(cand_pos_t i, cand_pos_t j){
	if (i+1 == j-1){ // TODO: do I need something like that or stack is taking care of this?
		return 0;
	}
	pf_t e_st = compute_int(i,j,i+1,j-1);
    return pow(e_st,e_stP_penalty); 
}

pf_t W_final_pf::get_e_intP(cand_pos_t i, cand_pos_t ip, cand_pos_t jp, cand_pos_t j){
	if(ip==i+1 && jp==j-1) return 0;
    pf_t e_int = compute_int(i,j,ip,jp);
	return pow(e_int,e_intP_penalty);
}

void W_final_pf::compute_WMBW(cand_pos_t i, cand_pos_t j, sparse_tree &tree){
	cand_pos_t ij = index[i]+j-i;

	pf_t contributions = 0;

	if(tree.tree[j].pair < j){
		for(cand_pos_t l = i+1; l<j; l++){
			if (tree.tree[l].pair < 0 && tree.tree[l].parent->index > -1 && tree.tree[j].parent->index > -1 && tree.tree[j].parent->index == tree.tree[l].parent->index){
				contributions += get_energy_WMBP(i,l)*get_energy_WI(l+1,j);
			}
		}
	}
	WMBW[ij] = contributions;
}

void W_final_pf::compute_WMBP(cand_pos_t i, cand_pos_t j, sparse_tree &tree){
    cand_pos_t ij = index[i]+j-i;
    pf_t contributions = 0;

    if (tree.tree[j].pair < 0){
        for (cand_pos_t l = i+1; l<j ; l++)	{
            cand_pos_t bp_il = tree.bp(i,l);
            cand_pos_t Bp_lj = tree.Bp(l,j);
            if (bp_il >= 0 && l>bp_il && Bp_lj > 0 && l<Bp_lj){ 
                cand_pos_t B_lj = tree.B(l,j);
                if (i <= tree.tree[l].parent->index && tree.tree[l].parent->index < j && l+TURN <=j){
                    contributions += get_BE(tree.tree[B_lj].pair,B_lj,tree.tree[Bp_lj].pair,Bp_lj,tree)*get_energy_WMBP(i,l-1)*get_energy_VP(l,j)*pow(expPB_penalty,2);
                }
            }			
        }
    }

    if (tree.tree[j].pair < 0){
        for (cand_pos_t l = i+1; l<j ; l++)	{
            cand_pos_t bp_il = tree.bp(i,l);
            cand_pos_t Bp_lj = tree.Bp(l,j);
            if (bp_il >= 0 && l>bp_il && Bp_lj > 0 && l<Bp_lj){ 
                cand_pos_t B_lj = tree.B(l,j);
                if (i <= tree.tree[l].parent->index && tree.tree[l].parent->index < j && l+TURN <=j){
                    contributions += get_BE(tree.tree[B_lj].pair,B_lj,tree.tree[Bp_lj].pair,Bp_lj,tree)*get_energy_WMBW(i,l-1)*get_energy_VP(l,j)*pow(expPB_penalty,2);
                }
            }     
        }
	}

    contributions += get_energy_VP(i,j)*expPB_penalty; // Make sure not to use non-Partition values

    if(tree.tree[j].pair < 0 && tree.tree[i].pair >= 0){
		energy_t tmp = INF;
		for (cand_pos_t l = i+1; l < j; l++){
			cand_pos_t bp_il = tree.bp(i,l);
			if(bp_il >= 0 && bp_il < n && l+TURN <= j){
                contributions += get_BE(i,tree.tree[i].pair,bp_il,tree.tree[bp_il].pair,tree)*get_energy_WI(bp_il+1,l-1)*get_energy_VP(l,j)*pow(expPB_penalty,2);
			}
		}
	}

    WMBP[ij] = contributions;
}

void W_final_pf::compute_WMB(cand_pos_t  i, cand_pos_t  j, sparse_tree &tree){
	cand_pos_t ij = index[i]+j-i;
    pf_t contributions = 0;
	//base case
	if (i == j){
		WMB[ij] = 0;
		return;
	}

	energy_t m2 = INF, mWMBP = INF;

	if (tree.tree[j].pair >= 0 && j > tree.tree[j].pair){
		cand_pos_t bp_j = tree.tree[j].pair;
		for (cand_pos_t l = (bp_j +1); (l < j); l++){
			cand_pos_t Bp_lj = tree.Bp(l,j);
			if (Bp_lj >= 0 && Bp_lj<n){
                contributions += get_BE(bp_j,j,tree.tree[Bp_lj].pair,Bp_lj,tree)*get_energy_WMBP(i,l)*get_energy_WI(l+1,Bp_lj-1)*expPB_penalty;
			}
		}
	}
    contributions += get_energy_WMBP(i,j);

	WMB[ij] = contributions;	
}

void W_final_pf::compute_BE(cand_pos_t i, cand_pos_t j, cand_pos_t ip, cand_pos_t jp, sparse_tree &tree){

	if (!( i >= 1 && i <= ip && ip < jp && jp <= j && j <= n && tree.tree[i].pair > 0 && tree.tree[j].pair > 0 && tree.tree[ip].pair > 0 && tree.tree[jp].pair > 0 && tree.tree[i].pair == j && tree.tree[j].pair == i && tree.tree[ip].pair == jp && tree.tree[jp].pair == ip)){ //impossible cases
		return;
	}
	cand_pos_t iip = index[i]+ip-i;
    pf_t contributions = 0;
	// base case: i.j and ip.jp must be in G
	if (tree.tree[i].pair != j || tree.tree[ip].pair != jp){
		BE[iip] = 0;
		return;
	}

	// base case:
	if(i == ip && j == jp && i<j){
		BE[iip] = 1;
		return;
	}
    
    if (tree.tree[i+1].pair == j-1){
		contributions += get_e_stP(i,j)*get_BE(i+1,j-1,ip,jp,tree);
	}

	for (cand_pos_t l = i+1; l<= ip ; l++){
		if (tree.tree[l].pair >= -1 && jp <= tree.tree[l].pair && tree.tree[l].pair < j){

			cand_pos_t lp = tree.tree[l].pair;
			cand_pos_t il = index[i]+l-i;
			cand_pos_t lpj = index[lp]+j-lp;

			bool empty_region_il = (tree.up[(l)-1] >= l-i-1); //empty between i+1 and lp-1
			bool empty_region_lpj = (tree.up[(j)-1] >= j-lp-1); // empty between l+1 and ip-1
			bool weakly_closed_il = tree.weakly_closed(i+1,l-1); // weakly closed between i+1 and lp-1
			bool weakly_closed_lpj = tree.weakly_closed(lp+1,j-1); // weakly closed between l+1 and ip-1

			if (empty_region_il && empty_region_lpj){//&& !(ip == (i+1) && jp==(j-1)) && !(l == (i+1) && lp == (j-1))){
                contributions += get_e_intP(i,l,lp,j)*get_BE(l,lp,ip,jp,tree); // Added to e_intP that l != i+1 and lp != j-1 at the same time
			}
			if (weakly_closed_il && weakly_closed_lpj){
                contributions += get_energy_WIP(i+1,l-1)*get_BE(l,lp,ip,jp,tree)*get_energy_WIP(lp+1,j-1)*expap_penalty*pow(expbp_penalty,2);
			}
			if (weakly_closed_il && empty_region_lpj){
                contributions += get_energy_WIP(i+1,l-1)*get_BE(l,lp,ip,jp,tree)*static_cast<pf_t>(pow(expcp_penalty,(jp-l+1)))*expap_penalty*pow(bp_penalty,2);
			}
			if (empty_region_il && weakly_closed_lpj){
                contributions += static_cast<pf_t>(pow(expcp_penalty,(l-i+1)))*get_BE(l,lp,ip,jp,tree)*get_energy_WIP(lp+1,j-1)*expap_penalty*pow(bp_penalty,2);
			}
		}
	}


	BE[iip] = contributions;
}


