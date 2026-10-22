/* Fork cost and child thread count with many util_queues (csf-v11 209).
 *
 * Models a panvk process: one disk cache ("disk$") plus Q single-thread
 * util_queues (panvk_heap per GPU queue, panvk_bcraw, ...), each with a live
 * worker and one slow job still queued at fork() time. Then forks F children
 * and times fork() -> child _exit -> waitpid in the parent.
 *
 * Child modes:
 *   exit    : count threads, _exit (pure fork + atfork handler cost)
 *   finish  : count threads, util_queue_finish on every queue and
 *             disk_cache_destroy (what vkDestroyInstance does), count again
 *   job     : like finish, but first add one job to queue 0 and one
 *             disk_cache_put (only those queues may start a worker)
 * The child reports its thread counts through a pipe; a 5 s alarm = HANG.
 *
 * Run: ./fork_cost <empty dir> <queues> <forks> <exit|finish|job>
 */
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "util/disk_cache.h"
#include "util/u_queue.h"

static void
on_alarm(int sig)
{
   (void)sig;
   static const char msg[] = "HANG child did not finish in 5 s\n";
   (void)write(2, msg, sizeof(msg) - 1);
   _exit(2);
}

static int
count_threads(void)
{
   DIR *d = opendir("/proc/self/task");
   int n = 0;
   struct dirent *e;
   if (!d)
      return -1;
   while ((e = readdir(d)))
      n += e->d_name[0] != '.';
   closedir(d);
   return n;
}

static int slow_us = 200 * 1000; /* FC_SLOW_US */

static void
slow_job(void *data, void *gdata, int thread_index)
{
   (void)data; (void)gdata; (void)thread_index;
   if (slow_us)
      usleep(slow_us);
}

static void
nop_job(void *data, void *gdata, int thread_index)
{
   (void)data; (void)gdata; (void)thread_index;
}

static double
now_us(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

int
main(int argc, char **argv)
{
   if (argc < 5) {
      fprintf(stderr, "usage: %s <cache dir> <queues> <forks> <exit|finish|job>\n", argv[0]);
      return 64;
   }
   int nq = atoi(argv[2]), nf = atoi(argv[3]);
   const char *mode = argv[4];
   if (getenv("FC_SLOW_US"))
      slow_us = atoi(getenv("FC_SLOW_US"));
   setenv("MESA_SHADER_CACHE_DIR", argv[1], 1);
   unsetenv("MESA_SHADER_CACHE_DISABLE");

   struct disk_cache *cache = disk_cache_create("uqfork_cost", "id0", 0);
   struct util_queue *qs = calloc(nq, sizeof(*qs));
   struct util_queue_fence *pf = calloc(nq, sizeof(*pf));
   for (int i = 0; i < nq; i++) {
      if (!util_queue_init(&qs[i], "q", 8, 1, 0, NULL))
         return 1;
      util_queue_fence_init(&pf[i]);
   }
   if (cache) {
      cache_key pk;
      disk_cache_compute_key(cache, "parent", 6, pk);
      disk_cache_put(cache, pk, "parent-data", 12, NULL);
      disk_cache_wait_for_idle(cache);
   }
   int parent_threads = count_threads();

   double total = 0, worst = 0;
   int t0_min = 1 << 30, t0_max = 0, t1_max = 0, fails = 0;
   for (int f = 0; f < nf; f++) {
      /* A slow parent job is queued or running on every queue at fork(). */
      for (int i = 0; i < nq; i++)
         util_queue_add_job(&qs[i], &qs[i], &pf[i], slow_job, NULL, 0);

      int p[2];
      if (pipe(p))
         return 1;
      double t0 = now_us();
      pid_t c = fork();
      if (c < 0)
         return 1;
      if (!c) {
         signal(SIGALRM, on_alarm);
         alarm(5);
         int r[2] = {count_threads(), 0};
         if (strcmp(mode, "exit")) {
            if (!strcmp(mode, "job")) {
               struct util_queue_fence cf;
               util_queue_fence_init(&cf);
               util_queue_add_job(&qs[0], &qs[0], &cf, nop_job, NULL, 0);
               if (cache) {
                  cache_key k;
                  disk_cache_compute_key(cache, "child", 5, k);
                  disk_cache_put(cache, k, "child-data", 11, NULL);
               }
               util_queue_fence_wait(&cf);
            }
            for (int i = 0; i < nq; i++)
               util_queue_finish(&qs[i]);
            if (cache)
               disk_cache_wait_for_idle(cache);
            r[1] = count_threads();
            if (cache)
               disk_cache_destroy(cache);
         }
         (void)write(p[1], r, sizeof(r));
         _exit(0);
      }
      close(p[1]);
      int r[2] = {-1, -1};
      (void)read(p[0], r, sizeof(r));
      close(p[0]);
      int st = 0;
      waitpid(c, &st, 0);
      double dt = now_us() - t0;
      if (!WIFEXITED(st) || WEXITSTATUS(st))
         fails++;
      total += dt;
      if (dt > worst)
         worst = dt;
      if (r[0] < t0_min) t0_min = r[0];
      if (r[0] > t0_max) t0_max = r[0];
      if (r[1] > t1_max) t1_max = r[1];

      for (int i = 0; i < nq; i++)
         util_queue_fence_wait(&pf[i]);
   }
   printf("mode=%s queues=%d forks=%d parent_threads=%d child_threads_at_fork=%d..%d "
          "child_threads_after=%d fork_to_exit_us mean=%.0f max=%.0f fails=%d\n",
          mode, nq, nf, parent_threads, t0_min, t0_max, t1_max, total / nf, worst, fails);

   for (int i = 0; i < nq; i++)
      util_queue_destroy(&qs[i]);
   if (cache)
      disk_cache_destroy(cache);
   printf(fails ? "FAIL\n" : "PASS\n");
   return fails ? 2 : 0;
}
