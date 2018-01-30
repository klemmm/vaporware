#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

#if  !__x86_64__ || !__linux__
#error "Portability is overrated" 
#endif

#define PAGE_SIZE 4096
#define VALIDATE_AUTH_RESPONSE 143
#define VTABLE_OFFSET 0
#define JMP32REL_OPCODE 0xE9

struct auth_response_s {
	uint64_t steamid;
	uint32_t response;
	uint64_t ownerid;
} __attribute__ ((packed));


static void (*auth_handler)(void *thisptr, struct auth_response_s *response) = NULL;
static void *auth_class = NULL;
static void *handle = NULL;
static void (*real_SteamAPI_RegisterCallback)(void *ptr, uint64_t idx) = NULL;
static void *real_ISteamGameServer_BeginAuthSession = NULL;

static void initialize() __attribute__ ((constructor));

// extern void glue();

static uint64_t BeginAuthSession(void *ticket, uint64_t ticksize, uint64_t steamid) {
	struct auth_response_s resp;

	printf("[vaporware] Handling BeginAuthSession, SteamID=%lu\n", steamid);
	if (auth_handler == NULL) {
		printf("wtf?\n");
		return 1;
	}
	resp.steamid = steamid;
	resp.ownerid = steamid;
	resp.response = 0;
	printf("[vaporware] Simulating ValidateAuthResponse callback, SteamID=%lu\n", steamid);
	auth_handler(auth_class, &resp);
	return 0;
}



void initialize() {
	printf("[vaporware] Init library...\n");
	handle = dlopen(STEAM_DLL, RTLD_LAZY|RTLD_LOCAL);
	if (handle == NULL) {
		printf("[vaporware] Couldn't open Steam DLL %s\n", STEAM_DLL);
		exit(1);
	}

	real_SteamAPI_RegisterCallback = dlsym(handle, "SteamAPI_RegisterCallback");
	real_ISteamGameServer_BeginAuthSession = dlsym(handle, "ISteamGameServer_BeginAuthSession");
	if (!real_SteamAPI_RegisterCallback || !real_ISteamGameServer_BeginAuthSession) {
		printf("[vaporware] Steam DLL %s is not valid\n", STEAM_DLL);
	}
	printf("[vaporware] Address of ISteamGameServer_BeginAuthSession: 0x%lx\n", (uint64_t) real_ISteamGameServer_BeginAuthSession);

	uint64_t page_base = ((uint64_t) real_ISteamGameServer_BeginAuthSession) & ~(PAGE_SIZE - 1UL);
	unsigned char *ptr = real_ISteamGameServer_BeginAuthSession;

	int r = mprotect((void*)page_base, PAGE_SIZE, PROT_READ|PROT_EXEC|PROT_WRITE);
	if (r == -1) {
		perror("mprotect");
		exit(1);
	}

	int64_t offset64 = (char*)BeginAuthSession - (char*)ptr - 5;
	if (offset64 >> 32) {
		printf("[vaporware] JMP offset does not fit (should not happen)\n");
		exit(1);
	}
	int32_t offset = (int32_t) offset64;
	ptr[0] = JMP32REL_OPCODE;

	memcpy(ptr + 1, &offset, sizeof(offset));
	printf("[vaporware] Patch function to jump to new address: 0x%lx\n", (uint64_t) BeginAuthSession);
	r = mprotect((void*)page_base, PAGE_SIZE, PROT_READ|PROT_EXEC);
	if (r == -1) {
		perror("mprotect");
		exit(1);
	}
	printf("[vaporware] Init OK.\n");
}


void SteamAPI_RegisterCallback(void *ptr, uint64_t idx) {
	void *vtable;

	if (idx == VALIDATE_AUTH_RESPONSE) {
		printf("[vaporware] Handling RegisterCallback() for ValidateAuthResponse event\n");
		auth_class = ptr;
		memcpy(&vtable, auth_class, sizeof(vtable));
		memcpy(&auth_handler, vtable + VTABLE_OFFSET, sizeof(auth_handler));
		printf("[vaporware] Got callback handler address: class=0x%lx method=0x%lx\n", (uint64_t) auth_class, (uint64_t) auth_handler);

	} else { 
		real_SteamAPI_RegisterCallback(ptr, idx);
	}
}
