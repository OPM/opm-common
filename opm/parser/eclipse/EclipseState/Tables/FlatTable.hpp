#ifndef OPM_FLAT_TABLE_HPP
#define OPM_FLAT_TABLE_HPP

namespace Opm {

class DeckKeyword;

template< typename T >
struct FlatTable : public std::vector< T > {
    FlatTable() = default;
    explicit FlatTable( const DeckKeyword& );
};

struct DENSITYRecord {
    static constexpr std::size_t size = 3;

    double oil;
    double water;
    double gas;
};

struct DensityTable : public FlatTable< DENSITYRecord > {
    using FlatTable< DENSITYRecord >::FlatTable;
};

struct PVTWRecord {
    static constexpr std::size_t size = 5;

    double reference_pressure;
    double volume_factor;
    double compressibility;
    double viscosity;
    double viscosibility;
};

struct PvtwTable : public FlatTable< PVTWRecord > {
    using FlatTable< PVTWRecord >::FlatTable;
};

struct ROCKRecord {
    static constexpr std::size_t size = 2;

    double reference_pressure;
    double compressibility;
};

struct RockTable : public FlatTable< ROCKRecord > {
    using FlatTable< ROCKRecord >::FlatTable;
};

struct PVCDORecord {
    static constexpr std::size_t size = 5;

    double reference_pressure;
    double volume_factor;
    double compressibility;
    double viscosity;
    double viscosibility;
};

struct PvcdoTable : public FlatTable< PVCDORecord > {
    using FlatTable< PVCDORecord >::FlatTable;
};

struct VISCREFRecord {
    static constexpr std::size_t size = 2;

    double reference_pressure;
    double reference_rs;
};

struct ViscrefTable : public FlatTable< VISCREFRecord > {
    using FlatTable< VISCREFRecord >::FlatTable;
};

struct WATDENTRecord {
    static constexpr std::size_t size = 3;

    double reference_temperature;
    double first_coefficient;
    double second_coefficient;
};

struct WatdentTable : public FlatTable< WATDENTRecord > {
    using FlatTable< WATDENTRecord >::FlatTable;
};

}

#endif //OPM_FLAT_TABLE_HPP
