/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/io/hdf5/Hdf5Util.hpp>

#include <type_traits>
#include <stdexcept>
#include <string.h>

#include <iostream>

#include <hdf5_hl.h>


namespace Opm {   namespace Hdf5IO {

    void write_array_1d(hid_t file_id, const char* data_set_name, hid_t datatype_id, const void * data, size_t size,
                        bool unlimited, hsize_t chunk_size);

    template <typename T>
    void add_value_1d(hid_t dataset_id, hid_t datatype_id, T value);

    void  write_array_2d(hid_t file_id, const char* data_set_name, hid_t datatype_id,
                           const void * data, size_t nx, size_t ny, bool unlimited2, hsize_t chunk_dims[2]);

    template <typename T>
    T* make_data_array(const std::vector<std::vector<T>>& dataVect, size_t& nx, size_t& ny);

    void append_1d_to_2d(hid_t file_id, const char* data_set_name,  hid_t datatype, const void * data);

    void new_append_1d_to_2d(hid_t file_id, const char* data_set_name,  hid_t datatype, const void * data);

    H5T_class_t open_1d_dset(hid_t file_id, const char* name, hid_t& dataset_id, hid_t& memspace,
                             hid_t& dataspace, size_t& size, size_t& size_e);

    template <typename T>
    void open2d_dataset(hid_t file_id, const char* name, std::vector<std::vector<T>>& vectData, int size);

    template <typename T>
    void get_1d_from_2d(hid_t file_id, const char* name, int vInd, std::vector<T>& data_vect, int size);

    template <typename T>
    void set_value_1d(hid_t dataset_id, hid_t datatype_id, hid_t filespace_id, const char* data_set_name, size_t pos, T value);


} }  // namespace Opm::Hdf5IO



void  Opm::Hdf5IO::write_str_variable(hid_t file_id, const std::string& data_set_name, const std::string& variable)
{
    size_t length = variable.size();

    hid_t dataspace_id = H5Screate_simple(0, NULL, NULL);
    hid_t datatype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size (datatype_id, length);

    hid_t dataset_id = H5Dcreate2(file_id, data_set_name.c_str(), datatype_id, dataspace_id,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Dwrite(dataset_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, variable.c_str());
    H5Dflush (dataset_id);

    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
}

std::string Opm::Hdf5IO::read_str_variable(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id = H5Dopen2(file_id, data_set_name.c_str(), H5P_DEFAULT);
    hid_t datatype  = H5Dget_type(dataset_id);
    int str_length = H5Tget_size(datatype);

    char tmpstr[str_length + 1];

    H5Dread(dataset_id, datatype, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT, &tmpstr);

    tmpstr[str_length] = '\0';

    std::string  resStr =std::string(tmpstr);

    return resStr;
}


template <typename T>
void Opm::Hdf5IO::set_value_1d(hid_t dataset_id, hid_t datatype_id, hid_t filespace_id, const char* data_set_name, size_t pos, T value)
{
    hsize_t   dims[1];

    dims[0] = 1;

    hsize_t   offset[1];
    offset[0] = pos;

    H5Sselect_hyperslab (filespace_id, H5S_SELECT_SET, offset, NULL, dims, NULL);

    // Define memory space
    hid_t memspace = H5Screate_simple (1, dims, NULL);

    T data[1]  {value};

    H5Dwrite (dataset_id, datatype_id, memspace, filespace_id, H5P_DEFAULT, data );
    H5Dflush (dataset_id);

    H5Sclose(memspace);
}



template <>
void Opm::Hdf5IO::set_value_for_1d_hdf5(hid_t file_id, const std::string& data_set_name, size_t pos, int value)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name.c_str(), H5P_DEFAULT);
    hsize_t dims[1];
    hid_t filespace_id = H5Dget_space (dataset_id);

    H5Sget_simple_extent_dims(filespace_id, dims, NULL);

    if (pos >= dims[0] )
        throw std::invalid_argument("pos " + std::to_string(pos) + " is outside dataset bounds");

    Opm::Hdf5IO::set_value_1d(dataset_id, H5T_NATIVE_INT, filespace_id, data_set_name.c_str(), pos, value);

    H5Sclose(filespace_id);
    H5Dclose(dataset_id);
}


void  Opm::Hdf5IO::write_array_1d(hid_t file_id, const char* data_set_name, hid_t datatype_id,
                                  const void * data, size_t size, bool unlimited, hsize_t chunk_size)
{
    hsize_t dims[1];
    hid_t dataset_id, dataspace_id;

    if (unlimited){
        dims[0] = size;

        hsize_t chunk_dims[1] = { chunk_size };

        hsize_t maxdims[1] = {H5S_UNLIMITED};

        dataspace_id = H5Screate_simple (1, dims, maxdims);

        hid_t prop = H5Pcreate (H5P_DATASET_CREATE);
        H5Pset_chunk (prop, 1, chunk_dims);

        dataset_id = H5Dcreate2 (file_id, data_set_name, datatype_id, dataspace_id,
                         H5P_DEFAULT, prop, H5P_DEFAULT);

        H5Dwrite (dataset_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        H5Dflush (dataset_id);

    } else {
        dims[0] = size;

        dataspace_id = H5Screate_simple(1, dims, NULL);

        dataset_id = H5Dcreate2(file_id, data_set_name, datatype_id, dataspace_id,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        H5Dwrite(dataset_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        H5Dflush (dataset_id);
    }

    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
}




template <>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<int>& dataVect, bool unlimited, int chunk_size)
{
    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_INT, dataVect.data(), dataVect.size(), unlimited, chunk_size);
}

template <>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<float>& dataVect, bool unlimited, int chunk_size)
{
    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_FLOAT, dataVect.data(), dataVect.size(), unlimited, chunk_size);
}

template <>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<double>& dataVect, bool unlimited, int chunk_size)
{
    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_DOUBLE, dataVect.data(), dataVect.size(), unlimited, chunk_size);
}


template<>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<std::string>& dataVect, bool unlimited, int chunk_size)
{
    std::vector<const char*> arr_c_str;
    hid_t datatype_id;

    for (unsigned ii = 0; ii < dataVect.size(); ++ii)
        arr_c_str.push_back(dataVect[ii].c_str());

    datatype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size (datatype_id, H5T_VARIABLE);

    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), datatype_id, arr_c_str.data(), arr_c_str.size(), unlimited, chunk_size);
}


template <typename T>
void Opm::Hdf5IO::add_value_1d(hid_t dataset_id, hid_t datatype_id, T value)
{
    hsize_t  size[1];
    hsize_t dims[1];

    hid_t filespace = H5Dget_space (dataset_id);

    H5Sget_simple_extent_dims(filespace, dims, NULL);

    size[0] = dims[0] + 1;

    H5Dset_extent (dataset_id, size);
    filespace = H5Dget_space (dataset_id);

    H5Sget_simple_extent_dims(filespace, dims, NULL);

    hsize_t  dimsext[1];
    dimsext[0] = 1;

    hsize_t   offset[1];
    offset[0] = dims[0] - 1;

    H5Sselect_hyperslab (filespace, H5S_SELECT_SET, offset, NULL, dimsext, NULL);

    // Define memory space
    hid_t memspace = H5Screate_simple (1, dimsext, NULL);

    T data[1]  {value};

    H5Dwrite (dataset_id, datatype_id, memspace, filespace, H5P_DEFAULT, data );

    H5Dflush (dataset_id);
    H5Sclose(memspace);
    H5Sclose(filespace);
    H5Dclose(dataset_id);
}


template <>
void Opm::Hdf5IO::add_value_to_1d_hdf5(hid_t file_id, const std::string& data_set_name, float value)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name.c_str(), H5P_DEFAULT);
    Opm::Hdf5IO::add_value_1d(dataset_id, H5T_NATIVE_FLOAT, value);
}

template <>
void Opm::Hdf5IO::add_value_to_1d_hdf5(hid_t file_id, const std::string& data_set_name, double value)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name.c_str(), H5P_DEFAULT);
    Opm::Hdf5IO::add_value_1d(dataset_id, H5T_NATIVE_DOUBLE, value);
}

template <>
void Opm::Hdf5IO::add_value_to_1d_hdf5(hid_t file_id, const std::string& data_set_name, int value)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name.c_str(), H5P_DEFAULT);
    Opm::Hdf5IO::add_value_1d(dataset_id, H5T_NATIVE_INT, value);
}

template <>
void Opm::Hdf5IO::add_value_to_1d_hdf5(hid_t file_id, const std::string& data_set_name, std::string value)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name.c_str(), H5P_DEFAULT);

    hid_t datatype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size (datatype_id, H5T_VARIABLE);

    Opm::Hdf5IO::add_value_1d(dataset_id, datatype_id, value);
}



void Opm::Hdf5IO::append_1d_to_2d(hid_t file_id, const char* data_set_name,  hid_t datatype, const void * data)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name, H5P_DEFAULT);

    hsize_t size[2];
    hsize_t dims[2];

    hid_t filespace = H5Dget_space (dataset_id);

    H5Sget_simple_extent_dims(filespace, dims, NULL);

    size[0] = dims[0];
    size[1] = dims[1] + 1;

    H5Dset_extent (dataset_id, size);
    filespace = H5Dget_space (dataset_id);

    H5Sget_simple_extent_dims(filespace, dims, NULL);

    hsize_t  dimsext[2];
    dimsext[0] = dims[0];
    dimsext[1] = 1;

    hsize_t   offset[2];
    offset[0] = 0;
    offset[1] = dims[1] - 1;

    H5Sselect_hyperslab (filespace, H5S_SELECT_SET, offset, NULL, dimsext, NULL);

     // Define memory space
    hid_t memspace = H5Screate_simple (2, dimsext, NULL);

    H5Dwrite (dataset_id, datatype, memspace, filespace, H5P_DEFAULT, data );

    H5Dflush (dataset_id);

    H5Sclose(memspace);
    H5Sclose(filespace);
    H5Dclose(dataset_id);
}




template <>
void Opm::Hdf5IO::add_1d_to_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                    const std::vector<float>& vectData)
{
    Opm::Hdf5IO::append_1d_to_2d(file_id, data_set_name.c_str(), H5T_NATIVE_FLOAT, vectData.data());
}

template <>
void Opm::Hdf5IO::add_1d_to_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                    const std::vector<double>& vectData)
{
    Opm::Hdf5IO::append_1d_to_2d(file_id, data_set_name.c_str(), H5T_NATIVE_DOUBLE, vectData.data());
}

template <>
void Opm::Hdf5IO::add_1d_to_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                    const std::vector<int>& vectData)
{
    Opm::Hdf5IO::append_1d_to_2d(file_id, data_set_name.c_str(), H5T_NATIVE_INT, vectData.data());
}

template <>
void Opm::Hdf5IO::add_1d_to_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                    const std::vector<std::string>& vectData)
{
    char ** data = new char*[vectData.size()];

    for (size_t n=0; n < vectData.size(); n++){
        data[n] = new char[vectData[n].size() + 1];
        std::copy(vectData[n].begin(), vectData[n].end(), data[n]);
        data[n][vectData[n].size()] = '\0';
    }

    hid_t datatype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size (datatype_id, H5T_VARIABLE);

    Opm::Hdf5IO::append_1d_to_2d(file_id, data_set_name.c_str(), datatype_id, data);


    for (size_t n = 0; n < vectData.size(); n++)
        delete[] data[n];

    delete[] data;
}




template <>
void Opm::Hdf5IO::set_value_for_2d_hdf5(hid_t file_id, const std::string& data_set_name, size_t pos, const std::vector<float>& data)
{
    hid_t dataset_id = H5Dopen2 (file_id, data_set_name.c_str(), H5P_DEFAULT);

    hsize_t dims[2];

    hid_t filespace_id = H5Dget_space (dataset_id);

    H5Sget_simple_extent_dims(filespace_id, dims, NULL);

    if (data.size() != dims[0] )
        throw std::invalid_argument("size of input vector not equal to first dimension");

    if (pos >= dims[1] )
        throw std::invalid_argument("pos " + std::to_string(pos) + " is outside dataset bounds");


    hsize_t   dims2[2];
    hsize_t   offset[2];

    dims2[0] = data.size();
    dims2[1] = 1;

    offset[0] = 0;
    offset[1] = pos;

    H5Sselect_hyperslab (filespace_id, H5S_SELECT_SET, offset, NULL, dims2, NULL);

    // Define memory space
    hid_t memspace = H5Screate_simple (2, dims2, NULL);

    H5Dwrite (dataset_id, H5T_NATIVE_FLOAT, memspace, filespace_id, H5P_DEFAULT, data.data() );
    H5Dflush (dataset_id);

    H5Sclose(memspace);

    H5Sclose(filespace_id);
    H5Dclose(dataset_id);
}


void  Opm::Hdf5IO::write_array_2d(hid_t file_id, const char* data_set_name, hid_t datatype_id,
                                  const void * data, size_t nx, size_t ny, bool unlimited2, hsize_t chunk_dims[2])
{
    hid_t dataset_id, dataspace_id;

    hsize_t  dims[2] {nx, ny};

    if ((chunk_dims[0] > 0) && (chunk_dims[1] == 0) ||  (chunk_dims[1] > 0) && (chunk_dims[0] == 0))
        throw std::invalid_argument("invalied chunk size, both elements should be > 0");

    if ((unlimited2) && ((chunk_dims[0] == 0) || (chunk_dims[1] == 0)))
        throw std::invalid_argument("chunk size must be set when using H5S_UNLIMITED");

    if (unlimited2) {
        hsize_t maxdims[2] = {nx, H5S_UNLIMITED};
        dataspace_id = H5Screate_simple (2, dims, maxdims);
    } else {
        dataspace_id = H5Screate_simple (2, dims, NULL);
    }

    if (chunk_dims[0] > 0)
    {
        hid_t prop = H5Pcreate (H5P_DATASET_CREATE);
        H5Pset_chunk (prop, 2, chunk_dims);

        size_t elements = chunk_dims[0]* chunk_dims[1];

        size_t nbytes = elements * 4;
        //nbytes = static_cast<size_t>((float)nbytes * 1.2);

        hid_t dapl_id = H5Pcreate (H5P_DATASET_ACCESS);
        H5Pset_chunk_cache( dapl_id, elements, nbytes, H5D_CHUNK_CACHE_W0_DEFAULT );

        dataset_id = H5Dcreate2 (file_id, data_set_name, datatype_id, dataspace_id,
                         H5P_DEFAULT, prop, dapl_id);

    } else {

        dataset_id = H5Dcreate2(file_id, data_set_name, datatype_id, dataspace_id,
                                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }


    if (ny > 0) {
        H5Dwrite(dataset_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        H5Dflush (dataset_id);
    }

    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
}


template <typename T>
T* Opm::Hdf5IO::make_data_array(const std::vector<std::vector<T>>& dataVect, size_t& nx, size_t& ny)
{
    nx = dataVect.size();

    if (nx < 1)
        throw std::invalid_argument("size of first dimension ( = " + std::to_string(nx) + ") must be > 0");

    ny = dataVect[0].size();

    T * data = new T[nx*ny];

    for (size_t n = 0; n < nx; n ++)
        for (size_t m = 0; m < ny; m ++)
            data[ m + n *ny] = dataVect[n][m];

    return data;
}


template <>
void Opm::Hdf5IO::write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                const std::vector<std::vector<float>>& dataVect,
                                const bool unlimited2, std::array<int, 2> chunk_size)
{
    size_t nx = 0;
    size_t ny = 0;

    float * data = Opm::Hdf5IO::make_data_array(dataVect, nx, ny);

    hsize_t chunk_dims[2] = {chunk_size[0], chunk_size[1]};

    Opm::Hdf5IO::write_array_2d(file_id, data_set_name.c_str(), H5T_NATIVE_FLOAT,
                                  data, nx, ny, unlimited2, chunk_dims);

    delete[] data;
}

template <>
void Opm::Hdf5IO::write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                const std::vector<std::vector<double>>& dataVect,
                                const bool unlimited2, std::array<int, 2> chunk_size)
{
    size_t nx = 0; size_t ny = 0;

    double * data = Opm::Hdf5IO::make_data_array(dataVect, nx, ny);

    hsize_t chunk_dims[2] = {chunk_size[0], chunk_size[1]};

    Opm::Hdf5IO::write_array_2d(file_id, data_set_name.c_str(), H5T_NATIVE_DOUBLE,
                                  data, nx, ny, unlimited2, chunk_dims);

    delete[] data;
}

template <>
void Opm::Hdf5IO::write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                const std::vector<std::vector<int>>& dataVect,
                                const bool unlimited2, std::array<int, 2> chunk_size)
{
    size_t nx = 0; size_t ny = 0;

    int * data = Opm::Hdf5IO::make_data_array(dataVect, nx, ny);

    hsize_t chunk_dims[2] = {chunk_size[0], chunk_size[1]};

    Opm::Hdf5IO::write_array_2d(file_id, data_set_name.c_str(), H5T_NATIVE_INT,
                                  data, nx, ny, unlimited2, chunk_dims);

    delete[] data;
}

template <>
void Opm::Hdf5IO::write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                                const std::vector<std::vector<std::string>>& dataVect,
                                const bool unlimited2, std::array<int, 2> chunk_size)
{
    size_t  nx = dataVect.size();
    hid_t datatype_id;

    if (nx < 1)
        throw std::invalid_argument("size of first dimension ( = " + std::to_string(nx) + ") must be > 0");

    size_t ny = dataVect[0].size();

    std::vector<const char*> data;
    data.resize( nx*ny , "");

    for (size_t n = 0; n < nx; n ++)
        for (size_t m = 0; m < ny; m ++)
            data[ m + n *ny] = dataVect[n][m].c_str();

    datatype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size (datatype_id, H5T_VARIABLE);

    hsize_t chunk_dims[2] = {chunk_size[0], chunk_size[1]};

    Opm::Hdf5IO::write_array_2d(file_id, data_set_name.c_str(), datatype_id,
                                  data.data(), nx, ny, unlimited2, chunk_dims);
}


H5T_class_t Opm::Hdf5IO::open_1d_dset(hid_t file_id, const char* name, hid_t& dataset_id, hid_t& memspace, hid_t& dataspace,
                         size_t& size, size_t& size_e)
{
    // Turns automatic error printing on or off.
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    dataset_id = H5Dopen2(file_id, name, H5P_DEFAULT);

    if( dataset_id < 0)
        throw std::invalid_argument("dataset not found in file");

    hid_t datatype  = H5Dget_type(dataset_id);

    H5T_class_t t_class   = H5Tget_class(datatype);

    size_e = H5Tget_size (datatype);

    H5Tclose(datatype);

    dataspace = H5Dget_space(dataset_id);

    int rank  = H5Sget_simple_extent_ndims(dataspace);

    if (rank != 1)
        throw std::invalid_argument("dataset found, but this is not of 1D");

    hsize_t dims[rank];

    H5Sget_simple_extent_dims(dataspace, dims, NULL);
    size = dims[0];

    memspace = H5Screate_simple(rank, dims, NULL);

    return t_class;
}


template<>
std::vector<int> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if ((t_class != H5T_INTEGER) || (size_e != 4))
        throw std::invalid_argument("dataset found, but this has wrong data type");

    int data_out[size]; //

    H5Dread(dataset_id, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    std::vector<int> data_vect(data_out, data_out + size);

    return data_vect;
}

template<>
std::vector<float> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);


    if ((t_class != H5T_FLOAT) || (size_e != 4))
        throw std::invalid_argument("dataset found, but this has wrong data type");

    float data_out[size]; //

    H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    std::vector<float> data_vect(data_out, data_out + size);

    return data_vect;
}

template<>
std::vector<double> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if ((t_class != H5T_FLOAT) || (size_e != 8))
        throw std::invalid_argument("dataset found, but this has wrong data type");

    double data_out[size]; //

    H5Dread(dataset_id, H5T_NATIVE_DOUBLE, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    std::vector<double> data_vect(data_out, data_out + size);

    return data_vect;
}

template<>
std::vector<std::string> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if (t_class != H5T_STRING)
        throw std::invalid_argument("dataset found, but this has wrong data type");

    std::vector<const char*> tmpvect( size, NULL );

    hid_t datatype  = H5Dget_type(dataset_id);
    H5Dread(dataset_id, datatype, memspace, dataspace, H5P_DEFAULT, tmpvect.data());

    std::vector<std::string> data_vect;
    data_vect.reserve(size);

    for(size_t n = 0; n < size; n++)
        data_vect.push_back(tmpvect[n]);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    return data_vect;
}

template <typename T>
void Opm::Hdf5IO::open2d_dataset(hid_t file_id, const char* name, std::vector<std::vector<T>>& vectData, int size)
{
    hid_t dataset_id, dataspace_id, datatype_id;

    dataset_id = H5Dopen2(file_id, name, H5P_DEFAULT);
    dataspace_id = H5Dget_space(dataset_id);
    datatype_id = H5Dget_type(dataset_id);

    if (H5Sget_simple_extent_ndims(dataspace_id) != 2)
        throw std::invalid_argument("this dataset is not a 2d array");

    hsize_t dims[2];
    H5Sget_simple_extent_dims(dataspace_id, dims, NULL);

    size_t length;

    if (size < 0)
        length = dims[1];
    else
        length = size;


    T * data = new T[dims[0] * dims[1]];

    vectData.resize(dims[0], {});

    for (size_t n = 0; n < dims[0]; n++)
        vectData[n].resize(length);

    H5Dread(dataset_id, datatype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT, data);

    for (size_t n = 0; n < dims[0]; n ++)
        for (size_t m = 0; m < length; m ++)
            vectData[n][m] = data[ m + n* dims[1]];

    delete[] data;
}


template<>
std::vector<std::vector<float>> Opm::Hdf5IO::get_2d_hdf5(hid_t file_id, const std::string& data_set_name, int size)
{
    std::vector<std::vector<float>> vectData;
    Opm::Hdf5IO::open2d_dataset(file_id, data_set_name.c_str(), vectData, size);

    return vectData;
}

template<>
std::vector<std::vector<double>> Opm::Hdf5IO::get_2d_hdf5(hid_t file_id, const std::string& data_set_name, int size)
{
    std::vector<std::vector<double>> vectData;
    Opm::Hdf5IO::open2d_dataset(file_id, data_set_name.c_str(), vectData, size );

    return vectData;
}

template<>
std::vector<std::vector<int>> Opm::Hdf5IO::get_2d_hdf5(hid_t file_id, const std::string& data_set_name, int size)
{
    std::vector<std::vector<int>> vectData;
    Opm::Hdf5IO::open2d_dataset(file_id, data_set_name.c_str(), vectData, size );

    return vectData;
}

template<>
std::vector<std::vector<std::string>> Opm::Hdf5IO::get_2d_hdf5(hid_t file_id, const std::string& data_set_name, int size)
{
    std::vector<std::vector<std::string>> vectData;

    hid_t dataset_id, dataspace_id, datatype_id;

    dataset_id = H5Dopen2(file_id, data_set_name.c_str(), H5P_DEFAULT);

    dataspace_id = H5Dget_space(dataset_id);

    if (H5Sget_simple_extent_ndims(dataspace_id) != 2)
        throw std::invalid_argument("this dataset is not a 2d array");

    hsize_t dims[2];
    H5Sget_simple_extent_dims(dataspace_id, dims, NULL);

    vectData.resize(dims[0], {});

    for (size_t n = 0; n < dims[0]; n++)
        vectData[n].resize(dims[1]);

    std::vector<const char*> tmpvect( dims[0] * dims[1], NULL );

    datatype_id  = H5Dget_type(dataset_id);
    H5Dread(dataset_id, datatype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT, tmpvect.data());

    for (size_t n = 0; n < dims[0]; n ++)
        for (size_t m = 0; m < dims[1]; m ++)
            vectData[n][m] = tmpvect[ m + n* dims[1]];

    return vectData;
}

template <typename T>
void Opm::Hdf5IO::get_1d_from_2d(hid_t file_id, const char* name, int vInd, std::vector<T>& data_vect, int size)
{
    hid_t dataset_id, dataspace, datatype_id;

    dataset_id = H5Dopen2(file_id, name, H5P_DEFAULT);
    dataspace = H5Dget_space(dataset_id);

    int rank  = H5Sget_simple_extent_ndims(dataspace);

    hsize_t dims[rank];

    if (rank != 2)
        throw std::invalid_argument("dimension of dataset should be 2");

    H5Sget_simple_extent_dims(dataspace, dims, NULL);

    //  Define hyperslab in the dataset;

    hsize_t  offset[2];   // hyperslab offset in the file
    hsize_t  count[2];    // size of the hyperslab in the file

    offset[0] = vInd;
    offset[1] = 0;

    count[0]  = 1;
    size_t length;

    if (size < 0)
        length  = dims[1];
    else
        length  = size;

    count[1] = length;

    //hsize_t  dims2[2]  = {1, dims[1]};
    hsize_t  dims2[2]  = {1, length};

    hid_t memspace = H5Screate_simple(2, dims2, NULL);

    H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);

    //T data_out[dims[1]]; //
    T data_out[length]; //

    datatype_id = H5Dget_type(dataset_id);

    //H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace, dataspace, H5P_DEFAULT, data_out);
    H5Dread(dataset_id, datatype_id, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Drefresh(dataset_id);


    //data_vect.reserve(dims[1]);
    data_vect.reserve(length);

    //for (size_t n=0; n < dims[1]; n++)
    for (size_t n=0; n < length; n++)
        data_vect.push_back(data_out[n]);

    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset_id);
}


template<>
std::vector<float> Opm::Hdf5IO::get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd, int size)
{
    std::vector<float> data_vect;
    Opm::Hdf5IO::get_1d_from_2d(file_id, data_set_name.c_str(), vInd, data_vect, size);

    return data_vect;
}

template<>
std::vector<double> Opm::Hdf5IO::get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd, int size)
{
    std::vector<double> data_vect;
    Opm::Hdf5IO::get_1d_from_2d(file_id, data_set_name.c_str(), vInd, data_vect, size);

    return data_vect;
}

template<>
std::vector<int> Opm::Hdf5IO::get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd, int size)
{
    std::vector<int> data_vect;
    Opm::Hdf5IO::get_1d_from_2d(file_id, data_set_name.c_str(), vInd, data_vect, size);

    return data_vect;
}

