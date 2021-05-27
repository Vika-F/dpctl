## Roadmap for 0.9  [ target: end of 06/2021 ]

1. Deprecate `dpctl.device_context`, solve [#11](http://github.com/IntelPython/dpctl/issues/11), and remove it `device_context` and 
   associated constructs 0.10
1. For `usm_ndarray` container progress in implementing array-API operations:
   * Implement copy-and-cast operator
   * Copy from/to host/other_usm_ndarray
   * Implement `scatter`, `gather`
   * Implement copy by mask 
   * Implement dlpack support
1. Address [#35](http://github.com/IntelPython/dpctl/issues/35) implementing error handling in the backend library.

### Stretch goal

1. Implement redesigned `SyclEvent` and ``cl::sycl::event`` interface: [#391](http://github.com/IntelPython/dpctl/issues/391)
1. Enable asynchronous queue operations (memcpy, etc.)
1. Design/explore asynchronous operations on `usm_ndarray`.
1. Build example of use `dpctl` from Pybind11

## Roadmap for 0.10

TBD