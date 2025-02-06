#include <opm/common/utility/numeric/GridUtil.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>
#include <tuple>
namespace Opm::GridUtil {


static std::vector<double> pillar_to_flat_array(const std::vector<std::array<std::array<double, 3>, 2>>& pillar)
{
    std::vector<double> flat_array;
    // Pre-allocate space for efficiency: 2 points per pillar entry, 3 components each
    flat_array.reserve(pillar.size() * 2 * 3);
    // Iterate through each pair of points in the pillar
    for (const auto& pair : pillar) {
        // Insert all components of the first point (x, y, z)
        flat_array.insert(flat_array.end(), pair[0].begin(), pair[0].end());
        // Insert all components of the second point (x, y, z)
        flat_array.insert(flat_array.end(), pair[1].begin(), pair[1].end());
    }
    return flat_array;
}


std::tuple<std::vector<double>,std::vector<double>> convertUnsToCPG(
                     const std::vector<std::array<double, 3>>& coord_uns, 
                     const std::vector<std::array<std::size_t, 8>>& element, 
                     std::size_t nx, std::size_t ny, std::size_t nz) 
{                        
    // nx, ny, nz are the number of cells in the x, y and z directions
    // converts unstructured mesh grid described by the coord_uns and element arrays
    // element contains a referece to the nodes described by coords_uns
    const std::size_t element_size = element.size();
    const std::size_t num_pillars = (nx+1)*(ny+1);
    GridDims cpg_grid = GridDims(nx, ny, nz);
    GridDims pillar_grid = GridDims(nx+1, ny+1, 0);
    


    auto ij_pillars = [ &pillar_grid](std::size_t i, std::size_t j) {
        return pillar_grid.getGlobalIndex(i, j, 0);
    };

    auto compute_zcornind = [&nx, &ny]
            (std::size_t  i, std::size_t j, std::size_t  k) 
    {
        std::array<std::size_t, 8> zind;
        const std::size_t num = k * nx * ny * 8 + j * nx * 4 + i * 2;
        const std::size_t offset_layer = 4 * nx * ny;
        zind[0] = num;
        zind[1] = num + 1;
        zind[2] = num + 2 * nx;
        zind[3] = zind[2] + 1;
        zind[4] = num + offset_layer;
        zind[5] = zind[1] + offset_layer;
        zind[6] = zind[2] + offset_layer;
        zind[7] = zind[3] + offset_layer;
        return zind;
    };    


    std::vector<std::array<std::array<double, 3>,2>> pillars(num_pillars);

    // looping through the first layer of elements
    for (std::size_t index = 0; index < nx*ny; index++) {
       auto [ii,jj,kk] = cpg_grid.getIJK((index));
       const std::array<std::size_t,8>& element_nodes = element[index];
       pillars[ij_pillars(ii,jj)][0] = coord_uns[element_nodes[0]];
       pillars[ij_pillars(ii+1,jj)][0] = coord_uns[element_nodes[1]];
       pillars[ij_pillars(ii,jj+1)][0] = coord_uns[element_nodes[2]];
       pillars[ij_pillars(ii+1,jj+1)][0] = coord_uns[element_nodes[3]];
    }
    // // looping through the last layer of elements
    for (std::size_t index = (element_size - nx*ny); index < element_size; index++) {
       auto [ii,jj,kk] = cpg_grid.getIJK((index));
       const std::array<std::size_t,8>& element_nodes = element[index];
       pillars[ij_pillars(ii,jj)][1] = coord_uns[element_nodes[4]];
       pillars[ij_pillars(ii+1,jj)][1] = coord_uns[element_nodes[5]];
       pillars[ij_pillars(ii,jj+1)][1] = coord_uns[element_nodes[6]];
       pillars[ij_pillars(ii+1,jj+1)][1] = coord_uns[element_nodes[7]];
    }
    std::vector<double> coord_cpg = pillar_to_flat_array(pillars);
    std::vector<double> zcorn_cpg(element_size*8);
    for (std::size_t index = 0; index < element_size; index++) {
       auto [ii,jj,kk] = cpg_grid.getIJK((index));
       std::array<std::size_t,8> z_ref = compute_zcornind(ii,jj,kk);       
       const std::array<std::size_t,8>& element_nodes = element[index];
       for (std::size_t index_el = 0; index_el < 8; index_el++) {
           std::size_t local_z = z_ref[index_el];
           std::size_t local_node = element_nodes[index_el];
           zcorn_cpg[local_z] = coord_uns[local_node][2];
       }
    }
    return std::make_tuple(coord_cpg,zcorn_cpg);
}


} // namespace GeometryUtils
