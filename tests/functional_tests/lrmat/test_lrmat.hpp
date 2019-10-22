#include <iostream>
#include <complex>
#include <vector>

#include <htool/lrmat/sympartialACA.hpp>


using namespace std;
using namespace htool;

class MyMatrix: public IMatrix<double>{
	const vector<R3>& p1;
	const vector<R3>& p2;

public:
	MyMatrix(const vector<R3>& p10,const vector<R3>& p20 ):IMatrix<double>(p10.size(),p20.size()),p1(p10),p2(p20) {}

	double get_coef(const int& i, const int& j)const {return 1./(4*M_PI*norm2(p1[i]-p2[j]));}
	
	std::vector<double> operator*(std::vector<double>& a) const{
		std::vector<double> result(p1.size(),0);
		for (int i=0;i<p1.size();i++){
			for (int k=0;k<p2.size();k++){
				result[i]+=this->get_coef(i,k)*a[k];
			}
		}
		return result;
	 }
};

void create_geometry(int distance, std::vector<R3>& xt, std::vector<int>& tabt, std::vector<R3>& xs, std::vector<int>& tabs){
	cout << "Distance between the clusters: " << NbrToStr(distance) << endl;
	
	srand (1);
	// we set a constant seed for rand because we want always the same result if we run the check many times
	// (two different initializations with the same seed will generate the same succession of results in the subsequent calls to rand)

	int nr = xt.size();
	int nc = xs.size();
	vector<int> Ir(nr); // row indices for the lrmatrix
	vector<int> Ic(nc); // column indices for the lrmatrix

	double z1 = 1;
	for(int j=0; j<nr; j++){
		Ir[j] = j;
		double rho = ((double) rand() / (double)(RAND_MAX)); // (double) otherwise integer division!
		double theta = ((double) rand() / (double)(RAND_MAX));
		xt[j][0] = sqrt(rho)*cos(2*M_PI*theta); xt[j][1] = sqrt(rho)*sin(2*M_PI*theta); xt[j][2] = z1;
		// sqrt(rho) otherwise the points would be concentrated in the center of the disk
		tabt[j]=j;
	}
	// p2: points in a unit disk of the plane z=z2
	double z2 = 1+distance;
	vector<int> tab2(nc);
	for(int j=0; j<nc; j++){
		Ic[j] = j;
		double rho = ((double) rand() / (RAND_MAX)); // (double) otherwise integer division!
		double theta = ((double) rand() / (RAND_MAX));
		xs[j][0] = sqrt(rho)*cos(2*M_PI*theta); xs[j][1] = sqrt(rho)*sin(2*M_PI*theta); xs[j][2] = z2;
		tabs[j]=j;
	}
}

template<class LowRankMatrix >
int test_lrmat(const MyMatrix& A,const LowRankMatrix& Fixed_approximation, const LowRankMatrix& Auto_approximation, std::vector<int>& permt, std::vector<int>& perms, std::pair<double,double> fixed_compression_interval, std::pair<double,double> auto_compression_interval){

	bool test = 0;
	int nr=permt.size();
	int nc=perms.size();
	


	// ACA with fixed rank
	int reqrank_max = 10;
	std::vector<double> fixed_errors;
	for (int k = 0 ; k < Fixed_approximation.rank_of()+1 ; k++){
		fixed_errors.push_back(Frobenius_absolute_error(Fixed_approximation,A,k));
	}

	cout << "Compression with fixed rank" << endl;
	// Test rank
	cout << "> rank : "<<Fixed_approximation.rank_of() << endl;
	test = test || !(Fixed_approximation.rank_of()==reqrank_max);

	// Test Frobenius errors
	test = test || !(fixed_errors.back()<1e-8);
	cout << "> Errors with Frobenius norm : "<<fixed_errors<<endl;

	// Test compression
	test = test || !(fixed_compression_interval.first<Fixed_approximation.compression() && Fixed_approximation.compression()<fixed_compression_interval.second);
	cout << "> Compression rate : "<<Fixed_approximation.compression()<<endl;

	// Test mat vec prod
	std::vector<double> f(nc,1),out_perm(nr);
	std::vector<double> out=Fixed_approximation*f;
	for (int i = 0; i<permt.size();i++){
		out_perm[permt[i]]=out[i];
	}
	double error=norm2(A*f-out_perm);
	test = test || !(error<1e-7);
	cout << "> Errors on a mat vec prod : "<< error<<endl;

	// ACA automatic building
	std::vector<double> auto_errors;
	for (int k = 0 ; k < Auto_approximation.rank_of()+1 ; k++){
		auto_errors.push_back(Frobenius_absolute_error(Auto_approximation,A,k));
	}

	cout << "Automatic compression" << endl;
	// Test Frobenius error
	test = test || !(auto_errors[Auto_approximation.rank_of()]<GetEpsilon());
	cout << "> Errors with Frobenius norm: "<<auto_errors<<endl;

	// Test compression rate
	test = test || !(auto_compression_interval.first<Auto_approximation.compression() && Auto_approximation.compression()<auto_compression_interval.second);
	cout << "> Compression rate : "<<Auto_approximation.compression()<<endl;

	// Test mat vec prod
	out=Auto_approximation*f;
	for (int i = 0; i<permt.size();i++){
		out_perm[permt[i]]=out[i];
	}
	error = norm2(A*f-out_perm);
	test = test || !(error<GetEpsilon());
	cout << "> Errors on a mat vec prod : "<< error<<endl;

	cout << "test : "<<test<<endl<<endl;
	return test;
}