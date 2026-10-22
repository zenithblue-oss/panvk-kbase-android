/* Host test of src/util/u_android_app_dir.h (csf-v11/204).
 *
 *   cc -O1 -Wall -I $MESA/src -o /tmp/acd test.c && /tmp/acd
 *
 * Builds a fake /data tree under a temp root (U_ANDROID_APP_DIR_ROOT is a
 * compile-time prefix, so the test uses a fixed one under /tmp) and checks
 * the parsers and the resolver for the PanProbe layout: the app forks and
 * execve()s /data/app/~~x/<pkg>-y/lib/arm64/libpt_runner.so with
 * HOME=/data, so /proc/self/cmdline is a path and the parent is the app.
 */
#define U_ANDROID_APP_DIR_TEST 1
#define U_ANDROID_APP_DIR_ROOT "/tmp/acd-root"
#include "util/u_android_app_dir.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void
pkg_path(const char *path, const char *want)
{
   char pkg[256] = "";
   bool ok = u_android_package_from_path(path, pkg, sizeof(pkg));
   if (want)
      CHECK(ok && !strcmp(pkg, want));
   else
      CHECK(!ok);
   printf("%-70s -> %s\n", path ? path : "(null)", ok ? pkg : "-");
}

static void
pkg_name(const char *name, const char *want)
{
   char pkg[256] = "";
   bool ok = u_android_package_from_procname(name, pkg, sizeof(pkg));
   if (want)
      CHECK(ok && !strcmp(pkg, want));
   else
      CHECK(!ok);
   printf("%-70s -> %s\n", name, ok ? pkg : "-");
}

int
main(void)
{
   pkg_name("dev.zenithblue.panvktest.bc4", "dev.zenithblue.panvktest.bc4");
   pkg_name("com.foo.bar:render", "com.foo.bar");
   pkg_name("/data/app/~~a/dev.x-b/lib/arm64/libpt_runner.so", NULL);
   pkg_name("app_process64", NULL);
   pkg_name("zygote64", NULL);
   pkg_name("a..b", NULL);
   pkg_name("1a.b", NULL);
   pkg_name("", NULL);

   pkg_path("/data/app/~~Zx-9_aQ==/dev.zenithblue.panvktest.bc4-Ab-Cd==/lib/arm64/libpt_runner.so",
            "dev.zenithblue.panvktest.bc4");
   pkg_path("/data/app/com.foo.bar-1/lib/arm64/x.so", "com.foo.bar");
   pkg_path("/data/app/com.foo.bar/base.apk", NULL);
   pkg_path("/data/user/0/dev.zenithblue.panvktest.bc4/files/logs", "dev.zenithblue.panvktest.bc4");
   pkg_path("/data/user/10/com.foo.bar", "com.foo.bar");
   pkg_path("/data/user_de/0/com.foo.bar/cache", "com.foo.bar");
   pkg_path("/data/data/com.foo.bar/cache", "com.foo.bar");
   pkg_path("/data/user/x/com.foo.bar", NULL);
   pkg_path("/data", NULL);
   pkg_path("/data/local/tmp", NULL);
   pkg_path(NULL, NULL);

   /* Resolver: fake tree, run in a child forked from a parent whose
    * cmdline is not a package (the host test binary), with cwd under
    * /data/user/0/<pkg>/files. Then /data/data fallback via TMPDIR. */
   char buf[512];
   system("rm -rf " U_ANDROID_APP_DIR_ROOT);
   system("mkdir -p " U_ANDROID_APP_DIR_ROOT "/data/user/0/dev.zenithblue.panvktest.bc4/cache "
          U_ANDROID_APP_DIR_ROOT "/data/user/0/dev.zenithblue.panvktest.bc4/files "
          U_ANDROID_APP_DIR_ROOT "/data/data/com.foo.bar/cache");
   const unsigned user = (unsigned)getuid() / 100000;
   if (user != 0)
      printf("note: host uid/100000 = %u, resolver cases need user 0\n", user);

   CHECK(!u_android_app_cache_dir(buf, sizeof(buf)));

   /* cwd outside the root has no /data prefix, so emulate by TMPDIR: the
    * resolver parses the path without the root, then prefixes the root. */
   setenv("TMPDIR", "/data/user/0/dev.zenithblue.panvktest.bc4/cache", 1);
   const char *d = u_android_app_cache_dir(buf, sizeof(buf));
   printf("TMPDIR user dir     -> %s\n", d ? d : "(null)");
   CHECK(d && !strcmp(d, U_ANDROID_APP_DIR_ROOT "/data/user/0/dev.zenithblue.panvktest.bc4/cache"));

   setenv("TMPDIR", "/data/app/~~q/com.foo.bar-z/lib", 1);
   d = u_android_app_cache_dir(buf, sizeof(buf));
   printf("/data/data fallback -> %s\n", d ? d : "(null)");
   CHECK(d && !strcmp(d, U_ANDROID_APP_DIR_ROOT "/data/data/com.foo.bar/cache"));

   /* Not writable: no result. */
   unsetenv("TMPDIR");
   setenv("HOME", "/data/user/0/com.nothere.app", 1);
   CHECK(!u_android_app_cache_dir(buf, sizeof(buf)));

   /* HOME under the app dir works too. */
   setenv("HOME", "/data/user/0/dev.zenithblue.panvktest.bc4", 1);
   d = u_android_app_cache_dir(buf, sizeof(buf));
   CHECK(d != NULL);

   /* Parent cmdline: fork a child whose parent is renamed to a package by
    * writing argv[0] is not portable, so check the /proc/<ppid> read path
    * directly instead. */
   char file[64], name[512];
   snprintf(file, sizeof(file), "/proc/%d/cmdline", (int)getppid());
   CHECK(u_android_read_file(file, name, sizeof(name)) > 0);

   system("rm -rf " U_ANDROID_APP_DIR_ROOT);
   printf("%s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
   return fails != 0;
}
