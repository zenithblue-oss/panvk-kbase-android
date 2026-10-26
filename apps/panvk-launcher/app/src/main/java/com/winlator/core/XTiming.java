package com.winlator.core;

import android.util.Log;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * panvk-launcher: X server stage timing, on while a game runs with PANPLAY_XTIMING=1 (GameLoop / Containers set
 * {@link #on}). Once a second, one line to xserver.log and logcat (tag XTiming), times in ms (mean/max):
 *   req      requests handled (X server thread) and the time spent in them, plus the 3 costliest kinds
 *   shm      MIT-SHM PutImage: wait for its locks (the GL thread holds DRAWABLE_MANAGER while it uploads), copy
 *   gl       GL frames drawn: whole onDrawFrame, wait for DRAWABLE_MANAGER, texture upload (count), wake (first
 *            content update to the frame that shows it), gap (start to start)
 */
public final class XTiming {
    public static volatile boolean on;
    private static final String TAG = "XTiming";

    private static final class Stat {
        long n, sum, max;
        void add(long ns) { n++; sum += ns; if (ns > max) max = ns; }
        String fmt() { return String.format(Locale.US, "%.2f/%.2f", n > 0 ? sum / 1e6 / n : 0.0, max / 1e6); }
    }

    private static long windowStart;
    private static final Stat req = new Stat(), shmLock = new Stat(), shmCopy = new Stat();
    private static final Stat glDraw = new Stat(), glLock = new Stat(), glUpload = new Stat(), glWake = new Stat(), glGap = new Stat();
    private static final Map<Integer, Stat> kinds = new HashMap<>();
    private static long lastDrawStart, pendingUpdate;

    /** X server thread: one request, [opcode] < 0 = extension (minor = request data). */
    public static synchronized void request(byte opcode, byte minor, long ns) {
        req.add(ns);
        int key = opcode < 0 ? ((opcode & 0xff) << 8) | (minor & 0xff) : (opcode & 0xff) << 8;
        Stat s = kinds.get(key);
        if (s == null) kinds.put(key, s = new Stat());
        s.add(ns);
        flushIfDue();
    }

    public static synchronized void shmPut(long lockNs, long copyNs) { shmLock.add(lockNs); shmCopy.add(copyNs); }

    /** Any thread: a window's content changed (the GL thread is asked to draw). */
    public static synchronized void contentUpdate() { if (pendingUpdate == 0) pendingUpdate = System.nanoTime(); }

    /** GL thread, at the start of onDrawFrame. */
    public static synchronized void drawStart(long t0) {
        if (lastDrawStart != 0) glGap.add(t0 - lastDrawStart);
        lastDrawStart = t0;
        if (pendingUpdate != 0) { glWake.add(t0 - pendingUpdate); pendingUpdate = 0; }
    }

    public static synchronized void drawEnd(long t0, long lockNs) { glDraw.add(System.nanoTime() - t0); glLock.add(lockNs); flushIfDue(); }

    public static synchronized void upload(long ns) { glUpload.add(ns); }

    private static void flushIfDue() {
        long now = System.nanoTime();
        if (windowStart == 0) { windowStart = now; return; }
        if (now - windowStart < 1_000_000_000L) return;
        StringBuilder top = new StringBuilder();
        kinds.entrySet().stream().sorted((a, b) -> Long.compare(b.getValue().sum, a.getValue().sum)).limit(3).forEach(e -> {
            int k = e.getKey(); Stat s = e.getValue();
            top.append(' ').append(k >> 8).append('.').append(k & 0xff).append(':').append(s.n).append('x').append(s.fmt());
        });
        String line = String.format(Locale.US, "XT %.0fms req=%d busy_ms=%.1f top%s | shm n=%d lock=%s copy=%s | gl n=%d draw=%s lock=%s upload(%d)=%s wake=%s gap=%s",
            (now - windowStart) / 1e6, req.n, req.sum / 1e6, top, shmCopy.n, shmLock.fmt(), shmCopy.fmt(),
            glDraw.n, glDraw.fmt(), glLock.fmt(), glUpload.n, glUpload.fmt(), glWake.fmt(), glGap.fmt());
        XLog.log(line);
        Log.i(TAG, line);
        windowStart = now;
        for (Stat s : new Stat[]{req, shmLock, shmCopy, glDraw, glLock, glUpload, glWake, glGap}) { s.n = s.sum = s.max = 0; }
        kinds.clear();
    }
}
