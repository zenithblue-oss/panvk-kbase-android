/* Host repro for the bc4 vkd3d_heap hang (csf-v11 209).
 *
 * vkd3d_heap creates the VkInstance in the parent (panvk creates the disk
 * cache and its "disk$" util_queue worker there), then fork()s one child per
 * case. The child's pipeline compile queues a disk_cache_put, and its
 * vkDestroyInstance -> disk_cache_destroy -> util_queue_finish waits for a
 * worker that only exists in the parent.
 *
 * This does the same with the disk cache alone:
 *   parent: disk_cache_create, fork
 *   child:  N x disk_cache_put, disk_cache_destroy (5 s alarm = HANG)
 *   parent: disk_cache_get of the child's keys (the child's writes must land,
 *           so forked processes still get the cache speed-up)
 *
 * Build (B = a host Mesa build dir, M = the Mesa tree under test):
 *   for f in u_queue disk_cache disk_cache_os; do
 *     cc <flags of src/util/libmesa_util.a.p/$f.c.o from B/compile_commands.json,
 *         with -I paths pointing at M> -c M/src/util/$f.c -o $f.o; done
 *   cc -IM/src -IM/include -IM/src/util -IB/src test.c u_queue.o disk_cache.o
 *      disk_cache_os.o B/src/util/libmesa_util.a B/src/c11/impl/libmesa_util_c11.a
 *      -lz -lm -pthread -o test
 * Run: ./test <empty dir> [puts]   -> "PASS" (exit 0) or "HANG" (exit 2)
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util/disk_cache.h"

static void
on_alarm(int sig)
{
   (void)sig;
   static const char msg[] = "HANG child disk_cache_destroy did not return in 5 s\n";
   (void)write(2, msg, sizeof(msg) - 1);
   _exit(2);
}

int
main(int argc, char **argv)
{
   if (argc < 2) {
      fprintf(stderr, "usage: %s <cache dir> [puts]\n", argv[0]);
      return 64;
   }
   int puts_n = argc > 2 ? atoi(argv[2]) : 3;
   setenv("MESA_SHADER_CACHE_DIR", argv[1], 1);
   unsetenv("MESA_SHADER_CACHE_DISABLE");

   struct disk_cache *cache = disk_cache_create("uqfork_test", "id0", 0);
   if (!cache) {
      fprintf(stderr, "FAIL disk_cache_create\n");
      return 1;
   }

   /* Also exercise a parent job queued right before fork. */
   cache_key pk;
   disk_cache_compute_key(cache, "parent", 6, pk);
   disk_cache_put(cache, pk, "parent-data", 12, NULL);

   pid_t child = fork();
   if (child < 0)
      return 1;
   if (!child) {
      signal(SIGALRM, on_alarm);
      alarm(5);
      for (int i = 0; i < puts_n; i++) {
         char name[32], data[64];
         snprintf(name, sizeof(name), "child%d", i);
         snprintf(data, sizeof(data), "child-data-%d", i);
         cache_key k;
         disk_cache_compute_key(cache, name, strlen(name), k);
         disk_cache_put(cache, k, data, strlen(data) + 1, NULL);
      }
      disk_cache_destroy(cache);
      _exit(0);
   }

   int status = 0;
   waitpid(child, &status, 0);
   if (!WIFEXITED(status) || WEXITSTATUS(status)) {
      printf("FAIL child status 0x%x\n", status);
      disk_cache_destroy(cache);
      return 2;
   }

   int found = 0;
   for (int i = 0; i < puts_n; i++) {
      char name[32], want[64];
      snprintf(name, sizeof(name), "child%d", i);
      snprintf(want, sizeof(want), "child-data-%d", i);
      cache_key k;
      size_t size = 0;
      disk_cache_compute_key(cache, name, strlen(name), k);
      char *got = disk_cache_get(cache, k, &size);
      if (got && size == strlen(want) + 1 && !strcmp(got, want))
         found++;
      free(got);
   }
   disk_cache_wait_for_idle(cache);
   size_t psize = 0;
   char *pgot = disk_cache_get(cache, pk, &psize);
   int parent_ok = pgot && !strcmp(pgot, "parent-data");
   free(pgot);
   disk_cache_destroy(cache);

   printf("child_entries_found %d/%d parent_entry %d\n", found, puts_n, parent_ok);
   if (found != puts_n || !parent_ok) {
      printf("FAIL\n");
      return 1;
   }
   printf("PASS\n");
   return 0;
}
