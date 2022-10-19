/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "../ptedit_header.h"

#include <unistd.h>
#include <pwd.h>
#define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "sgx_user.h"
#include "App.h"
#include "Enclave_u.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t
{
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {SGX_ERROR_UNEXPECTED,
     "Unexpected error occurred.",
     NULL},
    {SGX_ERROR_INVALID_PARAMETER,
     "Invalid parameter.",
     NULL},
    {SGX_ERROR_OUT_OF_MEMORY,
     "Out of memory.",
     NULL},
    {SGX_ERROR_ENCLAVE_LOST,
     "Power transition occurred.",
     "Please refer to the sample \"PowerTransition\" for details."},
    {SGX_ERROR_INVALID_ENCLAVE,
     "Invalid enclave image.",
     NULL},
    {SGX_ERROR_INVALID_ENCLAVE_ID,
     "Invalid enclave identification.",
     NULL},
    {SGX_ERROR_INVALID_SIGNATURE,
     "Invalid enclave signature.",
     NULL},
    {SGX_ERROR_OUT_OF_EPC,
     "Out of EPC memory.",
     NULL},
    {SGX_ERROR_NO_DEVICE,
     "Invalid SGX device.",
     "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."},
    {SGX_ERROR_MEMORY_MAP_CONFLICT,
     "Memory map conflicted.",
     NULL},
    {SGX_ERROR_INVALID_METADATA,
     "Invalid enclave metadata.",
     NULL},
    {SGX_ERROR_DEVICE_BUSY,
     "SGX device was busy.",
     NULL},
    {SGX_ERROR_INVALID_VERSION,
     "Enclave version was invalid.",
     NULL},
    {SGX_ERROR_INVALID_ATTRIBUTE,
     "Enclave was not authorized.",
     NULL},
    {SGX_ERROR_ENCLAVE_FILE_ACCESS,
     "Can't open enclave file.",
     NULL},
    {SGX_ERROR_MEMORY_MAP_FAILURE,
     "Failed to reserve memory for the enclave.",
     NULL},
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++)
    {
        if (ret == sgx_errlist[idx].err)
        {
            if (NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
        printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, 1, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS)
    {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

int open_sgx_driver()
{
    return open("/dev/isgx", O_RDWR);
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    printf("%s\n", str);
}

#define USE_SGX 0
#define PCD 4
#define PWT 3
#define ACCESED 5
#define RESERVED 48
#define PRESENT 0

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    /* Initialize the enclave */
    if (initialize_enclave() < 0)
    {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }
    if (ptedit_init())
    {
        printf("Error: Could not initalize PTEditor, did you load the kernel module?\n");
        return 1;
    }
    unsigned char *addr = (unsigned char *)mmap(NULL, 4 * 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    memset(addr, 0x42, 4 * 1024);
    addr[9] = '\n';
    addr[10] = 0;
    ptedit_entry_t entry = ptedit_resolve(addr, 0);
    long oldPmd = entry.pmd;
    printf("old PMD %llx\n", entry.pmd);
    printf("wanted PTE %llx\n", entry.pte);

    long int sgx_addr;
    if (USE_SGX)
    {
        unsigned long int *pages;
        dummyalloc(global_eid, entry.pte);
        get_page_malloc(global_eid, &pages);
        sgx_addr = (long int)pages;
    }
    else
    {

        unsigned char *non_sgx_test = (unsigned char *)malloc(8192);
        memset(non_sgx_test, 0x0, 8 * 1024);
        for (size_t i = 0; i < 8192 / 8; i++)
        {
            ((long *)non_sgx_test)[i] = entry.pte;
        }
        sgx_addr = (long int)non_sgx_test;
    }
    sgx_addr += 0x1000;
    printf("new PMD virt addr: %lx\n", sgx_addr);
    ptedit_entry_t sgx_entry = ptedit_resolve((void *)sgx_addr, 0);
    printf("new PMD phys addr: %lx\n", sgx_entry.pte);
    sgx_entry.pte &= ~(1ull << 63);
    sgx_entry.pte &= ~(0xfff);

    ptedit_entry_t vm = ptedit_resolve(addr, 0);
    vm.pmd = (entry.pmd & 0xfff) | sgx_entry.pte;
    vm.pmd |= 1 << PCD;
    vm.pmd |= 1 << PWT;
    vm.pmd |= 1ull << RESERVED;
    //vm.pmd &= ~(1 << ACCESED);
    //vm.pmd &= ~(1 << PRESENT);
    printf("new modified PMD entry: %lx\n", vm.pmd);
    vm.valid = PTEDIT_VALID_MASK_PMD;
    ptedit_update(addr, 0, &vm);

    ((unsigned char volatile *)addr)[0] = 1;
    printf("addr values: %s\n", addr);
    vm = ptedit_resolve(addr, 0);
    printf("Switching PMD back from %lx to %lx\n", vm.pmd, oldPmd);
    vm.pmd = oldPmd;
    vm.valid = PTEDIT_VALID_MASK_PMD;
    ptedit_update(addr, 0, &vm);
    printf("addr values: %s\n", addr);

    munmap(addr, 4096);
    ptedit_cleanup();

    // int sgxfd = open_sgx_driver();

    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);

    printf("Info: SampleEnclave successfully returned.\n");

    printf("Enter a character before exit ...\n");
    getchar();
    return 0;
}
