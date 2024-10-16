
#ifndef OPM_CO2_TABLES_STRUCTS_HPP
#define OPM_CO2_TABLES_STRUCTS_HPP

#include <vector>
struct TabulatedDensityTraits {
	typedef double Scalar;
	const char  *name;
	const int    numX;
	const Scalar xMin;
	const Scalar xMax;
	const int    numY;
	const Scalar yMin;
	const Scalar yMax;
	const std::vector<std::vector<Scalar>> vals;
    TabulatedDensityTraits();
};

struct TabulatedEnthalpyTraits {
	typedef double Scalar;
	const char  *name;
	const int    numX;
	const Scalar xMin;
	const Scalar xMax;
	const int    numY;
	const Scalar yMin;
	const Scalar yMax;
	const std::vector<std::vector<Scalar>> vals;
    TabulatedEnthalpyTraits();
};
#endif // OPM_CO2_TABLES_STRUCTS_HPP
