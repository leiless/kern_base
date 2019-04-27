/*
 * Created 190427 lynnl
 */

#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <mach-o/loader.h>

#include <kern/locks.h>
#include <string.h>

#include <sys/sysctl.h>

#ifndef __TS__
#define __TS__      "????/??/?? ??:??:??+????"
#endif

#define KERN_ADDR_MASK      0xfffffffffff00000

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(*a))
#define ARRAY_LAST(a)       (ARRAY_SIZE(a) - 1)

static uint64_t hib;
static uint64_t slide;

#define ADDR_BUFSZ          19

static char hib_str[ADDR_BUFSZ];
static char slide_str[ADDR_BUFSZ];

static SYSCTL_NODE(
    _kern,
    OID_AUTO,
    addr,
    CTLFLAG_RD,
    NULL,
    "" /* sysctl node: kern.addr */
)

static SYSCTL_STRING(
    _kern_addr,
    OID_AUTO,
    __hib,
    CTLFLAG_RD,
    hib_str,
    ARRAY_LAST(hib_str),
    "" /* sysctl nub: kern.addr.hib */
)

static SYSCTL_STRING(
    _kern_addr,
    OID_AUTO,
    slide,
    CTLFLAG_RD,
    slide_str,
    ARRAY_LAST(slide_str),
    "" /* sysctl nub: kern.addr.slide */
)

/*
 * All node entries should place before nub entries
 */
static struct sysctl_oid *sysctl_entries[] = {
    /* sysctl nodes */
    &sysctl__kern_addr,

    /* sysctl nubs */
    &sysctl__kern_addr___hib,
    &sysctl__kern_addr_slide,
};

static inline void addr_sysctl_register(void)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(sysctl_entries); i++) {
        sysctl_register_oid(sysctl_entries[i]);
    }
}

static inline void addr_sysctl_deregister(void)
{
    ssize_t i;
    /* Deregister should follow inverted order */
    for (i = ARRAY_LAST(sysctl_entries); i >= 0; i--) {
        if (sysctl_entries[i] != NULL) {
            /* Double sysctl_unregister_oid() call causes panic */
            sysctl_unregister_oid(sysctl_entries[i]);
            sysctl_entries[i] = NULL;
        }
    }
}

kern_return_t kern_slide_start(kmod_info_t *ki __unused, void *d __unused)
{
    uint32_t t;

    /* hib should less than base(specifically less 0x100000) */
    hib = ((uint64_t) bcopy) & KERN_ADDR_MASK;

    slide = ((uintptr_t) lck_mtx_assert) & KERN_ADDR_MASK;
    while (1) {
        t = *(uint32_t *) slide;
        /* Only support non-fat 64-bit mach-o kernel */
        if (t == MH_MAGIC_64 || t == MH_CIGAM_64) break;
        slide -= 0x100000;
    }

    if (slide != hib + 0x100000) return KERN_FAILURE;

    (void) snprintf(hib_str, ARRAY_SIZE(hib_str), "%#018llx", hib);
    (void) snprintf(slide_str, ARRAY_SIZE(hib_str), "%#018llx", slide);

    addr_sysctl_register();

    return KERN_SUCCESS;
}

kern_return_t kern_slide_stop(kmod_info_t *ki __unused, void *d __unused)
{
    addr_sysctl_deregister();
    return KERN_SUCCESS;
}

#ifdef __kext_makefile__
extern kern_return_t _start(kmod_info_t *, void *);
extern kern_return_t _stop(kmod_info_t *, void *);

/* Will expand name if it's a macro */
#define KMOD_EXPLICIT_DECL2(name, ver, start, stop) \
    __attribute__((visibility("default")))          \
        KMOD_EXPLICIT_DECL(name, ver, start, stop)

KMOD_EXPLICIT_DECL2(BUNDLEID, KEXTBUILD_S, _start, _stop)

/* If you intended to write a kext library  NULLify _realmain and _antimain */
__private_extern__ kmod_start_func_t *_realmain = kern_slide_start;
__private_extern__ kmod_stop_func_t *_antimain = kern_slide_stop;

__private_extern__ int _kext_apple_cc = __APPLE_CC__;
#endif

