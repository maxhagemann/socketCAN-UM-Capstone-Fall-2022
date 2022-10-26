#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/bcm.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "common.h"


void can_up_down();

struct state {
	int one_run;
	int verbose;
	int dump;
	int no_kcov;
	char *user_dmesg;
};

static void usage(const char *command)
{
	fprintf(stderr,
		"Usage:\n"
		"\n"
		"    %s [options]\n"
		"\n"
		"Options:\n"
		"\n"
		"  -v --verbose       Print stuff to stderr\n"
		"  -r --one-run       Exit after first data read\n"
		"  -d --dump          Dump KCOV offsets\n"
		"  -k --no-kcov       Don't attempt to run KCOV\n"
		"  -m --dmesg=FILE    Copy /dev/kmsg into a file\n"
		"\n",
		command);
}

void
can_up_down(char *finp)
{
	int sock, sockbcm;
	int nbytes;
	struct ifreq ifr;
	struct sockaddr_can addr;
	struct can_frame frame, rcvframe;
	struct can_filter rfilter[1];
	canid_t cid;
	canid_t filter_cid;
	struct { /* bcm message to the bcm socket */
		struct bcm_msg_head msg_head;
	        struct can_frame frame[1];
	} msg;
	unsigned char candata[8];



	/* 1 byte to fuzz the third arg of socket */
	/* docu says can either be CAN_RAW or BCM (broadcast) */
	if ((sock = socket(PF_CAN, SOCK_RAW, finp[0])) < 0) { /* make the socket */
		fprintf(stderr, "CAN't CAN Socket");
		return;
	}
	strcpy(ifr.ifr_name, "vcan0" ); /* create netif named appropriately for can */
	printf("[can] interface up\n");
	/* mark flag up */
	ifr.ifr_flags |= IFF_UP; /* bring it up */
	ioctl(sock, SIOCSIFFLAGS, &ifr);

	memset(&addr, 0, sizeof(addr));
	/* 1 byte for the can_family */
	/* docu says it can be AF_CAN */
	addr.can_family = finp[1];
	/* 1 byte for the ifr.ifindex */
	/* docu says to have that be the interface we brought up */
	addr.can_ifindex = finp[2];

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) { /* bind the socket to the if */
		fprintf(stderr, "CAN't bind");
		return;
	}

	/* craft a fuzzy can msg */
	/* 4 bytes for the can id */
	memcpy(&cid, finp+3, 4); /* copy 4 fuzzy bytes to where cid lives */
	frame.can_id = cid;
	/* 1 byte for the dlc which is supposedly the can msg length */
	frame.can_dlc = finp[7];
	/* 1 byte for a padding field */
	frame.__pad = finp[8];
	/* 1 byte for a reserved (res0) field */
	frame.__res0 = finp[9];
	/* 8 bytes for the message contents */
	memcpy(frame.data, finp+10, 8);

	if ((sockbcm = socket(PF_CAN, SOCK_RAW, CAN_BCM)) < 0) { /* make a broadcast control socket */
		fprintf(stderr, "CAN't CAN BCM Socket");
		return;
	}
	/* connect the bcm socket, it says in docu that you can't bind it */
	connect(sockbcm, (struct sockaddr *)&addr, sizeof(addr));

	msg.msg_head.opcode  = finp[19];
	msg.msg_head.can_id  = finp[20];
	msg.msg_head.flags   = finp[21];
	msg.msg_head.nframes = finp[22];
	memcpy(msg.frame, finp+23, 8);

	write(sockbcm, &msg, sizeof(msg));

	memcpy(&filter_cid, finp+32, 4); /* copy 4 fuzzy bytes to where filter_cid lives */
	rfilter[0].can_id   = filter_cid;
	rfilter[0].can_mask = finp[36];

	setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	/* send the message */
	if (write(sock, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
		fprintf(stderr, "write");
		return;
	}

	/* read a can frame */
	nbytes = read(sock, &rcvframe, sizeof(struct can_frame));
	if (nbytes < 0) {
		fprintf(stderr, "can raw socket read");
       		return; 
	}

	/* mark flag down */
	ifr.ifr_flags &= ~IFF_UP; /* bring the interface down */
	ioctl(sock, SIOCSIFFLAGS, &ifr);
	printf("[can] interface down\n");
}

int main(int argc, char **argv)
{
	static struct option long_options[] = {
		{"verbose", no_argument, 0, 'v'},
		{"one-run", no_argument, 0, 'r'},
		{"dump", no_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{"no-kcov", no_argument, 0, 'k'},
		{"dmesg", required_argument, 0, 'm'},
		{NULL, 0, 0, 0}};

	const char *optstring = optstring_from_long_options(long_options);

	struct state *state = calloc(1, sizeof(struct state));

	optind = 1;
	while (1) {
		int option_index = 0;
		int arg = getopt_long(argc, argv, optstring, long_options,
				      &option_index);
		if (arg == -1) {
			break;
		}

		switch (arg) {
		case 0:
			fprintf(stderr, "Unknown option: %s", argv[optind]);
			exit(-1);
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
			break;
		case '?':
			exit(-1);
			break;
		case 'v':
			state->verbose++;
			break;
		case 'r':
			state->one_run++;
			break;
		case 'd':
			state->dump++;
			break;
		case 'k':
			state->no_kcov++;
			break;
		case 'm':
			state->user_dmesg = optarg;
			break;
		default:
			fprintf(stderr, "Unknown option %c: %s\n", arg,
				argv[optind]);
			exit(-1);
		}
	}

	uint32_t child_pid = getpid() + 1;

	struct forksrv *forksrv = forksrv_new();
	forksrv_welcome(forksrv);
	uint8_t *afl_area_ptr = forksrv_area_ptr(forksrv);

	struct kcov *kcov = NULL;
	uint64_t *kcov_cover_buf = NULL; /* where the kcov measurements reside*/
	uint64_t kcov_n; /* how many kcov measurements */
	if (state->no_kcov == 0) {
		kcov = kcov_new();
		kcov_cover_buf = kcov_cover(kcov);
	}

	if (state->verbose) {
		fprintf(stderr, "[.] Starting v=%d r=%d\n",
			state->verbose, state->one_run);
	}

	/* Read on dmesg /dev/kmsg for crashes. */
	int dmesg_fs = -1;
	dmesg_fs = open("/dev/kmsg", O_RDONLY | O_NONBLOCK);
	if (dmesg_fs < 0) {
		PFATAL("open(/dev/kmsg)");
	}
	lseek(dmesg_fs, 0, SEEK_END);

	/* Perhaps copy over dmesg data to user file */
	int user_dmesg_fs = -1;
	if (state->user_dmesg) {
		user_dmesg_fs = open(state->user_dmesg,
				     O_APPEND | O_WRONLY | O_CREAT, 0644);
		if (user_dmesg_fs < 0) {
			PFATAL("can't open %s for append", state->user_dmesg);
		}
		char hello[] = ",;: Restarting fuzzing\n";
		int x = write(user_dmesg_fs, hello, strlen(hello));
		(void)x;
	}

	/* MAIN LOOP */
	int run_no;
	for (run_no = 0; 1; run_no += 1) {
		/* Convince AFL we started a child. */
		forksrv_cycle(forksrv, child_pid);

		/* Load input from AFL (stdin) */
		char buf[512 * 1024];
		memset(buf, 0, 32);
		int buf_len = read(0, buf, sizeof(buf));
		if (buf_len < 0) {
			PFATAL("read(stdin)");
		}
		if (buf_len < 5) {
			buf_len = 5;
		}
		if (state->verbose) {
			fprintf(stderr, "[.] %d bytes on input\n", buf_len);
		}

		int kcov_len = 0;


		/* reset number of measurements */
		__atomic_store_n(&kcov_cover_buf[0], 0, __ATOMIC_RELAXED);
		/* START coverage collection on the current task. */
		if (kcov) {
			kcov_enable(kcov);
		}

		can_up_down(buf);
		/* uncomment to read and print coverage*/
		/*
		kcov_n = __atomic_load_n(&kcov_cover_buf[0], __ATOMIC_RELAXED);
		for (uint64_t i = 0; i < kcov_n; i++)
			printf("0x%lx\n", kcov_cover_buf[i + 1]);
		*/

		/* STOP coverage */
		if (kcov) {
			kcov_len = kcov_disable(kcov);
		}

		/* Read recorded %rip */
		int i;
		uint64_t afl_prev_loc = 0;
		for (i = 0; i < kcov_len; i++) {
			uint64_t current_loc = kcov_cover_buf[i + 1];
			uint64_t hash = hsiphash_static(&current_loc,
							sizeof(unsigned long));
			uint64_t mixed = (hash & 0xffff) ^ afl_prev_loc;
			afl_prev_loc = (hash & 0xffff) >> 1;

			uint8_t *s = &afl_area_ptr[mixed];
			int r = __builtin_add_overflow(*s, 1, s);
			if (r) {
				/* Boxing. AFL is fine with overflows,
				 * but we can be better. Drop down to
				 * 128 on overflow. */
				*s = 128;
			}

			if (state->dump) {
				printf("0x%016lx%s\n", current_loc, "");
			}
		}

		if (state->verbose) {
			fprintf(stderr, "[.] %d measurements\n", kcov_len);
		}

		/* Check dmesg if there was something interesting */
		int crashed = 0;
		while (1) {
			// /dev/kmsg gives us one line per read
			char buf[8192];
			int r = read(dmesg_fs, buf, sizeof(buf) - 1);
			if (r <= 0) {
				break;
			}
			if (state->user_dmesg) {
				int x = write(user_dmesg_fs, buf, r);
				(void)x;
			}

			buf[r] = '\x00';
			if (strstr(buf, "Call Trace") != NULL ||
			    strstr(buf, "RIP:") != NULL ||
			    strstr(buf, "Code:") != NULL) {
				crashed += 1;
			}
		}
		if (crashed) {
			fprintf(stderr, "[!] BUG detected\n");
			forksrv_status(forksrv, 139);
		} else {
			forksrv_status(forksrv, 0);
		}

		if (state->one_run) {
			break;
		}
	}

	forksrv_free(forksrv);
	if (kcov) {
		kcov_free(kcov);
	}
	return 0;
}
