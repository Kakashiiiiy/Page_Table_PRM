#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_dummyalloc_t {
	long int ms_addr;
} ms_dummyalloc_t;

typedef struct ms_get_page_malloc_t {
	unsigned long int* ms_retval;
} ms_get_page_malloc_t;

typedef struct ms_ocall_print_string_t {
	const char* ms_str;
} ms_ocall_print_string_t;

static sgx_status_t SGX_CDECL Enclave_ocall_print_string(void* pms)
{
	ms_ocall_print_string_t* ms = SGX_CAST(ms_ocall_print_string_t*, pms);
	ocall_print_string(ms->ms_str);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[1];
} ocall_table_Enclave = {
	1,
	{
		(void*)Enclave_ocall_print_string,
	}
};
sgx_status_t dummyalloc(sgx_enclave_id_t eid, long int addr)
{
	sgx_status_t status;
	ms_dummyalloc_t ms;
	ms.ms_addr = addr;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t deadlock(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t get_page_malloc(sgx_enclave_id_t eid, unsigned long int** retval)
{
	sgx_status_t status;
	ms_get_page_malloc_t ms;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

