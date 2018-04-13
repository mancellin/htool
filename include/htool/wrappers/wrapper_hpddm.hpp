#ifndef HTOOL_WRAPPER_HPDDM_HPP
#define HTOOL_WRAPPER_HPDDM_HPP

#define HPDDM_NUMBERING 'F'
#define HPDDM_SCHWARZ 1
#define HPDDM_FETI 0
#define HPDDM_BDD 0
#define LAPACKSUB
#define DSUITESPARSE
#include <HPDDM.hpp>
#include "../types/hmatrix.hpp"
#include "../types/matrix.hpp"
#include "../solvers/proto_ddm.hpp"

namespace htool{

template<template<typename> class LowRankMatrix, typename T>
class DDM;

template<template<typename> class LowRankMatrix, typename T>
class Proto_DDM;

template< template<typename> class LowRankMatrix, typename T>
class HPDDMDense : public HpDense<T> {
private:
    const HMatrix<LowRankMatrix,T>& HA;
    std::vector<T>* in_global,*buffer;


public:
    typedef  HpDense<T> super;

    HPDDMDense(const HMatrix<LowRankMatrix,T>& A):HA(A){
        in_global = new std::vector<T> ;
        buffer = new std::vector<T>;
    }
    ~HPDDMDense(){delete in_global;delete buffer;}


    void GMV(const T* const in, T* const out, const int& mu = 1) const {
        int local_size = HA.get_local_size();

        // Tranpose without overlap
        if (mu!=1){
            for (int i=0;i<mu;i++){
                for (int j=0;j<local_size;j++){
                    (*buffer)[i+j*mu]=in[i*this->getDof()+j];
                }
            }
        }

        // All gather
        if (mu==1){// C'est moche
            HA.mvprod_local(in,out,in_global->data(),mu);
        }
        else{
            HA.mvprod_local(buffer->data(),buffer->data()+local_size*mu,in_global->data(),mu);
        }



        // Tranpose
        if (mu!=1){
            for (int i=0;i<mu;i++){
                for (int j=0;j<local_size;j++){
                    out[i*this->getDof()+j]=(*buffer)[i+j*mu+local_size*mu];
                }
            }
        }
        this->scaledExchange(out, mu);
    }

    void exchange(T* const out, const int& mu = 1){
    this->template scaledExchange<true>(out, mu);
    }

    friend class DDM<LowRankMatrix,T>;

};

template< template<typename> class LowRankMatrix, typename T>
class Proto_HPDDM : public HPDDM::EmptyOperator<T> {
private:
    const HMatrix<LowRankMatrix,T>& HA;
    std::vector<T>* in_global;
    htool::Proto_DDM<LowRankMatrix,T>& P;

public:

    Proto_HPDDM(const HMatrix<LowRankMatrix,T>& A,  Proto_DDM<LowRankMatrix,T>& P0):HPDDM::EmptyOperator<T>(A.get_local_size()),HA(A),P(P0){
        in_global = new std::vector<T> (A.nb_cols());
    }
    ~Proto_HPDDM(){delete in_global;}

    void GMV(const T* const in, T* const out, const int& mu = 1) const {
        HA.mvprod_local(in,out,in_global->data(),1);
    }

    template<bool = true>
    void apply(const T* const in, T* const out, const unsigned short& mu = 1, T* = nullptr, const unsigned short& = 0) const {
        P.apply(in,out);
        // std::copy_n(in, this->_n, out);
    }


    void solve(const T* const rhs, T* const x, const int& mu=1 ){
        //
        int rankWorld = HA.get_rankworld();
        int sizeWorld = HA.get_sizeworld();
        int offset  = HA.get_local_offset();
        int nb_cols = HA.nb_cols();
        int nb_rows = HA.nb_rows();
        double time = MPI_Wtime();
        int n = P.get_n();
        int n_inside = P.get_n_inside();

        //
        std::vector<T> rhs_perm(nb_cols);
        std::vector<T> x_local(n,0);
        std::vector<T> local_rhs(n,0);

        // Permutation
        HA.source_to_cluster_permutation(rhs,rhs_perm.data());
        std::copy_n(rhs_perm.begin()+offset,n_inside,local_rhs.begin());


        // TODO avoid com here
        // for (int i=0;i<n-n_inside;i++){
        //   local_rhs[i]=rhs_perm[]
        // }
        // this->exchange(local_rhs.data(), mu);

        // Solve
        int nb_it = HPDDM::IterativeMethod::solve(*this, local_rhs.data(), x_local.data(), mu,HA.get_comm());

        // // Delete the overlap (useful only when mu>1 and n!=n_inside)
        // for (int i=0;i<mu;i++){
        //     std::copy_n(x_local.data()+i*n,n_inside,local_rhs.data()+i*n_inside);
        // }

        // Local to global
        // hpddm_op.HA.local_to_global(x_local.data(),hpddm_op.in_global->data(),mu);
        std::vector<int> recvcounts(sizeWorld);
        std::vector<int>  displs(sizeWorld);

        displs[0] = 0;

        for (int i=0; i<sizeWorld; i++) {
        recvcounts[i] = (HA.get_MasterOffset_t(i).second);
        if (i > 0)
            displs[i] = displs[i-1] + recvcounts[i-1];
        }

        MPI_Allgatherv(x_local.data(), recvcounts[rankWorld], wrapper_mpi<T>::mpi_type(), in_global->data() , &(recvcounts[0]), &(displs[0]), wrapper_mpi<T>::mpi_type(), HA.get_comm());

        // Permutation
        HA.cluster_to_target_permutation(in_global->data(),x);

    }

};

}
#endif
