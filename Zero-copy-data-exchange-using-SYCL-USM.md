Dpctl includes a DPC++ USM memory manager to allow Python objects to be allocated using USM memory allocators. Supporting USM memory allocators is needed to achieve seamless interoperability between SYCL-based Python extensions. USM pointers make sense within a SYCL context and can be of four allocation types: `"host"`, `"device"`, `"shared"`, or `"unknown"`. Host applications, including CPython interpreter, can work with USM pointers of type `host` and `"shared"` as if they were ordinary host pointers. Accessing `"device"` USM pointers by host applications is not allowed.

## SYCL USM Array Interface
A SYCL library may allocate USM memory for the result that needs to be passed to Python. A native Python extension that makes use of such a library may expose this memory as an instance of Python class that will implement memory management logic (ensures that memory is freed when the instance is no longer needed). The need to manage memory arises whenever a library uses a custom allocator. For example, `daal4py` uses Python capsule to ensure that DAAL-allocated memory is freed using the appropriate deallocator. 

To enable native extensions to pass the memory allocated by a native SYCL library to Numba, or another SYCL-aware Python extension without making a copy, the class must provide `__sycl_usm_array_interface__` attribute which returns a Python dictionary with the following fields:

* `shape`: tuple of integers
* `typestr`: string
* `typedescr`: a list of tuples 
* `data`: (int, bool)
* `strides`: tuple
* `offset`: int
* `version`: int
* `syclobj`: `dpctl.SyclQueue` or `dpctl.SyclContext` object

The dictionary keys align with those of [``numpy.ndarray.__array_interface__``](https://numpy.org/doc/stable/reference/arrays.interface.html) and [``__cuda_array_interface__``](http://numba.pydata.org/numba-doc/latest/cuda/cuda_array_interface.html). For host accessible USM pointers, the object may also implement CPython [PEP-3118](https://www.python.org/dev/peps/pep-3118/) compliant buffer interface which will be used if a `data` key is not present in the dictionary. Use of a buffer interface extends the interoperability to other Python objects, such as `bytes`, `bytearray`, `array.array`, or `memoryview`. The type of the USM pointer stored in the object can be queried using methods of the `dpctl`.
