#include <vector>
#include <array>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

template<int N> array<complex<double>, N> operator*(const array<complex<double>, N>& a, double b) {
	array<complex<double>, N> res;
	for(int i=0;i<N;i++)
		res[i] = a[i]*b;
	return res;
}

// solve for the nodes in a signal flow graph, given excitation applied to each node
VectorXcd solveSFG(MatrixXcd sfg, VectorXcd excitation) {
	assert(sfg.rows() == sfg.cols());
	assert(sfg.rows() == excitation.size());
	
	VectorXcd rhs = -excitation;
	MatrixXcd A = sfg - MatrixXcd::Identity(sfg.rows(), sfg.rows());
	return A.colPivHouseholderQr().solve(rhs);
}
// solve for the nodes in a signal flow graph, but allow floating input nodes (allowed to take on any value)
// and allow for forced nodes (nodes that are constrained to a value, but still obeying edge equations)
// for example this can be used to solve for the inputs to a signal flow graph given the outputs
// note that the number of floating (free) nodes must equal the number of forced nodes for the system to
// have a unique solution
VectorXcd solveSFGImplicit(MatrixXcd sfg, VectorXcd excitation,
							vector<int> freeNodes, vector<int> forcedNodes, vector<complex<double> > forcedNodeValues) {
	assert(freeNodes.size() == forcedNodes.size());
	assert(forcedNodes.size() == forcedNodeValues.size());
	
	for(int i=0;i<(int)freeNodes.size();i++) {
		// create a self-loop of gain 1 on the free node
		// the self-loop allows the free node to take on any value with no incoming edge
		sfg(freeNodes[i], freeNodes[i]) = 1.;
		
		// create an edge from the forced node to the free node with value -1
		sfg(freeNodes[i], forcedNodes[i]) = -1.;
		// add an excitation on the free node with the value of the forced node value
		excitation(forcedNodes[i]) = forcedNodeValues[i];
		
		// the self-loop of gain 1 on the free node enforces all the incoming edges must sum to 0,
		// because otherwise the feedback will cause the value to blow up;
		// the effect of this constraint is that the forced node must in the end have a value
		// equal to the forced value
	}
	return solveSFG(sfg, excitation);
}


