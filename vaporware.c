#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

#include "vaporware.h"

static void initialize() __attribute__ ((constructor));

static void *handle = NULL;

static void *auth_class = NULL;
static void (*auth_handler)(void *thisptr, struct auth_response_s *response, struct auth_response_s *r2, struct auth_response_s *r3) = NULL;

static void (*RegisterCallback_fnptr)(void*, int) = NULL;
static char RegisterCallback_save[PATCH_SIZE];

static uint64_t VaporWare_BeginAuthSession(void *clazz, void *ticket, uint64_t ticksize, uint64_t steamid) {
	struct auth_response_s resp;

	fprintf(stderr, "[vaporware] Handling BeginAuthSession, SteamID=%lu\n", steamid);
	if (auth_handler == NULL) {
		fprintf(stderr, "wtf?\n");
		return 1;
	}
	resp.steamid = steamid;
	resp.ownerid = steamid;
	resp.response = 0;
	fprintf(stderr, "[vaporware] Simulating ValidateAuthResponse callback, SteamID=%lu\n", steamid);
	auth_handler(auth_class, &resp, &resp, &resp);
	return 0;
}


void VaporWare_RegisterCallback(void *clazz, int cb_type) { 
	void *vtable;
	if (cb_type == VALIDATE_AUTH_RESPONSE) {
		auth_class = clazz;
		memcpy(&vtable, auth_class, sizeof(vtable));
		memcpy(&auth_handler, vtable + VTABLE_OFFSET, sizeof(auth_handler));
		fprintf(stderr, "[vaporware] Successfully initialized.\n");
		return;
	} 
	restore(RegisterCallback_fnptr, RegisterCallback_save);
	RegisterCallback_fnptr(clazz, cb_type);
	intercept("SteamAPI_RegisterCallback", VaporWare_RegisterCallback, NULL);
}

void *intercept(const char *sym, void *dst, char *save) {
	if (handle == NULL) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}
	void *src = dlsym(handle, sym);
	unsigned char *ptr = src;
	if (!src) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}
	uint64_t page_base = ((uint64_t) src) & ~(PAGE_SIZE - 1UL);
	int r = mprotect((void*)page_base, PAGE_SIZE, PROT_READ|PROT_EXEC|PROT_WRITE);
	if (r == -1) {
		perror("ERROR: mprotect");
		exit(1);
	}
	int64_t offset64 = (char*)dst - (char*)ptr - 5;
	if (offset64 >> 32) {
		fprintf(stderr, "ERROR: JMP offset does not fit (should not happen)\n");
		exit(1);
	}
	int32_t offset = (int32_t) offset64;
	if (save != NULL)
		memcpy(save, ptr, PATCH_SIZE);
	
	ptr[0] = JMP32REL_OPCODE;

	memcpy(ptr + 1, &offset, sizeof(offset));
	r = mprotect((void*)page_base, PAGE_SIZE, PROT_READ|PROT_EXEC);
	if (r == -1) {
		perror("ERROR: mprotect");
		exit(1);
	}
	return src;
}

void restore(void *src, const char *save) {
	uint64_t page_base = ((uint64_t) src) & ~(PAGE_SIZE - 1UL);
	int r = mprotect((void*)page_base, PAGE_SIZE, PROT_READ|PROT_EXEC|PROT_WRITE);
	if (r == -1) {
		perror("ERROR: mprotect");
		exit(1);
	}
	memcpy(src, save, PATCH_SIZE);

	r = mprotect((void*)page_base, PAGE_SIZE, PROT_READ|PROT_EXEC);
	if (r == -1) {
		perror("ERROR: mprotect");
		exit(1);
	}
}

void initialize() {
	char *dllname = getenv("STEAM_DLL");
	if (dllname == NULL) {
		fprintf(stderr, "[vaporware] Please set STEAM_DLL env var to libsteam_api.so path.\n");
		exit(1);
	}
	handle = dlopen(dllname, RTLD_LAZY|RTLD_LOCAL);
	if (handle == NULL) {
		fprintf(stderr, "[vaporware] Couldn't open Steam DLL %s\n", dllname);
		exit(1);
	}
	intercept("SteamAPI_ISteamGameServer_BeginAuthSession", VaporWare_BeginAuthSession, NULL);
	RegisterCallback_fnptr = intercept("SteamAPI_RegisterCallback", VaporWare_RegisterCallback, RegisterCallback_save);
}

