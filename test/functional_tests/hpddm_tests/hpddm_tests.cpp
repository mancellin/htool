#include <htool/wrapper_hpddm.hpp>
#include <htool/fullACA.hpp>



using namespace std;
using namespace htool;


class MyMatrix: public IMatrix<double>{
	const vector<R3>& p1;
	const vector<R3>& p2;

public:
	MyMatrix(const vector<R3>& p10,const vector<R3>& p20 ):IMatrix(p10.size(),p20.size()),p1(p10),p2(p20) {}

	double get_coef(const int& i, const int& j)const {return 1./(4*M_PI*norm(p1[i]-p2[j]));}


  std::vector<double> operator*(std::vector<double> a){
		std::vector<double> result(p1.size(),0);
		for (int i=0;i<p1.size();i++){
			for (int k=0;k<p2.size();k++){
				result[i]+=this->get_coef(i,k)*a[k];
			}
		}
		return result;
	 }
};


int main(){

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);

	// Get the number of processes
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Get the rank of the process
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//
	bool test = 0;
	const int ndistance = 4;
	double distance[ndistance];
	distance[0] = 10; distance[1] = 20; distance[2] = 30; distance[3] = 40;
	SetNdofPerElt(1);
	SetEpsilon(1e-8);
	SetEta(-0.1);

	for(int idist=0; idist<ndistance; idist++)
	{
		// cout << "Distance between the clusters: " << distance[idist] << endl;

		srand (1);
		// we set a constant seed for rand because we want always the same result if we run the check many times
		// (two different initializations with the same seed will generate the same succession of results in the subsequent calls to rand)

		int nr = 10;
		int nc = 10;
		vector<int> Ir(nr); // row indices for the lrmatrix
		vector<int> Ic(nc); // column indices for the lrmatrix

		double z1 = 1;
		vector<R3>     p1(nr);
		vector<int>  tab1(nr);
		for(int j=0; j<nr; j++){
			Ir[j] = j;
			double rho = ((double) rand() / (double)(RAND_MAX)); // (double) otherwise integer division!
			double theta = ((double) rand() / (double)(RAND_MAX));
			p1[j][0] = sqrt(rho)*cos(2*M_PI*theta); p1[j][1] = sqrt(rho)*sin(2*M_PI*theta); p1[j][2] = z1;
			// sqrt(rho) otherwise the points would be concentrated in the center of the disk
			tab1[j]=j;
		}
		// p2: points in a unit disk of the plane z=z2
		double z2 = 1+distance[idist];
		vector<R3> p2(nc);
		vector<int> tab2(nc);
		for(int j=0; j<nc; j++){
            Ic[j] = j;
			double rho = ((double) rand() / (RAND_MAX)); // (double) otherwise integer division!
			double theta = ((double) rand() / (RAND_MAX));
			p2[j][0] = sqrt(rho)*cos(2*M_PI*theta); p2[j][1] = sqrt(rho)*sin(2*M_PI*theta); p2[j][2] = z2;
			tab2[j]=j;
		}

		MyMatrix A(p1,p2);
		HMatrix<fullACA,double> HA(A,p1,tab1,p2,tab2);
		std::vector<double> x_ref(nc,1),x_test(nc,1),f(nr,1);
    // f = HA*x_ref;
    cout << "f "<<f<<std::endl;
    HPDDMOperator<fullACA,double> A_HPDDM(HA);
    double* const rhs = &(f[0]);
    double* x = &(x_test[0]);
    HPDDM::IterativeMethod::solve(A_HPDDM, rhs, x, 1,HA.get_comm());
    cout << "x_test "<<x_test<<endl;
    cout << "x_ref  "<<x_ref<<endl;
		double erreur2 = norm2(A*x_ref-f);
    double inv_erreur2 = norm2(x_ref-x_test);
		// double erreurFrob = Frobenius_absolute_error(HA,A);
		// double compression = HA.compression();
		// int nb_lrmat = HA.get_nlrmat();
		// int nb_dmat  = HA.get_ndmat();
    // if (rank==0){
    //   cout <<"error on mat vec prod : "<<erreur2 << endl;
    //   cout <<"error on inversion : "<<inv_erreur2 << endl;
    //
    // }
		// test = test || !(erreurFrob<GetEpsilon()*10);
		// test = test || !(erreur2<GetEpsilon()*100);

	}
	// Finalize the MPI environment.
	MPI_Finalize();
	return test;
}