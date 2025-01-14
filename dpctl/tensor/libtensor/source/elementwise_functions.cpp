//===----------- Implementation of _tensor_impl module  ---------*-C++-*-/===//
//
//                      Data Parallel Control (dpctl)
//
// Copyright 2020-2023 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines functions of dpctl.tensor._tensor_impl extensions,
/// specifically functions for elementwise operations.
//===----------------------------------------------------------------------===//

#include "dpctl4pybind11.hpp"
#include <CL/sycl.hpp>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "elementwise_functions.hpp"
#include "utils/type_dispatch.hpp"

#include "kernels/elementwise_functions/abs.hpp"
#include "kernels/elementwise_functions/add.hpp"
#include "kernels/elementwise_functions/conj.hpp"
#include "kernels/elementwise_functions/cos.hpp"
#include "kernels/elementwise_functions/equal.hpp"
#include "kernels/elementwise_functions/exp.hpp"
#include "kernels/elementwise_functions/expm1.hpp"
#include "kernels/elementwise_functions/floor_divide.hpp"
#include "kernels/elementwise_functions/greater.hpp"
#include "kernels/elementwise_functions/greater_equal.hpp"
#include "kernels/elementwise_functions/imag.hpp"
#include "kernels/elementwise_functions/isfinite.hpp"
#include "kernels/elementwise_functions/isinf.hpp"
#include "kernels/elementwise_functions/isnan.hpp"
#include "kernels/elementwise_functions/less.hpp"
#include "kernels/elementwise_functions/less_equal.hpp"
#include "kernels/elementwise_functions/log.hpp"
#include "kernels/elementwise_functions/log10.hpp"
#include "kernels/elementwise_functions/log1p.hpp"
#include "kernels/elementwise_functions/log2.hpp"
#include "kernels/elementwise_functions/logical_and.hpp"
#include "kernels/elementwise_functions/logical_not.hpp"
#include "kernels/elementwise_functions/logical_or.hpp"
#include "kernels/elementwise_functions/logical_xor.hpp"
#include "kernels/elementwise_functions/multiply.hpp"
#include "kernels/elementwise_functions/negative.hpp"
#include "kernels/elementwise_functions/not_equal.hpp"
#include "kernels/elementwise_functions/positive.hpp"
#include "kernels/elementwise_functions/pow.hpp"
#include "kernels/elementwise_functions/proj.hpp"
#include "kernels/elementwise_functions/real.hpp"
#include "kernels/elementwise_functions/sin.hpp"
#include "kernels/elementwise_functions/sqrt.hpp"
#include "kernels/elementwise_functions/square.hpp"
#include "kernels/elementwise_functions/subtract.hpp"
#include "kernels/elementwise_functions/true_divide.hpp"

namespace dpctl
{
namespace tensor
{
namespace py_internal
{

namespace td_ns = dpctl::tensor::type_dispatch;

py::dtype _dtype_from_typenum(td_ns::typenum_t dst_typenum_t)
{
    switch (dst_typenum_t) {
    case td_ns::typenum_t::BOOL:
        return py::dtype("?");
    case td_ns::typenum_t::INT8:
        return py::dtype("i1");
    case td_ns::typenum_t::UINT8:
        return py::dtype("u1");
    case td_ns::typenum_t::INT16:
        return py::dtype("i2");
    case td_ns::typenum_t::UINT16:
        return py::dtype("u2");
    case td_ns::typenum_t::INT32:
        return py::dtype("i4");
    case td_ns::typenum_t::UINT32:
        return py::dtype("u4");
    case td_ns::typenum_t::INT64:
        return py::dtype("i8");
    case td_ns::typenum_t::UINT64:
        return py::dtype("u8");
    case td_ns::typenum_t::HALF:
        return py::dtype("f2");
    case td_ns::typenum_t::FLOAT:
        return py::dtype("f4");
    case td_ns::typenum_t::DOUBLE:
        return py::dtype("f8");
    case td_ns::typenum_t::CFLOAT:
        return py::dtype("c8");
    case td_ns::typenum_t::CDOUBLE:
        return py::dtype("c16");
    default:
        throw py::value_error("Unrecognized dst_typeid");
    }
}

int _result_typeid(int arg_typeid, const int *fn_output_id)
{
    if (arg_typeid < 0 || arg_typeid >= td_ns::num_types) {
        throw py::value_error("Input typeid " + std::to_string(arg_typeid) +
                              " is outside of expected bounds.");
    }

    return fn_output_id[arg_typeid];
}

namespace ew_cmn_ns = dpctl::tensor::kernels::elementwise_common;
using ew_cmn_ns::binary_contig_impl_fn_ptr_t;
using ew_cmn_ns::binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t;
using ew_cmn_ns::binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t;
using ew_cmn_ns::binary_strided_impl_fn_ptr_t;
using ew_cmn_ns::unary_contig_impl_fn_ptr_t;
using ew_cmn_ns::unary_strided_impl_fn_ptr_t;

using ew_cmn_ns::binary_inplace_contig_impl_fn_ptr_t;
using ew_cmn_ns::binary_inplace_row_matrix_broadcast_impl_fn_ptr_t;
using ew_cmn_ns::binary_inplace_strided_impl_fn_ptr_t;

// U01: ==== ABS   (x)
namespace impl
{

namespace abs_fn_ns = dpctl::tensor::kernels::abs;

static unary_contig_impl_fn_ptr_t abs_contig_dispatch_vector[td_ns::num_types];
static int abs_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    abs_strided_dispatch_vector[td_ns::num_types];

void populate_abs_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = abs_fn_ns;

    using fn_ns::AbsContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, AbsContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(abs_contig_dispatch_vector);

    using fn_ns::AbsStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, AbsStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(abs_strided_dispatch_vector);

    using fn_ns::AbsTypeMapFactory;
    DispatchVectorBuilder<int, AbsTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(abs_output_typeid_vector);
};

} // namespace impl

// U02: ==== ACOS   (x)
namespace impl
{
// FIXME: add code for U02
} // namespace impl

// U03: ===== ACOSH (x)
namespace impl
{
// FIXME: add code for U03
} // namespace impl

// B01: ===== ADD   (x1, x2)
namespace impl
{
namespace add_fn_ns = dpctl::tensor::kernels::add;

static binary_contig_impl_fn_ptr_t add_contig_dispatch_table[td_ns::num_types]
                                                            [td_ns::num_types];
static int add_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    add_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

// add(matrix, row)
static binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t
    add_contig_matrix_contig_row_broadcast_dispatch_table[td_ns::num_types]
                                                         [td_ns::num_types];

// add(row, matrix)
static binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t
    add_contig_row_contig_matrix_broadcast_dispatch_table[td_ns::num_types]
                                                         [td_ns::num_types];

static binary_inplace_contig_impl_fn_ptr_t
    add_inplace_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static binary_inplace_strided_impl_fn_ptr_t
    add_inplace_strided_dispatch_table[td_ns::num_types][td_ns::num_types];
static binary_inplace_row_matrix_broadcast_impl_fn_ptr_t
    add_inplace_row_matrix_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_add_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = add_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::AddTypeMapFactory;
    DispatchTableBuilder<int, AddTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(add_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::AddStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, AddStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(add_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::AddContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, AddContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(add_contig_dispatch_table);

    // function pointers for operation on contiguous matrix, contiguous row
    // with contiguous matrix output
    using fn_ns::AddContigMatrixContigRowBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t,
        AddContigMatrixContigRowBroadcastFactory, num_types>
        dtb4;
    dtb4.populate_dispatch_table(
        add_contig_matrix_contig_row_broadcast_dispatch_table);

    // function pointers for operation on contiguous row, contiguous matrix
    // with contiguous matrix output
    using fn_ns::AddContigRowContigMatrixBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t,
        AddContigRowContigMatrixBroadcastFactory, num_types>
        dtb5;
    dtb5.populate_dispatch_table(
        add_contig_row_contig_matrix_broadcast_dispatch_table);

    // function pointers for inplace operation on general strided arrays
    using fn_ns::AddInplaceStridedFactory;
    DispatchTableBuilder<binary_inplace_strided_impl_fn_ptr_t,
                         AddInplaceStridedFactory, num_types>
        dtb6;
    dtb6.populate_dispatch_table(add_inplace_strided_dispatch_table);

    // function pointers for inplace operation on contiguous inputs and output
    using fn_ns::AddInplaceContigFactory;
    DispatchTableBuilder<binary_inplace_contig_impl_fn_ptr_t,
                         AddInplaceContigFactory, num_types>
        dtb7;
    dtb7.populate_dispatch_table(add_inplace_contig_dispatch_table);

    // function pointers for inplace operation on contiguous matrix
    // and contiguous row
    using fn_ns::AddInplaceRowMatrixBroadcastFactory;
    DispatchTableBuilder<binary_inplace_row_matrix_broadcast_impl_fn_ptr_t,
                         AddInplaceRowMatrixBroadcastFactory, num_types>
        dtb8;
    dtb8.populate_dispatch_table(add_inplace_row_matrix_dispatch_table);
};

} // namespace impl

// U04: ===== ASIN  (x)
namespace impl
{
// FIXME: add code for U04
} // namespace impl

// U05: ===== ASINH (x)
namespace impl
{
// FIXME: add code for U05
} // namespace impl

// U06: ===== ATAN  (x)
namespace impl
{
// FIXME: add code for U06
} // namespace impl

// B02: ===== ATAN2 (x1, x2)
namespace impl
{
// FIXME: add code for B02
} // namespace impl

// U07: ===== ATANH (x)
namespace impl
{
// FIXME: add code for U07
} // namespace impl

// B03: ===== BITWISE_AND           (x1, x2)
namespace impl
{
// FIXME: add code for B03
} // namespace impl

// B04: ===== BITWISE_LEFT_SHIFT    (x1, x2)
namespace impl
{
// FIXME: add code for B04
} // namespace impl

// U08: ===== BITWISE_INVERT        (x)
namespace impl
{
// FIXME: add code for U08
} // namespace impl

// B05: ===== BITWISE_OR            (x1, x2)
namespace impl
{
// FIXME: add code for B05
} // namespace impl

// B06: ===== BITWISE_RIGHT_SHIFT   (x1, x2)
namespace impl
{
// FIXME: add code for B06
} // namespace impl

// B07: ===== BITWISE_XOR           (x1, x2)
namespace impl
{
// FIXME: add code for B07
} // namespace impl

// U09: ==== CEIL          (x)
namespace impl
{
// FIXME: add code for U09
} // namespace impl

// U10: ==== CONJ          (x)
namespace impl
{

namespace conj_fn_ns = dpctl::tensor::kernels::conj;

static unary_contig_impl_fn_ptr_t conj_contig_dispatch_vector[td_ns::num_types];
static int conj_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    conj_strided_dispatch_vector[td_ns::num_types];

void populate_conj_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = conj_fn_ns;

    using fn_ns::ConjContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, ConjContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(conj_contig_dispatch_vector);

    using fn_ns::ConjStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, ConjStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(conj_strided_dispatch_vector);

    using fn_ns::ConjTypeMapFactory;
    DispatchVectorBuilder<int, ConjTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(conj_output_typeid_vector);
}
} // namespace impl

// U11: ==== COS           (x)
namespace impl
{

namespace cos_fn_ns = dpctl::tensor::kernels::cos;

static unary_contig_impl_fn_ptr_t cos_contig_dispatch_vector[td_ns::num_types];
static int cos_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    cos_strided_dispatch_vector[td_ns::num_types];

void populate_cos_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = cos_fn_ns;

    using fn_ns::CosContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, CosContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(cos_contig_dispatch_vector);

    using fn_ns::CosStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, CosStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(cos_strided_dispatch_vector);

    using fn_ns::CosTypeMapFactory;
    DispatchVectorBuilder<int, CosTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(cos_output_typeid_vector);
}

} // namespace impl

// U12: ==== COSH          (x)
namespace impl
{
// FIXME: add code for U12
} // namespace impl

// B08: ==== DIVIDE        (x1, x2)
namespace impl
{
namespace true_divide_fn_ns = dpctl::tensor::kernels::true_divide;

static binary_contig_impl_fn_ptr_t
    true_divide_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int true_divide_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    true_divide_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

// divide(matrix, row)
static binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t
    true_divide_contig_matrix_contig_row_broadcast_dispatch_table
        [td_ns::num_types][td_ns::num_types];

// divide(row, matrix)
static binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t
    true_divide_contig_row_contig_matrix_broadcast_dispatch_table
        [td_ns::num_types][td_ns::num_types];

void populate_true_divide_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = true_divide_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::TrueDivideTypeMapFactory;
    DispatchTableBuilder<int, TrueDivideTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(true_divide_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::TrueDivideStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, TrueDivideStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(true_divide_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::TrueDivideContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, TrueDivideContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(true_divide_contig_dispatch_table);

    // function pointers for operation on contiguous matrix, contiguous row
    // with contiguous matrix output
    using fn_ns::TrueDivideContigMatrixContigRowBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t,
        TrueDivideContigMatrixContigRowBroadcastFactory, num_types>
        dtb4;
    dtb4.populate_dispatch_table(
        true_divide_contig_matrix_contig_row_broadcast_dispatch_table);

    // function pointers for operation on contiguous row, contiguous matrix
    // with contiguous matrix output
    using fn_ns::TrueDivideContigRowContigMatrixBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t,
        TrueDivideContigRowContigMatrixBroadcastFactory, num_types>
        dtb5;
    dtb5.populate_dispatch_table(
        true_divide_contig_row_contig_matrix_broadcast_dispatch_table);
};

} // namespace impl

// B09: ==== EQUAL         (x1, x2)
namespace impl
{
namespace equal_fn_ns = dpctl::tensor::kernels::equal;

static binary_contig_impl_fn_ptr_t
    equal_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int equal_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    equal_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_equal_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = equal_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::EqualTypeMapFactory;
    DispatchTableBuilder<int, EqualTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(equal_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::EqualStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, EqualStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(equal_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::EqualContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, EqualContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(equal_contig_dispatch_table);
};
} // namespace impl

// U13: ==== EXP           (x)
namespace impl
{

namespace exp_fn_ns = dpctl::tensor::kernels::exp;

static unary_contig_impl_fn_ptr_t exp_contig_dispatch_vector[td_ns::num_types];
static int exp_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    exp_strided_dispatch_vector[td_ns::num_types];

void populate_exp_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = exp_fn_ns;

    using fn_ns::ExpContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, ExpContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(exp_contig_dispatch_vector);

    using fn_ns::ExpStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, ExpStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(exp_strided_dispatch_vector);

    using fn_ns::ExpTypeMapFactory;
    DispatchVectorBuilder<int, ExpTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(exp_output_typeid_vector);
}

} // namespace impl

// U14: ==== EXPM1         (x)
namespace impl
{

namespace expm1_fn_ns = dpctl::tensor::kernels::expm1;

static unary_contig_impl_fn_ptr_t
    expm1_contig_dispatch_vector[td_ns::num_types];
static int expm1_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    expm1_strided_dispatch_vector[td_ns::num_types];

void populate_expm1_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = expm1_fn_ns;

    using fn_ns::Expm1ContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, Expm1ContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(expm1_contig_dispatch_vector);

    using fn_ns::Expm1StridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, Expm1StridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(expm1_strided_dispatch_vector);

    using fn_ns::Expm1TypeMapFactory;
    DispatchVectorBuilder<int, Expm1TypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(expm1_output_typeid_vector);
}

} // namespace impl

// U15: ==== FLOOR         (x)
namespace impl
{
// FIXME: add code for U15
} // namespace impl

// B10: ==== FLOOR_DIVIDE  (x1, x2)
namespace impl
{
namespace floor_divide_fn_ns = dpctl::tensor::kernels::floor_divide;

static binary_contig_impl_fn_ptr_t
    floor_divide_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int floor_divide_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    floor_divide_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_floor_divide_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = floor_divide_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::FloorDivideTypeMapFactory;
    DispatchTableBuilder<int, FloorDivideTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(floor_divide_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::FloorDivideStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t,
                         FloorDivideStridedFactory, num_types>
        dtb2;
    dtb2.populate_dispatch_table(floor_divide_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::FloorDivideContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, FloorDivideContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(floor_divide_contig_dispatch_table);
};

} // namespace impl

// B11: ==== GREATER       (x1, x2)
namespace impl
{
namespace greater_fn_ns = dpctl::tensor::kernels::greater;

static binary_contig_impl_fn_ptr_t
    greater_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int greater_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    greater_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_greater_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = greater_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::GreaterTypeMapFactory;
    DispatchTableBuilder<int, GreaterTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(greater_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::GreaterStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, GreaterStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(greater_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::GreaterContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, GreaterContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(greater_contig_dispatch_table);
};
} // namespace impl

// B12: ==== GREATER_EQUAL (x1, x2)
namespace impl
{
namespace greater_equal_fn_ns = dpctl::tensor::kernels::greater_equal;

static binary_contig_impl_fn_ptr_t
    greater_equal_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int greater_equal_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    greater_equal_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_greater_equal_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = greater_equal_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::GreaterEqualTypeMapFactory;
    DispatchTableBuilder<int, GreaterEqualTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(greater_equal_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::GreaterEqualStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t,
                         GreaterEqualStridedFactory, num_types>
        dtb2;
    dtb2.populate_dispatch_table(greater_equal_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::GreaterEqualContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, GreaterEqualContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(greater_equal_contig_dispatch_table);
};
} // namespace impl

// U16: ==== IMAG        (x)
namespace impl
{

namespace imag_fn_ns = dpctl::tensor::kernels::imag;

static unary_contig_impl_fn_ptr_t imag_contig_dispatch_vector[td_ns::num_types];
static int imag_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    imag_strided_dispatch_vector[td_ns::num_types];

void populate_imag_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = imag_fn_ns;

    using fn_ns::ImagContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, ImagContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(imag_contig_dispatch_vector);

    using fn_ns::ImagStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, ImagStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(imag_strided_dispatch_vector);

    using fn_ns::ImagTypeMapFactory;
    DispatchVectorBuilder<int, ImagTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(imag_output_typeid_vector);
}
} // namespace impl

// U17: ==== ISFINITE    (x)
namespace impl
{
namespace isfinite_fn_ns = dpctl::tensor::kernels::isfinite;

static unary_contig_impl_fn_ptr_t
    isfinite_contig_dispatch_vector[td_ns::num_types];
static int isfinite_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    isfinite_strided_dispatch_vector[td_ns::num_types];

void populate_isfinite_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = isfinite_fn_ns;

    using fn_ns::IsFiniteContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, IsFiniteContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(isfinite_contig_dispatch_vector);

    using fn_ns::IsFiniteStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, IsFiniteStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(isfinite_strided_dispatch_vector);

    using fn_ns::IsFiniteTypeMapFactory;
    DispatchVectorBuilder<int, IsFiniteTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(isfinite_output_typeid_vector);
}

} // namespace impl

// U18: ==== ISINF       (x)
namespace impl
{
namespace isinf_fn_ns = dpctl::tensor::kernels::isinf;

static unary_contig_impl_fn_ptr_t
    isinf_contig_dispatch_vector[td_ns::num_types];
static int isinf_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    isinf_strided_dispatch_vector[td_ns::num_types];

void populate_isinf_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = isinf_fn_ns;

    using fn_ns::IsInfContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, IsInfContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(isinf_contig_dispatch_vector);

    using fn_ns::IsInfStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, IsInfStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(isinf_strided_dispatch_vector);

    using fn_ns::IsInfTypeMapFactory;
    DispatchVectorBuilder<int, IsInfTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(isinf_output_typeid_vector);
}

} // namespace impl

// U19: ==== ISNAN       (x)
namespace impl
{
namespace isnan_fn_ns = dpctl::tensor::kernels::isnan;

static unary_contig_impl_fn_ptr_t
    isnan_contig_dispatch_vector[td_ns::num_types];
static int isnan_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    isnan_strided_dispatch_vector[td_ns::num_types];

void populate_isnan_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = isnan_fn_ns;

    using fn_ns::IsNanContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, IsNanContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(isnan_contig_dispatch_vector);

    using fn_ns::IsNanStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, IsNanStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(isnan_strided_dispatch_vector);

    using fn_ns::IsNanTypeMapFactory;
    DispatchVectorBuilder<int, IsNanTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(isnan_output_typeid_vector);
}

} // namespace impl

// B13: ==== LESS        (x1, x2)
namespace impl
{
namespace less_fn_ns = dpctl::tensor::kernels::less;

static binary_contig_impl_fn_ptr_t less_contig_dispatch_table[td_ns::num_types]
                                                             [td_ns::num_types];
static int less_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    less_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_less_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = less_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::LessTypeMapFactory;
    DispatchTableBuilder<int, LessTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(less_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::LessStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, LessStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(less_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::LessContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, LessContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(less_contig_dispatch_table);
};
} // namespace impl

// B14: ==== LESS_EQUAL  (x1, x2)
namespace impl
{
namespace less_equal_fn_ns = dpctl::tensor::kernels::less_equal;

static binary_contig_impl_fn_ptr_t
    less_equal_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int less_equal_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    less_equal_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_less_equal_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = less_equal_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::LessEqualTypeMapFactory;
    DispatchTableBuilder<int, LessEqualTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(less_equal_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::LessEqualStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, LessEqualStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(less_equal_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::LessEqualContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, LessEqualContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(less_equal_contig_dispatch_table);
};
} // namespace impl

// U20: ==== LOG         (x)
namespace impl
{

namespace log_fn_ns = dpctl::tensor::kernels::log;

static unary_contig_impl_fn_ptr_t log_contig_dispatch_vector[td_ns::num_types];
static int log_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    log_strided_dispatch_vector[td_ns::num_types];

void populate_log_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = log_fn_ns;

    using fn_ns::LogContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, LogContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(log_contig_dispatch_vector);

    using fn_ns::LogStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, LogStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(log_strided_dispatch_vector);

    using fn_ns::LogTypeMapFactory;
    DispatchVectorBuilder<int, LogTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(log_output_typeid_vector);
}

} // namespace impl

// U21: ==== LOG1P       (x)
namespace impl
{

namespace log1p_fn_ns = dpctl::tensor::kernels::log1p;

static unary_contig_impl_fn_ptr_t
    log1p_contig_dispatch_vector[td_ns::num_types];
static int log1p_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    log1p_strided_dispatch_vector[td_ns::num_types];

void populate_log1p_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = log1p_fn_ns;

    using fn_ns::Log1pContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, Log1pContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(log1p_contig_dispatch_vector);

    using fn_ns::Log1pStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, Log1pStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(log1p_strided_dispatch_vector);

    using fn_ns::Log1pTypeMapFactory;
    DispatchVectorBuilder<int, Log1pTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(log1p_output_typeid_vector);
}

} // namespace impl

// U22: ==== LOG2        (x)
namespace impl
{

namespace log2_fn_ns = dpctl::tensor::kernels::log2;

static unary_contig_impl_fn_ptr_t log2_contig_dispatch_vector[td_ns::num_types];
static int log2_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    log2_strided_dispatch_vector[td_ns::num_types];

void populate_log2_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = log2_fn_ns;

    using fn_ns::Log2ContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, Log2ContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(log2_contig_dispatch_vector);

    using fn_ns::Log2StridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, Log2StridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(log2_strided_dispatch_vector);

    using fn_ns::Log2TypeMapFactory;
    DispatchVectorBuilder<int, Log2TypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(log2_output_typeid_vector);
};

} // namespace impl

// U23: ==== LOG10       (x)
namespace impl
{

namespace log10_fn_ns = dpctl::tensor::kernels::log10;

static unary_contig_impl_fn_ptr_t
    log10_contig_dispatch_vector[td_ns::num_types];
static int log10_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    log10_strided_dispatch_vector[td_ns::num_types];

void populate_log10_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = log10_fn_ns;

    using fn_ns::Log10ContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, Log10ContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(log10_contig_dispatch_vector);

    using fn_ns::Log10StridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, Log10StridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(log10_strided_dispatch_vector);

    using fn_ns::Log10TypeMapFactory;
    DispatchVectorBuilder<int, Log10TypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(log10_output_typeid_vector);
};

} // namespace impl

// B15: ==== LOGADDEXP   (x1, x2)
namespace impl
{
// FIXME: add code for B15
} // namespace impl

// B16: ==== LOGICAL_AND (x1, x2)
namespace impl
{
namespace logical_and_fn_ns = dpctl::tensor::kernels::logical_and;

static binary_contig_impl_fn_ptr_t
    logical_and_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int logical_and_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    logical_and_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_logical_and_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = logical_and_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::LogicalAndTypeMapFactory;
    DispatchTableBuilder<int, LogicalAndTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(logical_and_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::LogicalAndStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, LogicalAndStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(logical_and_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::LogicalAndContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, LogicalAndContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(logical_and_contig_dispatch_table);
};
} // namespace impl

// U24: ==== LOGICAL_NOT (x)
namespace impl
{
namespace logical_not_fn_ns = dpctl::tensor::kernels::logical_not;

static unary_contig_impl_fn_ptr_t
    logical_not_contig_dispatch_vector[td_ns::num_types];
static int logical_not_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    logical_not_strided_dispatch_vector[td_ns::num_types];

void populate_logical_not_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = logical_not_fn_ns;

    using fn_ns::LogicalNotContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, LogicalNotContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(logical_not_contig_dispatch_vector);

    using fn_ns::LogicalNotStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, LogicalNotStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(logical_not_strided_dispatch_vector);

    using fn_ns::LogicalNotTypeMapFactory;
    DispatchVectorBuilder<int, LogicalNotTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(logical_not_output_typeid_vector);
};
} // namespace impl

// B17: ==== LOGICAL_OR  (x1, x2)
namespace impl
{
namespace logical_or_fn_ns = dpctl::tensor::kernels::logical_or;

static binary_contig_impl_fn_ptr_t
    logical_or_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int logical_or_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    logical_or_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_logical_or_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = logical_or_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::LogicalOrTypeMapFactory;
    DispatchTableBuilder<int, LogicalOrTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(logical_or_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::LogicalOrStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, LogicalOrStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(logical_or_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::LogicalOrContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, LogicalOrContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(logical_or_contig_dispatch_table);
};
} // namespace impl

// B18: ==== LOGICAL_XOR (x1, x2)
namespace impl
{
namespace logical_xor_fn_ns = dpctl::tensor::kernels::logical_xor;

static binary_contig_impl_fn_ptr_t
    logical_xor_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int logical_xor_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    logical_xor_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_logical_xor_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = logical_xor_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::LogicalXorTypeMapFactory;
    DispatchTableBuilder<int, LogicalXorTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(logical_xor_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::LogicalXorStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, LogicalXorStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(logical_xor_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::LogicalXorContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, LogicalXorContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(logical_xor_contig_dispatch_table);
};
} // namespace impl

// B19: ==== MULTIPLY    (x1, x2)
namespace impl
{

namespace multiply_fn_ns = dpctl::tensor::kernels::multiply;

static binary_contig_impl_fn_ptr_t
    multiply_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int multiply_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    multiply_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

// mul(matrix, row)
static binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t
    multiply_contig_matrix_contig_row_broadcast_dispatch_table
        [td_ns::num_types][td_ns::num_types];

// mul(row, matrix)
static binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t
    multiply_contig_row_contig_matrix_broadcast_dispatch_table
        [td_ns::num_types][td_ns::num_types];

static binary_inplace_contig_impl_fn_ptr_t
    multiply_inplace_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static binary_inplace_strided_impl_fn_ptr_t
    multiply_inplace_strided_dispatch_table[td_ns::num_types][td_ns::num_types];
static binary_inplace_row_matrix_broadcast_impl_fn_ptr_t
    multiply_inplace_row_matrix_dispatch_table[td_ns::num_types]
                                              [td_ns::num_types];

void populate_multiply_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = multiply_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::MultiplyTypeMapFactory;
    DispatchTableBuilder<int, MultiplyTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(multiply_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::MultiplyStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, MultiplyStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(multiply_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::MultiplyContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, MultiplyContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(multiply_contig_dispatch_table);

    // function pointers for operation on contiguous matrix, contiguous row
    // with contiguous matrix output
    using fn_ns::MultiplyContigMatrixContigRowBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t,
        MultiplyContigMatrixContigRowBroadcastFactory, num_types>
        dtb4;
    dtb4.populate_dispatch_table(
        multiply_contig_matrix_contig_row_broadcast_dispatch_table);

    // function pointers for operation on contiguous row, contiguous matrix
    // with contiguous matrix output
    using fn_ns::MultiplyContigRowContigMatrixBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t,
        MultiplyContigRowContigMatrixBroadcastFactory, num_types>
        dtb5;
    dtb5.populate_dispatch_table(
        multiply_contig_row_contig_matrix_broadcast_dispatch_table);

    // function pointers for inplace operation on general strided arrays
    using fn_ns::MultiplyInplaceStridedFactory;
    DispatchTableBuilder<binary_inplace_strided_impl_fn_ptr_t,
                         MultiplyInplaceStridedFactory, num_types>
        dtb6;
    dtb6.populate_dispatch_table(multiply_inplace_strided_dispatch_table);

    // function pointers for inplace operation on contiguous inputs and output
    using fn_ns::MultiplyInplaceContigFactory;
    DispatchTableBuilder<binary_inplace_contig_impl_fn_ptr_t,
                         MultiplyInplaceContigFactory, num_types>
        dtb7;
    dtb7.populate_dispatch_table(multiply_inplace_contig_dispatch_table);

    // function pointers for inplace operation on contiguous matrix
    // and contiguous row
    using fn_ns::MultiplyInplaceRowMatrixBroadcastFactory;
    DispatchTableBuilder<binary_inplace_row_matrix_broadcast_impl_fn_ptr_t,
                         MultiplyInplaceRowMatrixBroadcastFactory, num_types>
        dtb8;
    dtb8.populate_dispatch_table(multiply_inplace_row_matrix_dispatch_table);
};

} // namespace impl

// U25: ==== NEGATIVE    (x)
namespace impl
{

namespace negative_fn_ns = dpctl::tensor::kernels::negative;

static unary_contig_impl_fn_ptr_t
    negative_contig_dispatch_vector[td_ns::num_types];
static int negative_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    negative_strided_dispatch_vector[td_ns::num_types];

void populate_negative_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = negative_fn_ns;

    using fn_ns::NegativeContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, NegativeContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(negative_contig_dispatch_vector);

    using fn_ns::NegativeStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, NegativeStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(negative_strided_dispatch_vector);

    using fn_ns::NegativeTypeMapFactory;
    DispatchVectorBuilder<int, NegativeTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(negative_output_typeid_vector);
}

} // namespace impl

// B20: ==== NOT_EQUAL   (x1, x2)
namespace impl
{
namespace not_equal_fn_ns = dpctl::tensor::kernels::not_equal;

static binary_contig_impl_fn_ptr_t
    not_equal_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int not_equal_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    not_equal_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_not_equal_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = not_equal_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::NotEqualTypeMapFactory;
    DispatchTableBuilder<int, NotEqualTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(not_equal_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::NotEqualStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, NotEqualStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(not_equal_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::NotEqualContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, NotEqualContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(not_equal_contig_dispatch_table);
};
} // namespace impl

// U26: ==== POSITIVE    (x)
namespace impl
{

namespace positive_fn_ns = dpctl::tensor::kernels::positive;

static unary_contig_impl_fn_ptr_t
    positive_contig_dispatch_vector[td_ns::num_types];
static int positive_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    positive_strided_dispatch_vector[td_ns::num_types];

void populate_positive_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = positive_fn_ns;

    using fn_ns::PositiveContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, PositiveContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(positive_contig_dispatch_vector);

    using fn_ns::PositiveStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, PositiveStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(positive_strided_dispatch_vector);

    using fn_ns::PositiveTypeMapFactory;
    DispatchVectorBuilder<int, PositiveTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(positive_output_typeid_vector);
}

} // namespace impl

// B21: ==== POW         (x1, x2)
namespace impl
{

namespace pow_fn_ns = dpctl::tensor::kernels::pow;

static binary_contig_impl_fn_ptr_t pow_contig_dispatch_table[td_ns::num_types]
                                                            [td_ns::num_types];
static int pow_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    pow_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

void populate_pow_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = pow_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::PowTypeMapFactory;
    DispatchTableBuilder<int, PowTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(pow_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::PowStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, PowStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(pow_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::PowContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, PowContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(pow_contig_dispatch_table);
};

} // namespace impl

// U??: ==== PROJ        (x)
namespace impl
{

namespace proj_fn_ns = dpctl::tensor::kernels::proj;

static unary_contig_impl_fn_ptr_t proj_contig_dispatch_vector[td_ns::num_types];
static int proj_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    proj_strided_dispatch_vector[td_ns::num_types];

void populate_proj_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = proj_fn_ns;

    using fn_ns::ProjContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, ProjContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(proj_contig_dispatch_vector);

    using fn_ns::ProjStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, ProjStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(proj_strided_dispatch_vector);

    using fn_ns::ProjTypeMapFactory;
    DispatchVectorBuilder<int, ProjTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(proj_output_typeid_vector);
}
} // namespace impl

// U27: ==== REAL        (x)
namespace impl
{

namespace real_fn_ns = dpctl::tensor::kernels::real;

static unary_contig_impl_fn_ptr_t real_contig_dispatch_vector[td_ns::num_types];
static int real_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    real_strided_dispatch_vector[td_ns::num_types];

void populate_real_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = real_fn_ns;

    using fn_ns::RealContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, RealContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(real_contig_dispatch_vector);

    using fn_ns::RealStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, RealStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(real_strided_dispatch_vector);

    using fn_ns::RealTypeMapFactory;
    DispatchVectorBuilder<int, RealTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(real_output_typeid_vector);
}
} // namespace impl

// B22: ==== REMAINDER   (x1, x2)
namespace impl
{
// FIXME: add code for B22
} // namespace impl

// U28: ==== ROUND       (x)
namespace impl
{
// FIXME: add code for U28
} // namespace impl

// U29: ==== SIGN        (x)
namespace impl
{
// FIXME: add code for U29
} // namespace impl

// U30: ==== SIN         (x)
namespace impl
{

namespace sin_fn_ns = dpctl::tensor::kernels::sin;

static unary_contig_impl_fn_ptr_t sin_contig_dispatch_vector[td_ns::num_types];
static int sin_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    sin_strided_dispatch_vector[td_ns::num_types];

void populate_sin_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = sin_fn_ns;

    using fn_ns::SinContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, SinContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(sin_contig_dispatch_vector);

    using fn_ns::SinStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, SinStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(sin_strided_dispatch_vector);

    using fn_ns::SinTypeMapFactory;
    DispatchVectorBuilder<int, SinTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(sin_output_typeid_vector);
}

} // namespace impl

// U31: ==== SINH        (x)
namespace impl
{
// FIXME: add code for U31
} // namespace impl

// U32: ==== SQUARE      (x)
namespace impl
{

namespace square_fn_ns = dpctl::tensor::kernels::square;

static unary_contig_impl_fn_ptr_t
    square_contig_dispatch_vector[td_ns::num_types];
static int square_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    square_strided_dispatch_vector[td_ns::num_types];

void populate_square_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = square_fn_ns;

    using fn_ns::SquareContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, SquareContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(square_contig_dispatch_vector);

    using fn_ns::SquareStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, SquareStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(square_strided_dispatch_vector);

    using fn_ns::SquareTypeMapFactory;
    DispatchVectorBuilder<int, SquareTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(square_output_typeid_vector);
}

} // namespace impl

// U33: ==== SQRT        (x)
namespace impl
{

namespace sqrt_fn_ns = dpctl::tensor::kernels::sqrt;

static unary_contig_impl_fn_ptr_t sqrt_contig_dispatch_vector[td_ns::num_types];
static int sqrt_output_typeid_vector[td_ns::num_types];
static unary_strided_impl_fn_ptr_t
    sqrt_strided_dispatch_vector[td_ns::num_types];

void populate_sqrt_dispatch_vectors(void)
{
    using namespace td_ns;
    namespace fn_ns = sqrt_fn_ns;

    using fn_ns::SqrtContigFactory;
    DispatchVectorBuilder<unary_contig_impl_fn_ptr_t, SqrtContigFactory,
                          num_types>
        dvb1;
    dvb1.populate_dispatch_vector(sqrt_contig_dispatch_vector);

    using fn_ns::SqrtStridedFactory;
    DispatchVectorBuilder<unary_strided_impl_fn_ptr_t, SqrtStridedFactory,
                          num_types>
        dvb2;
    dvb2.populate_dispatch_vector(sqrt_strided_dispatch_vector);

    using fn_ns::SqrtTypeMapFactory;
    DispatchVectorBuilder<int, SqrtTypeMapFactory, num_types> dvb3;
    dvb3.populate_dispatch_vector(sqrt_output_typeid_vector);
}

} // namespace impl

// B23: ==== SUBTRACT    (x1, x2)
namespace impl
{
namespace subtract_fn_ns = dpctl::tensor::kernels::subtract;

static binary_contig_impl_fn_ptr_t
    subtract_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static int subtract_output_id_table[td_ns::num_types][td_ns::num_types];

static binary_strided_impl_fn_ptr_t
    subtract_strided_dispatch_table[td_ns::num_types][td_ns::num_types];

// sub(matrix, row)
static binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t
    subtract_contig_matrix_contig_row_broadcast_dispatch_table
        [td_ns::num_types][td_ns::num_types];

// sub(row, matrix)
static binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t
    subtract_contig_row_contig_matrix_broadcast_dispatch_table
        [td_ns::num_types][td_ns::num_types];

static binary_inplace_contig_impl_fn_ptr_t
    subtract_inplace_contig_dispatch_table[td_ns::num_types][td_ns::num_types];
static binary_inplace_strided_impl_fn_ptr_t
    subtract_inplace_strided_dispatch_table[td_ns::num_types][td_ns::num_types];
static binary_inplace_row_matrix_broadcast_impl_fn_ptr_t
    subtract_inplace_row_matrix_dispatch_table[td_ns::num_types]
                                              [td_ns::num_types];

void populate_subtract_dispatch_tables(void)
{
    using namespace td_ns;
    namespace fn_ns = subtract_fn_ns;

    // which input types are supported, and what is the type of the result
    using fn_ns::SubtractTypeMapFactory;
    DispatchTableBuilder<int, SubtractTypeMapFactory, num_types> dtb1;
    dtb1.populate_dispatch_table(subtract_output_id_table);

    // function pointers for operation on general strided arrays
    using fn_ns::SubtractStridedFactory;
    DispatchTableBuilder<binary_strided_impl_fn_ptr_t, SubtractStridedFactory,
                         num_types>
        dtb2;
    dtb2.populate_dispatch_table(subtract_strided_dispatch_table);

    // function pointers for operation on contiguous inputs and output
    using fn_ns::SubtractContigFactory;
    DispatchTableBuilder<binary_contig_impl_fn_ptr_t, SubtractContigFactory,
                         num_types>
        dtb3;
    dtb3.populate_dispatch_table(subtract_contig_dispatch_table);

    // function pointers for operation on contiguous matrix, contiguous row
    // with contiguous matrix output
    using fn_ns::SubtractContigMatrixContigRowBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t,
        SubtractContigMatrixContigRowBroadcastFactory, num_types>
        dtb4;
    dtb4.populate_dispatch_table(
        subtract_contig_matrix_contig_row_broadcast_dispatch_table);

    // function pointers for operation on contiguous row, contiguous matrix
    // with contiguous matrix output
    using fn_ns::SubtractContigRowContigMatrixBroadcastFactory;
    DispatchTableBuilder<
        binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t,
        SubtractContigRowContigMatrixBroadcastFactory, num_types>
        dtb5;
    dtb5.populate_dispatch_table(
        subtract_contig_row_contig_matrix_broadcast_dispatch_table);

    // function pointers for inplace operation on general strided arrays
    using fn_ns::SubtractInplaceStridedFactory;
    DispatchTableBuilder<binary_inplace_strided_impl_fn_ptr_t,
                         SubtractInplaceStridedFactory, num_types>
        dtb6;
    dtb6.populate_dispatch_table(subtract_inplace_strided_dispatch_table);

    // function pointers for inplace operation on contiguous inputs and output
    using fn_ns::SubtractInplaceContigFactory;
    DispatchTableBuilder<binary_inplace_contig_impl_fn_ptr_t,
                         SubtractInplaceContigFactory, num_types>
        dtb7;
    dtb7.populate_dispatch_table(subtract_inplace_contig_dispatch_table);

    // function pointers for inplace operation on contiguous matrix
    // and contiguous row
    using fn_ns::SubtractInplaceRowMatrixBroadcastFactory;
    DispatchTableBuilder<binary_inplace_row_matrix_broadcast_impl_fn_ptr_t,
                         SubtractInplaceRowMatrixBroadcastFactory, num_types>
        dtb8;
    dtb8.populate_dispatch_table(subtract_inplace_row_matrix_dispatch_table);
};

} // namespace impl

// U34: ==== TAN         (x)
namespace impl
{
// FIXME: add code for U34
} // namespace impl

// U35: ==== TANH        (x)
namespace impl
{
// FIXME: add code for U35
} // namespace impl

// U36: ==== TRUNC       (x)
namespace impl
{
// FIXME: add code for U36
} // namespace impl

// ==========================================================================================
// //

namespace py = pybind11;

void init_elementwise_functions(py::module_ m)
{
    using arrayT = dpctl::tensor::usm_ndarray;
    using event_vecT = std::vector<sycl::event>;

    // U01: ==== ABS   (x)
    {
        impl::populate_abs_dispatch_vectors();
        using impl::abs_contig_dispatch_vector;
        using impl::abs_output_typeid_vector;
        using impl::abs_strided_dispatch_vector;

        auto abs_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                             const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, abs_output_typeid_vector,
                abs_contig_dispatch_vector, abs_strided_dispatch_vector);
        };
        m.def("_abs", abs_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto abs_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, abs_output_typeid_vector);
        };
        m.def("_abs_result_type", abs_result_type_pyapi);
    }

    // U02: ==== ACOS   (x)
    // FIXME:

    // U03: ===== ACOSH (x)
    // FIXME:

    // B01: ===== ADD   (x1, x2)
    {
        impl::populate_add_dispatch_tables();
        using impl::add_contig_dispatch_table;
        using impl::add_contig_matrix_contig_row_broadcast_dispatch_table;
        using impl::add_contig_row_contig_matrix_broadcast_dispatch_table;
        using impl::add_output_id_table;
        using impl::add_strided_dispatch_table;

        auto add_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                             dpctl::tensor::usm_ndarray src2,
                             dpctl::tensor::usm_ndarray dst, sycl::queue exec_q,
                             const std::vector<sycl::event> &depends = {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, add_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                add_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                add_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                add_contig_matrix_contig_row_broadcast_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                add_contig_row_contig_matrix_broadcast_dispatch_table);
        };
        auto add_result_type_pyapi = [&](py::dtype dtype1, py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               add_output_id_table);
        };
        m.def("_add", add_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_add_result_type", add_result_type_pyapi, "");

        using impl::add_inplace_contig_dispatch_table;
        using impl::add_inplace_row_matrix_dispatch_table;
        using impl::add_inplace_strided_dispatch_table;

        auto add_inplace_pyapi =
            [&](dpctl::tensor::usm_ndarray src, dpctl::tensor::usm_ndarray dst,
                sycl::queue exec_q,
                const std::vector<sycl::event> &depends = {}) {
                return py_binary_inplace_ufunc(
                    src, dst, exec_q, depends, add_output_id_table,
                    // function pointers to handle inplace operation on
                    // contiguous arrays (pointers may be nullptr)
                    add_inplace_contig_dispatch_table,
                    // function pointers to handle inplace operation on strided
                    // arrays (most general case)
                    add_inplace_strided_dispatch_table,
                    // function pointers to handle inplace operation on
                    // c-contig matrix with c-contig row with broadcasting
                    // (may be nullptr)
                    add_inplace_row_matrix_dispatch_table);
            };
        m.def("_add_inplace", add_inplace_pyapi, "", py::arg("lhs"),
              py::arg("rhs"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
    }

    // U04: ===== ASIN  (x)
    // FIXME:

    // U05: ===== ASINH (x)
    // FIXME:

    // U06: ===== ATAN  (x)
    // FIXME:

    // B02: ===== ATAN2 (x1, x2)
    // FIXME:

    // U07: ===== ATANH (x)
    // FIXME:

    // B03: ===== BITWISE_AND           (x1, x2)
    // FIXME:

    // B04: ===== BITWISE_LEFT_SHIFT    (x1, x2)
    // FIXME:

    // U08: ===== BITWISE_INVERT        (x)
    // FIXME:

    // B05: ===== BITWISE_OR            (x1, x2)
    // FIXME:

    // B06: ===== BITWISE_RIGHT_SHIFT   (x1, x2)
    // FIXME:

    // B07: ===== BITWISE_XOR           (x1, x2)
    // FIXME:

    // U09: ==== CEIL          (x)
    // FIXME:

    // U10: ==== CONJ          (x)
    {
        impl::populate_conj_dispatch_vectors();
        using impl::conj_contig_dispatch_vector;
        using impl::conj_output_typeid_vector;
        using impl::conj_strided_dispatch_vector;

        auto conj_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                              const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, conj_output_typeid_vector,
                conj_contig_dispatch_vector, conj_strided_dispatch_vector);
        };
        m.def("_conj", conj_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto conj_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, conj_output_typeid_vector);
        };
        m.def("_conj_result_type", conj_result_type_pyapi);
    }

    // U11: ==== COS           (x)
    {
        impl::populate_cos_dispatch_vectors();
        using impl::cos_contig_dispatch_vector;
        using impl::cos_output_typeid_vector;
        using impl::cos_strided_dispatch_vector;

        auto cos_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                             const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, cos_output_typeid_vector,
                cos_contig_dispatch_vector, cos_strided_dispatch_vector);
        };
        m.def("_cos", cos_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto cos_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, cos_output_typeid_vector);
        };
        m.def("_cos_result_type", cos_result_type_pyapi);
    }

    // U12: ==== COSH          (x)
    // FIXME:

    // B08: ==== DIVIDE        (x1, x2)
    {
        impl::populate_true_divide_dispatch_tables();
        using impl::true_divide_contig_dispatch_table;
        using impl::
            true_divide_contig_matrix_contig_row_broadcast_dispatch_table;
        using impl::
            true_divide_contig_row_contig_matrix_broadcast_dispatch_table;
        using impl::true_divide_output_id_table;
        using impl::true_divide_strided_dispatch_table;

        auto divide_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                dpctl::tensor::usm_ndarray src2,
                                dpctl::tensor::usm_ndarray dst,
                                sycl::queue exec_q,
                                const std::vector<sycl::event> &depends = {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, true_divide_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                true_divide_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                true_divide_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                true_divide_contig_matrix_contig_row_broadcast_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                true_divide_contig_row_contig_matrix_broadcast_dispatch_table);
        };
        auto divide_result_type_pyapi = [&](py::dtype dtype1,
                                            py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               true_divide_output_id_table);
        };
        m.def("_divide", divide_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_divide_result_type", divide_result_type_pyapi, "");
    }

    // B09: ==== EQUAL         (x1, x2)
    {
        impl::populate_equal_dispatch_tables();
        using impl::equal_contig_dispatch_table;
        using impl::equal_output_id_table;
        using impl::equal_strided_dispatch_table;

        auto equal_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                               dpctl::tensor::usm_ndarray src2,
                               dpctl::tensor::usm_ndarray dst,
                               sycl::queue exec_q,
                               const std::vector<sycl::event> &depends = {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, equal_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                equal_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                equal_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto equal_result_type_pyapi = [&](py::dtype dtype1, py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               equal_output_id_table);
        };
        m.def("_equal", equal_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_equal_result_type", equal_result_type_pyapi, "");
    }

    // U13: ==== EXP           (x)
    {
        impl::populate_exp_dispatch_vectors();
        using impl::exp_contig_dispatch_vector;
        using impl::exp_output_typeid_vector;
        using impl::exp_strided_dispatch_vector;

        auto exp_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                             const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, exp_output_typeid_vector,
                exp_contig_dispatch_vector, exp_strided_dispatch_vector);
        };
        m.def("_exp", exp_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto exp_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, exp_output_typeid_vector);
        };
        m.def("_exp_result_type", exp_result_type_pyapi);
    }

    // U14: ==== EXPM1         (x)
    {
        impl::populate_expm1_dispatch_vectors();
        using impl::expm1_contig_dispatch_vector;
        using impl::expm1_output_typeid_vector;
        using impl::expm1_strided_dispatch_vector;

        auto expm1_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                               const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, expm1_output_typeid_vector,
                expm1_contig_dispatch_vector, expm1_strided_dispatch_vector);
        };
        m.def("_expm1", expm1_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto expm1_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              expm1_output_typeid_vector);
        };
        m.def("_expm1_result_type", expm1_result_type_pyapi);
    }

    // U15: ==== FLOOR         (x)
    // FIXME:

    // B10: ==== FLOOR_DIVIDE  (x1, x2)
    {
        impl::populate_floor_divide_dispatch_tables();
        using impl::floor_divide_contig_dispatch_table;
        using impl::floor_divide_output_id_table;
        using impl::floor_divide_strided_dispatch_table;

        auto floor_divide_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                      dpctl::tensor::usm_ndarray src2,
                                      dpctl::tensor::usm_ndarray dst,
                                      sycl::queue exec_q,
                                      const std::vector<sycl::event> &depends =
                                          {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, floor_divide_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                floor_divide_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                floor_divide_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto floor_divide_result_type_pyapi = [&](py::dtype dtype1,
                                                  py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               floor_divide_output_id_table);
        };
        m.def("_floor_divide", floor_divide_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_floor_divide_result_type", floor_divide_result_type_pyapi, "");
    }

    // B11: ==== GREATER       (x1, x2)
    {
        impl::populate_greater_dispatch_tables();
        using impl::greater_contig_dispatch_table;
        using impl::greater_output_id_table;
        using impl::greater_strided_dispatch_table;

        auto greater_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                 dpctl::tensor::usm_ndarray src2,
                                 dpctl::tensor::usm_ndarray dst,
                                 sycl::queue exec_q,
                                 const std::vector<sycl::event> &depends = {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, greater_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                greater_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                greater_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto greater_result_type_pyapi = [&](py::dtype dtype1,
                                             py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               greater_output_id_table);
        };
        m.def("_greater", greater_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_greater_result_type", greater_result_type_pyapi, "");
    }

    // B12: ==== GREATER_EQUAL (x1, x2)
    {
        impl::populate_greater_equal_dispatch_tables();
        using impl::greater_equal_contig_dispatch_table;
        using impl::greater_equal_output_id_table;
        using impl::greater_equal_strided_dispatch_table;

        auto greater_equal_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                       dpctl::tensor::usm_ndarray src2,
                                       dpctl::tensor::usm_ndarray dst,
                                       sycl::queue exec_q,
                                       const std::vector<sycl::event> &depends =
                                           {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, greater_equal_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                greater_equal_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                greater_equal_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto greater_equal_result_type_pyapi = [&](py::dtype dtype1,
                                                   py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               greater_equal_output_id_table);
        };
        m.def("_greater_equal", greater_equal_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_greater_equal_result_type", greater_equal_result_type_pyapi,
              "");
    }

    // U16: ==== IMAG        (x)
    {
        impl::populate_imag_dispatch_vectors();
        using impl::imag_contig_dispatch_vector;
        using impl::imag_output_typeid_vector;
        using impl::imag_strided_dispatch_vector;

        auto imag_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                              const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, imag_output_typeid_vector,
                imag_contig_dispatch_vector, imag_strided_dispatch_vector);
        };
        m.def("_imag", imag_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto imag_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, imag_output_typeid_vector);
        };
        m.def("_imag_result_type", imag_result_type_pyapi);
    }

    // U17: ==== ISFINITE    (x)
    {
        impl::populate_isfinite_dispatch_vectors();

        using impl::isfinite_contig_dispatch_vector;
        using impl::isfinite_output_typeid_vector;
        using impl::isfinite_strided_dispatch_vector;
        auto isfinite_pyapi =
            [&](dpctl::tensor::usm_ndarray src, dpctl::tensor::usm_ndarray dst,
                sycl::queue exec_q,
                const std::vector<sycl::event> &depends = {}) {
                return py_unary_ufunc(src, dst, exec_q, depends,
                                      isfinite_output_typeid_vector,
                                      isfinite_contig_dispatch_vector,
                                      isfinite_strided_dispatch_vector);
            };
        auto isfinite_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              isfinite_output_typeid_vector);
        };
        m.def("_isfinite", isfinite_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());
        m.def("_isfinite_result_type", isfinite_result_type_pyapi, "");
    }

    // U18: ==== ISINF       (x)
    {
        impl::populate_isinf_dispatch_vectors();

        using impl::isinf_contig_dispatch_vector;
        using impl::isinf_output_typeid_vector;
        using impl::isinf_strided_dispatch_vector;
        auto isinf_pyapi = [&](dpctl::tensor::usm_ndarray src,
                               dpctl::tensor::usm_ndarray dst,
                               sycl::queue exec_q,
                               const std::vector<sycl::event> &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, isinf_output_typeid_vector,
                isinf_contig_dispatch_vector, isinf_strided_dispatch_vector);
        };
        auto isinf_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              isinf_output_typeid_vector);
        };
        m.def("_isinf", isinf_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());
        m.def("_isinf_result_type", isinf_result_type_pyapi, "");
    }

    // U19: ==== ISNAN       (x)
    {
        impl::populate_isnan_dispatch_vectors();

        using impl::isnan_contig_dispatch_vector;
        using impl::isnan_output_typeid_vector;
        using impl::isnan_strided_dispatch_vector;
        auto isnan_pyapi = [&](dpctl::tensor::usm_ndarray src,
                               dpctl::tensor::usm_ndarray dst,
                               sycl::queue exec_q,
                               const std::vector<sycl::event> &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, isnan_output_typeid_vector,
                isnan_contig_dispatch_vector, isnan_strided_dispatch_vector);
        };
        auto isnan_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              isnan_output_typeid_vector);
        };
        m.def("_isnan", isnan_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());
        m.def("_isnan_result_type", isnan_result_type_pyapi, "");
    }

    // B13: ==== LESS        (x1, x2)
    {
        impl::populate_less_dispatch_tables();
        using impl::less_contig_dispatch_table;
        using impl::less_output_id_table;
        using impl::less_strided_dispatch_table;

        auto less_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                              dpctl::tensor::usm_ndarray src2,
                              dpctl::tensor::usm_ndarray dst,
                              sycl::queue exec_q,
                              const std::vector<sycl::event> &depends = {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, less_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                less_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                less_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto less_result_type_pyapi = [&](py::dtype dtype1, py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               less_output_id_table);
        };
        m.def("_less", less_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_less_result_type", less_result_type_pyapi, "");
    }

    // B14: ==== LESS_EQUAL  (x1, x2)
    {
        impl::populate_less_equal_dispatch_tables();
        using impl::less_equal_contig_dispatch_table;
        using impl::less_equal_output_id_table;
        using impl::less_equal_strided_dispatch_table;

        auto less_equal_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                    dpctl::tensor::usm_ndarray src2,
                                    dpctl::tensor::usm_ndarray dst,
                                    sycl::queue exec_q,
                                    const std::vector<sycl::event> &depends =
                                        {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, less_equal_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                less_equal_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                less_equal_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto less_equal_result_type_pyapi = [&](py::dtype dtype1,
                                                py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               less_equal_output_id_table);
        };
        m.def("_less_equal", less_equal_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_less_equal_result_type", less_equal_result_type_pyapi, "");
    }

    // U20: ==== LOG         (x)
    {
        impl::populate_log_dispatch_vectors();
        using impl::log_contig_dispatch_vector;
        using impl::log_output_typeid_vector;
        using impl::log_strided_dispatch_vector;

        auto log_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                             const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, log_output_typeid_vector,
                log_contig_dispatch_vector, log_strided_dispatch_vector);
        };
        m.def("_log", log_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto log_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, log_output_typeid_vector);
        };
        m.def("_log_result_type", log_result_type_pyapi);
    }

    // U21: ==== LOG1P       (x)
    {
        impl::populate_log1p_dispatch_vectors();
        using impl::log1p_contig_dispatch_vector;
        using impl::log1p_output_typeid_vector;
        using impl::log1p_strided_dispatch_vector;

        auto log1p_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                               const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, log1p_output_typeid_vector,
                log1p_contig_dispatch_vector, log1p_strided_dispatch_vector);
        };
        m.def("_log1p", log1p_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto log1p_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              log1p_output_typeid_vector);
        };
        m.def("_log1p_result_type", log1p_result_type_pyapi);
    }

    // U22: ==== LOG2        (x)
    {
        impl::populate_log2_dispatch_vectors();

        using impl::log2_contig_dispatch_vector;
        using impl::log2_output_typeid_vector;
        using impl::log2_strided_dispatch_vector;
        auto log2_pyapi = [&](dpctl::tensor::usm_ndarray src,
                              dpctl::tensor::usm_ndarray dst,
                              sycl::queue exec_q,
                              const std::vector<sycl::event> &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, log2_output_typeid_vector,
                log2_contig_dispatch_vector, log2_strided_dispatch_vector);
        };
        auto log2_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, log2_output_typeid_vector);
        };
        m.def("_log2", log2_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());
        m.def("_log2_result_type", log2_result_type_pyapi, "");
    }

    // U23: ==== LOG10       (x)
    {
        impl::populate_log10_dispatch_vectors();

        using impl::log10_contig_dispatch_vector;
        using impl::log10_output_typeid_vector;
        using impl::log10_strided_dispatch_vector;
        auto log10_pyapi = [&](dpctl::tensor::usm_ndarray src,
                               dpctl::tensor::usm_ndarray dst,
                               sycl::queue exec_q,
                               const std::vector<sycl::event> &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, log10_output_typeid_vector,
                log10_contig_dispatch_vector, log10_strided_dispatch_vector);
        };
        auto log10_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              log10_output_typeid_vector);
        };
        m.def("_log10", log10_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());
        m.def("_log10_result_type", log10_result_type_pyapi, "");
    }

    // B15: ==== LOGADDEXP   (x1, x2)
    // FIXME:

    // B16: ==== LOGICAL_AND (x1, x2)
    {
        impl::populate_logical_and_dispatch_tables();
        using impl::logical_and_contig_dispatch_table;
        using impl::logical_and_output_id_table;
        using impl::logical_and_strided_dispatch_table;

        auto logical_and_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                     dpctl::tensor::usm_ndarray src2,
                                     dpctl::tensor::usm_ndarray dst,
                                     sycl::queue exec_q,
                                     const std::vector<sycl::event> &depends =
                                         {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, logical_and_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                logical_and_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                logical_and_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto logical_and_result_type_pyapi = [&](py::dtype dtype1,
                                                 py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               logical_and_output_id_table);
        };
        m.def("_logical_and", logical_and_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_logical_and_result_type", logical_and_result_type_pyapi, "");
    }

    // U24: ==== LOGICAL_NOT (x)
    {
        impl::populate_logical_not_dispatch_vectors();
        using impl::logical_not_contig_dispatch_vector;
        using impl::logical_not_output_typeid_vector;
        using impl::logical_not_strided_dispatch_vector;

        auto logical_not_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                                     const event_vecT &depends = {}) {
            return py_unary_ufunc(src, dst, exec_q, depends,
                                  logical_not_output_typeid_vector,
                                  logical_not_contig_dispatch_vector,
                                  logical_not_strided_dispatch_vector);
        };
        m.def("_logical_not", logical_not_pyapi, "", py::arg("src"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());

        auto logical_not_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              logical_not_output_typeid_vector);
        };
        m.def("_logical_not_result_type", logical_not_result_type_pyapi);
    }

    // B17: ==== LOGICAL_OR  (x1, x2)
    {
        impl::populate_logical_or_dispatch_tables();
        using impl::logical_or_contig_dispatch_table;
        using impl::logical_or_output_id_table;
        using impl::logical_or_strided_dispatch_table;

        auto logical_or_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                    dpctl::tensor::usm_ndarray src2,
                                    dpctl::tensor::usm_ndarray dst,
                                    sycl::queue exec_q,
                                    const std::vector<sycl::event> &depends =
                                        {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, logical_or_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                logical_or_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                logical_or_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto logical_or_result_type_pyapi = [&](py::dtype dtype1,
                                                py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               logical_or_output_id_table);
        };
        m.def("_logical_or", logical_or_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_logical_or_result_type", logical_or_result_type_pyapi, "");
    }

    // B18: ==== LOGICAL_XOR (x1, x2)
    {
        impl::populate_logical_xor_dispatch_tables();
        using impl::logical_xor_contig_dispatch_table;
        using impl::logical_xor_output_id_table;
        using impl::logical_xor_strided_dispatch_table;

        auto logical_xor_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                     dpctl::tensor::usm_ndarray src2,
                                     dpctl::tensor::usm_ndarray dst,
                                     sycl::queue exec_q,
                                     const std::vector<sycl::event> &depends =
                                         {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, logical_xor_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                logical_xor_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                logical_xor_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto logical_xor_result_type_pyapi = [&](py::dtype dtype1,
                                                 py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               logical_xor_output_id_table);
        };
        m.def("_logical_xor", logical_xor_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_logical_xor_result_type", logical_xor_result_type_pyapi, "");
    }

    // B19: ==== MULTIPLY    (x1, x2)
    {
        impl::populate_multiply_dispatch_tables();
        using impl::multiply_contig_dispatch_table;
        using impl::multiply_contig_matrix_contig_row_broadcast_dispatch_table;
        using impl::multiply_contig_row_contig_matrix_broadcast_dispatch_table;
        using impl::multiply_output_id_table;
        using impl::multiply_strided_dispatch_table;

        auto multiply_pyapi =
            [&](dpctl::tensor::usm_ndarray src1,
                dpctl::tensor::usm_ndarray src2, dpctl::tensor::usm_ndarray dst,
                sycl::queue exec_q,
                const std::vector<sycl::event> &depends = {}) {
                return py_binary_ufunc(
                    src1, src2, dst, exec_q, depends, multiply_output_id_table,
                    // function pointers to handle operation on contiguous
                    // arrays (pointers may be nullptr)
                    multiply_contig_dispatch_table,
                    // function pointers to handle operation on strided arrays
                    // (most general case)
                    multiply_strided_dispatch_table,
                    // function pointers to handle operation of c-contig matrix
                    // and c-contig row with broadcasting (may be nullptr)
                    multiply_contig_matrix_contig_row_broadcast_dispatch_table,
                    // function pointers to handle operation of c-contig matrix
                    // and c-contig row with broadcasting (may be nullptr)
                    multiply_contig_row_contig_matrix_broadcast_dispatch_table);
            };
        auto multiply_result_type_pyapi = [&](py::dtype dtype1,
                                              py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               multiply_output_id_table);
        };
        m.def("_multiply", multiply_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_multiply_result_type", multiply_result_type_pyapi, "");

        using impl::multiply_inplace_contig_dispatch_table;
        using impl::multiply_inplace_row_matrix_dispatch_table;
        using impl::multiply_inplace_strided_dispatch_table;

        auto multiply_inplace_pyapi =
            [&](dpctl::tensor::usm_ndarray src, dpctl::tensor::usm_ndarray dst,
                sycl::queue exec_q,
                const std::vector<sycl::event> &depends = {}) {
                return py_binary_inplace_ufunc(
                    src, dst, exec_q, depends, multiply_output_id_table,
                    // function pointers to handle inplace operation on
                    // contiguous arrays (pointers may be nullptr)
                    multiply_inplace_contig_dispatch_table,
                    // function pointers to handle inplace operation on strided
                    // arrays (most general case)
                    multiply_inplace_strided_dispatch_table,
                    // function pointers to handle inplace operation on
                    // c-contig matrix with c-contig row with broadcasting
                    // (may be nullptr)
                    multiply_inplace_row_matrix_dispatch_table);
            };
        m.def("_multiply_inplace", multiply_inplace_pyapi, "", py::arg("lhs"),
              py::arg("rhs"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
    }

    // U25: ==== NEGATIVE    (x)
    {
        impl::populate_negative_dispatch_vectors();
        using impl::negative_contig_dispatch_vector;
        using impl::negative_output_typeid_vector;
        using impl::negative_strided_dispatch_vector;

        auto negative_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                                  const event_vecT &depends = {}) {
            return py_unary_ufunc(src, dst, exec_q, depends,
                                  negative_output_typeid_vector,
                                  negative_contig_dispatch_vector,
                                  negative_strided_dispatch_vector);
        };
        m.def("_negative", negative_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto negative_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              negative_output_typeid_vector);
        };
        m.def("_negative_result_type", negative_result_type_pyapi);
    }

    // B20: ==== NOT_EQUAL   (x1, x2)
    {
        impl::populate_not_equal_dispatch_tables();
        using impl::not_equal_contig_dispatch_table;
        using impl::not_equal_output_id_table;
        using impl::not_equal_strided_dispatch_table;

        auto not_equal_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                                   dpctl::tensor::usm_ndarray src2,
                                   dpctl::tensor::usm_ndarray dst,
                                   sycl::queue exec_q,
                                   const std::vector<sycl::event> &depends =
                                       {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, not_equal_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                not_equal_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                not_equal_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto not_equal_result_type_pyapi = [&](py::dtype dtype1,
                                               py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               not_equal_output_id_table);
        };
        m.def("_not_equal", not_equal_pyapi, "", py::arg("src1"),
              py::arg("src2"), py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_not_equal_result_type", not_equal_result_type_pyapi, "");
    }

    // U26: ==== POSITIVE    (x)
    {
        impl::populate_positive_dispatch_vectors();
        using impl::positive_contig_dispatch_vector;
        using impl::positive_output_typeid_vector;
        using impl::positive_strided_dispatch_vector;

        auto positive_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                                  const event_vecT &depends = {}) {
            return py_unary_ufunc(src, dst, exec_q, depends,
                                  positive_output_typeid_vector,
                                  positive_contig_dispatch_vector,
                                  positive_strided_dispatch_vector);
        };
        m.def("_positive", positive_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto positive_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              positive_output_typeid_vector);
        };
        m.def("_positive_result_type", positive_result_type_pyapi);
    }

    // B21: ==== POW         (x1, x2)
    {

        impl::populate_pow_dispatch_tables();
        using impl::pow_contig_dispatch_table;
        using impl::pow_output_id_table;
        using impl::pow_strided_dispatch_table;

        auto pow_pyapi = [&](dpctl::tensor::usm_ndarray src1,
                             dpctl::tensor::usm_ndarray src2,
                             dpctl::tensor::usm_ndarray dst, sycl::queue exec_q,
                             const std::vector<sycl::event> &depends = {}) {
            return py_binary_ufunc(
                src1, src2, dst, exec_q, depends, pow_output_id_table,
                // function pointers to handle operation on contiguous arrays
                // (pointers may be nullptr)
                pow_contig_dispatch_table,
                // function pointers to handle operation on strided arrays (most
                // general case)
                pow_strided_dispatch_table,
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_matrix_contig_row_broadcast_impl_fn_ptr_t>{},
                // function pointers to handle operation of c-contig matrix and
                // c-contig row with broadcasting (may be nullptr)
                td_ns::NullPtrTable<
                    binary_contig_row_contig_matrix_broadcast_impl_fn_ptr_t>{});
        };
        auto pow_result_type_pyapi = [&](py::dtype dtype1, py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               pow_output_id_table);
        };
        m.def("_pow", pow_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_pow_result_type", pow_result_type_pyapi, "");
    }

    // U??: ==== PROJ        (x)
    {
        impl::populate_proj_dispatch_vectors();
        using impl::proj_contig_dispatch_vector;
        using impl::proj_output_typeid_vector;
        using impl::proj_strided_dispatch_vector;

        auto proj_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                              const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, proj_output_typeid_vector,
                proj_contig_dispatch_vector, proj_strided_dispatch_vector);
        };
        m.def("_proj", proj_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto proj_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, proj_output_typeid_vector);
        };
        m.def("_proj_result_type", proj_result_type_pyapi);
    }

    // U27: ==== REAL        (x)
    {
        impl::populate_real_dispatch_vectors();
        using impl::real_contig_dispatch_vector;
        using impl::real_output_typeid_vector;
        using impl::real_strided_dispatch_vector;

        auto real_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                              const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, real_output_typeid_vector,
                real_contig_dispatch_vector, real_strided_dispatch_vector);
        };
        m.def("_real", real_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto real_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, real_output_typeid_vector);
        };
        m.def("_real_result_type", real_result_type_pyapi);
    }

    // B22: ==== REMAINDER   (x1, x2)
    // FIXME:

    // U28: ==== ROUND       (x)
    // FIXME:

    // U29: ==== SIGN        (x)
    // FIXME:

    // U30: ==== SIN         (x)
    {
        impl::populate_sin_dispatch_vectors();
        using impl::sin_contig_dispatch_vector;
        using impl::sin_output_typeid_vector;
        using impl::sin_strided_dispatch_vector;

        auto sin_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                             const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, sin_output_typeid_vector,
                sin_contig_dispatch_vector, sin_strided_dispatch_vector);
        };
        m.def("_sin", sin_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto sin_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, sin_output_typeid_vector);
        };
        m.def("_sin_result_type", sin_result_type_pyapi);
    }
    // U31: ==== SINH        (x)
    // FIXME:

    // U32: ==== SQUARE      (x)
    {
        impl::populate_square_dispatch_vectors();
        using impl::square_contig_dispatch_vector;
        using impl::square_output_typeid_vector;
        using impl::square_strided_dispatch_vector;

        auto square_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                                const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, square_output_typeid_vector,
                square_contig_dispatch_vector, square_strided_dispatch_vector);
        };
        m.def("_square", square_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto square_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype,
                                              square_output_typeid_vector);
        };
        m.def("_square_result_type", square_result_type_pyapi);
    }

    // U33: ==== SQRT        (x)
    {
        impl::populate_sqrt_dispatch_vectors();
        using impl::sqrt_contig_dispatch_vector;
        using impl::sqrt_output_typeid_vector;
        using impl::sqrt_strided_dispatch_vector;

        auto sqrt_pyapi = [&](arrayT src, arrayT dst, sycl::queue exec_q,
                              const event_vecT &depends = {}) {
            return py_unary_ufunc(
                src, dst, exec_q, depends, sqrt_output_typeid_vector,
                sqrt_contig_dispatch_vector, sqrt_strided_dispatch_vector);
        };
        m.def("_sqrt", sqrt_pyapi, "", py::arg("src"), py::arg("dst"),
              py::arg("sycl_queue"), py::arg("depends") = py::list());

        auto sqrt_result_type_pyapi = [&](py::dtype dtype) {
            return py_unary_ufunc_result_type(dtype, sqrt_output_typeid_vector);
        };
        m.def("_sqrt_result_type", sqrt_result_type_pyapi);
    }

    // B23: ==== SUBTRACT    (x1, x2)
    {
        impl::populate_subtract_dispatch_tables();
        using impl::subtract_contig_dispatch_table;
        using impl::subtract_contig_matrix_contig_row_broadcast_dispatch_table;
        using impl::subtract_contig_row_contig_matrix_broadcast_dispatch_table;
        using impl::subtract_output_id_table;
        using impl::subtract_strided_dispatch_table;

        auto subtract_pyapi =
            [&](dpctl::tensor::usm_ndarray src1,
                dpctl::tensor::usm_ndarray src2, dpctl::tensor::usm_ndarray dst,
                sycl::queue exec_q,
                const std::vector<sycl::event> &depends = {}) {
                return py_binary_ufunc(
                    src1, src2, dst, exec_q, depends, subtract_output_id_table,
                    // function pointers to handle operation on contiguous
                    // arrays (pointers may be nullptr)
                    subtract_contig_dispatch_table,
                    // function pointers to handle operation on strided arrays
                    // (most general case)
                    subtract_strided_dispatch_table,
                    // function pointers to handle operation of c-contig matrix
                    // and c-contig row with broadcasting (may be nullptr)
                    subtract_contig_matrix_contig_row_broadcast_dispatch_table,
                    // function pointers to handle operation of c-contig matrix
                    // and c-contig row with broadcasting (may be nullptr)
                    subtract_contig_row_contig_matrix_broadcast_dispatch_table);
            };
        auto subtract_result_type_pyapi = [&](py::dtype dtype1,
                                              py::dtype dtype2) {
            return py_binary_ufunc_result_type(dtype1, dtype2,
                                               subtract_output_id_table);
        };
        m.def("_subtract", subtract_pyapi, "", py::arg("src1"), py::arg("src2"),
              py::arg("dst"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
        m.def("_subtract_result_type", subtract_result_type_pyapi, "");

        using impl::subtract_inplace_contig_dispatch_table;
        using impl::subtract_inplace_row_matrix_dispatch_table;
        using impl::subtract_inplace_strided_dispatch_table;

        auto subtract_inplace_pyapi =
            [&](dpctl::tensor::usm_ndarray src, dpctl::tensor::usm_ndarray dst,
                sycl::queue exec_q,
                const std::vector<sycl::event> &depends = {}) {
                return py_binary_inplace_ufunc(
                    src, dst, exec_q, depends, subtract_output_id_table,
                    // function pointers to handle inplace operation on
                    // contiguous arrays (pointers may be nullptr)
                    subtract_inplace_contig_dispatch_table,
                    // function pointers to handle inplace operation on strided
                    // arrays (most general case)
                    subtract_inplace_strided_dispatch_table,
                    // function pointers to handle inplace operation on
                    // c-contig matrix with c-contig row with broadcasting
                    // (may be nullptr)
                    subtract_inplace_row_matrix_dispatch_table);
            };
        m.def("_subtract_inplace", subtract_inplace_pyapi, "", py::arg("lhs"),
              py::arg("rhs"), py::arg("sycl_queue"),
              py::arg("depends") = py::list());
    }

    // U34: ==== TAN         (x)
    // FIXME:

    // U35: ==== TANH        (x)
    // FIXME:

    // U36: ==== TRUNC       (x)
    // FIXME:
}

} // namespace py_internal
} // namespace tensor
} // namespace dpctl
