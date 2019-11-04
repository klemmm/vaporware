#ifndef VAPORWARE_H
#define VAPORWARE_H 1

#if  !__x86_64__ || !__linux__
#error "Portability is overrated" 
#endif

#define PAGE_SIZE 4096
#define VALIDATE_AUTH_RESPONSE 143
#define VTABLE_OFFSET 0
#define JMP32REL_OPCODE 0xE9
#define PATCH_SIZE (1 + sizeof(uint32_t))

struct auth_response_s {
	uint64_t steamid;
	uint32_t response;
	uint64_t ownerid;
} __attribute__ ((packed));

void *intercept(const char *sym, void *dst, char *save);
void restore(void *src, const char *save);

#endif