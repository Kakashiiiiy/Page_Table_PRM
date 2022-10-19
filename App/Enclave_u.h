#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OCALL_PRINT_STRING_DEFINED__
#define OCALL_PRINT_STRING_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_string, (const char* str));
#endif

sgx_status_t dummyalloc(sgx_enclave_id_t eid, long int addr);
sgx_status_t deadlock(sgx_enclave_id_t eid);
sgx_status_t get_page_malloc(sgx_enclave_id_t eid, unsigned long int** retval);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
