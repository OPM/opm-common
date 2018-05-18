#include <opm/output/eclipse/LogiHEAD.hpp>

#include <utility>
#include <vector>

enum index : std::vector<int>::size_type {
lh_000	=	0	,		//	TRUE
lh_001	=	1	,		//	TRUE
lh_002	=	2	,		//	FALSE
lh_003	=	3	,		//	FALSE		Flag set to FALSE for a non-radial model, TRUE for a radial model (ECLIPSE 300 and other simulators)
lh_004	=	4	,		//	FALSE		Flag set to FALSE for a non-radial model, TRUE for a radial model (ECLIPSE 100)
lh_005	=	5	,		//	FALSE
lh_006	=	6	,		//	FALSE
lh_007	=	7	,		//	FALSE
lh_008	=	8	,		//	FALSE
lh_009	=	9	,		//	FALSE
lh_010	=	10	,		//	FALSE
lh_011	=	11	,		//	FALSE
lh_012	=	12	,		//	FALSE
lh_013	=	13	,		//	FALSE
lh_014	=	14	,		//	FALSE		Flag for dual porosity model
lh_015	=	15	,		//	FALSE
lh_016	=	16	,		//	TRUE
lh_017	=	17	,		//	FALSE
lh_018	=	18	,		//	TRUE
lh_019	=	19	,		//
lh_020	=	20	,		//	FALSE
lh_021	=	21	,		//	FALSE
lh_022	=	22	,		//	FALSE
lh_023	=	23	,		//	FALSE
lh_024	=	24	,		//	FALSE
lh_025	=	25	,		//	FALSE
lh_026	=	26	,		//	FALSE
lh_027	=	27	,		//	FALSE
lh_028	=	28	,		//	FALSE
lh_029	=	29	,		//	FALSE
lh_030	=	30	,		//	FALSE		Flag for coalbed methane (ECLIPSE 100)
lh_031	=	31	,		//	TRUE
lh_032	=	32	,		//	FALSE
lh_033	=	33	,		//	FALSE
lh_034	=	34	,		//	FALSE
lh_035	=	35	,		//	FALSE
lh_036	=	36	,		//	FALSE
lh_037	=	37	,		//
lh_038	=	38	,		//	FALSE
lh_039	=	39	,		//	FALSE
lh_040	=	40	,		//	FALSE
lh_041	=	41	,		//	FALSE
lh_042	=	42	,		//	FALSE
lh_043	=	43	,		//
lh_044	=	44	,		//	TRUE
lh_045	=	45	,		//	FALSE
lh_046	=	46	,		//	FALSE
lh_047	=	47	,		//	FALSE
lh_048	=	48	,		//
lh_049	=	49	,		//	FALSE
lh_050	=	50	,		//	FALSE
lh_051	=	51	,		//	FALSE
lh_052	=	52	,		//	FALSE
lh_053	=	53	,		//	FALSE
lh_054	=	54	,		//	FALSE
lh_055	=	55	,		//	FALSE
lh_056	=	56	,		//	FALSE
lh_057	=	57	,		//	FALSE
lh_058	=	58	,		//	FALSE
lh_059	=	59	,		//	FALSE
lh_060	=	60	,		//	FALSE
lh_061	=	61	,		//	FALSE
lh_062	=	62	,		//	FALSE
lh_063	=	63	,		//	FALSE
lh_064	=	64	,		//	FALSE
lh_065	=	65	,		//	FALSE
lh_066	=	66	,		//	FALSE
lh_067	=	67	,		//	FALSE
lh_068	=	68	,		//	FALSE
lh_069	=	69	,		//	FALSE
lh_070	=	70	,		//	FALSE
lh_071	=	71	,		//	FALSE
lh_072	=	72	,		//	FALSE
lh_073	=	73	,		//	FALSE
lh_074	=	74	,		//	FALSE
lh_075	=	75	,		//	TRUE	If segmented well model is used
lh_076	=	76	,		//	TRUE
lh_077	=	77	,		//	FALSE
lh_078	=	78	,		//	FALSE
lh_079	=	79	,		//	FALSE
lh_080	=	80	,		//	FALSE
lh_081	=	81	,		//	FALSE
lh_082	=	82	,		//	FALSE
lh_083	=	83	,		//	FALSE
lh_084	=	84	,		//	FALSE
lh_085	=	85	,		//	FALSE
lh_086	=	86	,		//
lh_087	=	87	,		//	TRUE
lh_088	=	88	,		//	FALSE
lh_089	=	89	,		//	FALSE
lh_090	=	90	,		//	FALSE
lh_091	=	91	,		//	FALSE
lh_092	=	92	,		//	FALSE
lh_093	=	93	,		//
lh_094	=	94	,		//	FALSE
lh_095	=	95	,		//	FALSE
lh_096	=	96	,		//
lh_097	=	97	,		//	FALSE
lh_098	=	98	,		//	FALSE
lh_099	=	99	,		//	TRUE
lh_100	=	100	,		//	FALSE
lh_101	=	101	,		//	FALSE
lh_102	=	102	,		//	FALSE
lh_103	=	103	,		//	FALSE
lh_104	=	104	,		//	FALSE
lh_105	=	105	,		//	FALSE
lh_106	=	106	,		//	FALSE
lh_107	=	107	,		//	FALSE
lh_108	=	108	,		//	FALSE
lh_109	=	109	,		//	FALSE
lh_110	=	110	,		//	FALSE
lh_111	=	111	,		//	FALSE
lh_112	=	112	,		//	FALSE
lh_113	=	113	,		//	TRUE
lh_114	=	114	,		//	TRUE
lh_115	=	115	,		//	TRUE
lh_116	=	116	,		//	FALSE
lh_117	=	117	,		//	TRUE
lh_118	=	118	,		//	FALSE
lh_119	=	119	,		//	FALSE
lh_120	=	120	,		//	FALSE

  // ---------------------------------------------------------------------
  // ---------------------------------------------------------------------

  LOGIHEAD_NUMBER_OF_ITEMS        // MUST be last element of enum.
};

// =====================================================================
// Public Interface Below Separator
// =====================================================================

Opm::RestartIO::LogiHEAD::LogiHEAD()
    : data_(LOGIHEAD_NUMBER_OF_ITEMS, false)
{}

Opm::RestartIO::LogiHEAD&
Opm::RestartIO::LogiHEAD::
variousParam(const bool e300_radial, const bool e100_radial, const int nswlmx)
{
    this -> data_[lh_000] = true;
    this -> data_[lh_001] = true;
    this -> data_[lh_003] = e300_radial;
    this -> data_[lh_004] = e100_radial;
    //this -> data_[lh_016] = true;
    //this -> data_[lh_018] = true;
    //this -> data_[lh_031] = true;
    //this -> data_[lh_044] = true;
    this -> data_[lh_075] = nswlmx >= 1; // True if MS Wells exist.
    //this -> data_[lh_076] = true;
    //this -> data_[lh_087] = true;
    //this -> data_[lh_099] = true;
    //this -> data_[lh_113] = true;
    //this -> data_[lh_114] = true;
    //this -> data_[lh_115] = true;
    //this -> data_[lh_117] = true;

    return *this;
}
