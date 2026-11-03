/*
 * timefixd - one-shot SNTP sync for jailbroken iOS.
 * launchd runs it every few minutes. If the system clock differs from
 * NTP by more than THRESHOLD_SEC, the clock is corrected.
 * Does not use GPS or HTTPS, so it works even when the date is wrong.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#ifdef __APPLE__
#include <notify.h>
#endif

#define NTP_UNIX_DELTA   2208988800.0
#define THRESHOLD_SEC    5.0
#define MAX_RTT_SEC      3.0
#define MIN_VALID_UNIX   1700000000.0   /* reject server times before Nov 2023 */

static const char *servers[] = {
    "time.apple.com", "time.google.com", "time.cloudflare.com", "pool.ntp.org", NULL
};

typedef struct {
    uint8_t  li_vn_mode;
    uint8_t  stratum;
    uint8_t  poll;
    int8_t   precision;
    uint32_t root_delay;
    uint32_t root_disp;
    uint32_t ref_id;
    uint32_t ref_ts[2];
    uint32_t orig_ts[2];
    uint32_t rx_ts[2];
    uint32_t tx_ts[2];
} ntp_packet;

typedef char ntp_packet_must_be_48_bytes[(sizeof(ntp_packet) == 48) ? 1 : -1];

static double now_d(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1e6;
}

/* returns 0 on success and stores the estimated current Unix time in *out */
static int query(const char *host, double *out) {
    struct addrinfo hints, *res = NULL, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, "123", &hints, &res) != 0) return -1;

    int ok = -1;
    for (ai = res; ai && ok != 0; ai = ai->ai_next) {
        int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;

        struct timeval to = { 4, 0 };
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to);

        ntp_packet p, r;
        memset(&p, 0, sizeof p);
        p.li_vn_mode = (0 << 6) | (4 << 3) | 3;   /* LI=0, VN=4, mode=client */

        double t1 = now_d();
        if (sendto(fd, &p, sizeof p, 0, ai->ai_addr, ai->ai_addrlen) == (ssize_t)sizeof p) {
            ssize_t n = recv(fd, &r, sizeof r, 0);
            double t4 = now_d();
            if (n == (ssize_t)sizeof r) {
                int mode = r.li_vn_mode & 7;
                int li   = r.li_vn_mode >> 6;
                /* doubles only: avoids 64-bit int helpers missing from new Xcode on armv7 */
                double secs = (double)ntohl(r.tx_ts[0]);
                double frac = (double)ntohl(r.tx_ts[1]);
                if (secs < NTP_UNIX_DELTA) secs += 4294967296.0;  /* NTP era 1 */
                double t3 = (secs - NTP_UNIX_DELTA) + frac / 4294967296.0;
                double rtt = t4 - t1;

                if (mode == 4 && li != 3 && r.stratum > 0 && r.stratum < 16 &&
                    rtt >= 0 && rtt < MAX_RTT_SEC && t3 > MIN_VALID_UNIX) {
                    *out = t3 + rtt / 2.0;
                    ok = 0;
                }
            }
        }
        close(fd);
    }
    freeaddrinfo(res);
    return ok;
}

int main(void) {
    openlog("timefixd", LOG_PID, LOG_DAEMON);

    for (int i = 0; servers[i]; i++) {
        double srv;
        if (query(servers[i], &srv) != 0) continue;

        double local = now_d();
        double off = srv - local;
        if (fabs(off) < THRESHOLD_SEC) return 0;   /* clock is fine */

        srv += (now_d() - local);                  /* account for time spent */
        struct timeval tv;
        tv.tv_sec  = (time_t)floor(srv);
        tv.tv_usec = (suseconds_t)((srv - floor(srv)) * 1e6);

        if (settimeofday(&tv, NULL) != 0) {
            syslog(LOG_ERR, "settimeofday failed (not root?)");
            return 2;
        }
#ifdef __APPLE__
        notify_post("com.apple.system.clock_set");
#endif
        syslog(LOG_NOTICE, "clock corrected by %.1f s using %s", off, servers[i]);
        return 0;
    }
    syslog(LOG_WARNING, "no NTP server reachable");
    return 1;
}
