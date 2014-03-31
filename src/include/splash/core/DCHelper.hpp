/**
 * Copyright 2013 Felix Schmitt
 *
 * This file is part of libSplash. 
 * 
 * libSplash is free software: you can redistribute it and/or modify 
 * it under the terms of of either the GNU General Public License or 
 * the GNU Lesser General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version. 
 * libSplash is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License and the GNU Lesser General Public License 
 * for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * and the GNU Lesser General Public License along with libSplash. 
 * If not, see <http://www.gnu.org/licenses/>. 
 */



#ifndef DCHELPER_H
#define	DCHELPER_H

#include <vector>
#include <cmath>
#include <sstream>
#include <iostream>
#include <hdf5.h>

namespace splash
{

    /**
     * Provides static help functions for DataCollectors (e.g. SerialDataCollector).
     * \cond HIDDEN_SYMBOLS
     */
    class DCHelper
    {
    private:
        DCHelper();
    public:

        static void printhsizet(const char *name, const hsize_t *data, hsize_t rank)
        {
            std::cerr << name << " = (";
            for (hsize_t i = 0; i < rank - 1; i++)
                std::cerr << data[i] << ", ";
            std::cerr << data[rank - 1];
            std::cerr << ")" << std::endl;

        }

        /**
         * swaps dimensions of hsize_t vector depending on rank
         * @param hs the hsize_t vector
         * @param rank dimensions used in hs
         */
        static void swapHSize(hsize_t *hs, uint32_t rank)
        {
            if (hs == NULL)
                return;

            hsize_t tmp1;
            hsize_t tmp3[3];

            switch (rank)
            {
                case 2:
                    tmp1 = hs[0];
                    hs[0] = hs[1];
                    hs[1] = tmp1;
                    break;
                case 3:
                    tmp3[0] = hs[2];
                    tmp3[1] = hs[1];
                    tmp3[2] = hs[0];

                    for (uint32_t i = 0; i < 3; i++)
                        hs[i] = tmp3[i];
                    break;
                default:
                    return;
            }
        }

        /**
         * @param dims dimensions to get chunk dims for
         * @param ndims number of dimensions for dims and chunkDims
         * @param typeSize size of each element in bytes
         * @param chunkDims pointer to array for resulting chunk dimensions
         */
        static void getOptimalChunkDims(const hsize_t *dims, uint32_t ndims,
                size_t typeSize, hsize_t *chunkDims)
        {
            const size_t NUM_CHUNK_SIZES = 7;
            const size_t CHUNK_SIZES[] = {4096, 2048, 1024, 512, 256, 128, 64};
            
            size_t total_data_size = typeSize;
            size_t max_chunk_size = typeSize;
            size_t target_chunk_size = 0;

            // compute the order of dimensions (descending)
            // large dataset dimensions should have larger chunk sizes
            std::vector<uint32_t> dims_order;
            dims_order.reserve(ndims);
            dims_order.push_back(0);
            for (uint32_t i = 1; i < ndims; ++i)
            {
                std::vector<uint32_t>::iterator iter;
                for (iter = dims_order.begin(); iter != dims_order.end(); ++iter)
                {
                    if (dims[*iter] < dims[i])
                    {
                        iter = dims_order.insert(iter, i);
                        break;
                    }
                }

                if (iter == dims_order.end())
                    dims_order.push_back(i);
            }
            
            for (uint32_t i = 0; i < ndims; ++i)
            {
                // initial number of chunks per dimension
                chunkDims[i] = 1;
                
                // try to make at least two chunks for each dimension
                size_t half_dim = dims[i] / 2;
                
                // compute sizes
                max_chunk_size *= (half_dim > 0) ? half_dim : 1;
                total_data_size *= dims[i];
            }
            
            // compute the target chunk size
            for (uint32_t i = 0; i < NUM_CHUNK_SIZES; ++i)
            {
                target_chunk_size = CHUNK_SIZES[i] * 1024;
                if (target_chunk_size <= max_chunk_size)
                    break;
            }
            
            size_t current_chunk_size = typeSize;
            size_t last_chunk_diff = target_chunk_size;
            int current_index = 0;

            while (current_chunk_size < target_chunk_size)
            {
                // test if increasing chunk size optimizes towards target chunk size
                size_t chunk_diff = std::abs(target_chunk_size - (current_chunk_size * 2));
                if (chunk_diff >= last_chunk_diff)
                    break;

                // find next dimension to increase chunk size for
                int can_increase_dim = 0;
                for (uint32_t d = 0; d < ndims; ++d)
                {
                    int current_dim = dims_order[current_index];
                    
                    // increasing chunk size possible
                    if (chunkDims[current_dim] * 2 <= dims[current_dim])
                    {
                        chunkDims[current_dim] *= 2;
                        current_chunk_size *= 2;
                        can_increase_dim = 1;
                    }

                    current_index = (current_index + 1) % ndims;

                    if (can_increase_dim)
                        break;
                }

                // can not increase chunk size in any dimension
                // we must use the current chunk sizes
                if (!can_increase_dim)
                    break;

                last_chunk_diff = chunk_diff;
            }
        }

        /**
         * tests filename for common mistakes when using this library and prints a warning
         * @param filename the filename to test
         * @result true if no errors occurred, false otherwise
         */
        static bool testFilename(const std::string& filename)
        {
            if (filename.rfind(".h5.h5") == filename.length() - 6)
            {
                std::cerr << std::endl << "\tWarning: DCHelper: Do you really want to access "
                        << filename.c_str() << "?" << std::endl;
                return false;
            }

            return true;
        }

    };
    /**
     * \endcond
     */

} // namespace DataCollector

#endif	/* DCHELPER_H */
