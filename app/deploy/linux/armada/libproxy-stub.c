/* Minimal libproxy replacement for the AppImage: always reports a direct
 * connection so Qt doesn't pull in libproxy's curl/gnutls/p11-kit stack. */
#include <stdlib.h>
#include <string.h>
typedef struct { int unused; } pxProxyFactory;
pxProxyFactory *px_proxy_factory_new(void) { return calloc(1, sizeof(pxProxyFactory)); }
char **px_proxy_factory_get_proxies(pxProxyFactory *f, const char *url) {
    (void)f; (void)url;
    char **r = calloc(2, sizeof(char *));
    if (r) r[0] = strdup("direct://");
    return r;
}
void px_proxy_factory_free_proxies(char **p) {
    if (!p) return;
    for (char **i = p; *i; i++) free(*i);
    free(p);
}
void px_proxy_factory_free(pxProxyFactory *f) { free(f); }
