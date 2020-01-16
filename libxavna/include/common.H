#ifndef __XAVNA_COMMON_H
#define __XAVNA_COMMON_H
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/StdVector>
#include <vector>
using namespace Eigen;
using namespace std;


//EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(Matrix2cd);


namespace xaxaxa {
using VNARawValue = Matrix2cd;
using VNACalibratedValue = Matrix2cd;
inline Matrix2cd mirror(Matrix2cd a) {
	complex<double> tmp;
	tmp = a(0,0);
	a(0,0) = a(1,1);
	a(1,1) = tmp;
	tmp = a(0,1);
	a(0,1) = a(1,0);
	a(1,0) = tmp;
	return a;
}
}


#endif
