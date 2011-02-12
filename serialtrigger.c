/*
 * serialtrigger
 *
 * Copyright 2008, 2011 Norvald H. Ryeng
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>

char *device = NULL;
char *cts_exec = NULL;
int cts_exit = 0;
char *dcd_exec = NULL;
int dcd_exit = 0;
char *dsr_exec = NULL;
int dsr_exit = 0;
char *ri_exec = NULL;
int ri_exit = 0;
int dtr = 0;
int rts = 0;
int cleardtr = 0;
int clearrts = 0;
int once = 0;
int retest_wait = 20;
int verbosity = 0;

int debugprintf(int level, const char *format, ...)
{
  int res = 0;

  if (level > verbosity)
    return 0;

  va_list args;
  va_start(args, format);
  res = vprintf(format, args);
  va_end(args);

  return res;
}

void show_version()
{
  printf("serialtrigger 0.5\n");
  exit(0);
}

void show_help()
{
  printf("Usage: serialtrigger [OPTION] DEVICE\n");
  printf("\n Actions on signals from DCE:\n");
  printf("  -c, --cts-exec=COMMAND  Execute command on clear to send (CTS)\n");
  printf("  -C, --cts-exit          Exit on clear to send (CTS)\n");
  printf("  -a, --dcd-exec=COMMAND  Execute command on data carrier detect (DCD)\n");
  printf("  -A, --dcd-exit          Exit on data carrier detect (DCD)\n");
  printf("  -s, --dsr-exec=COMMAND  Execute command on data set ready (DSR)\n");
  printf("  -S, --dsr-exit          Exit on data set ready (DSR)\n");
  printf("  -i, --ri-exec=COMMAND   Execute command on ring indicator (RI)\n");
  printf("  -I, --ri-exit           Exit on ring indicator (RI)\n");
  printf("\n Output signals from DTE:\n");
  printf("  -d, --dtr               Set data terminal ready (DTR)\n");
  printf("  -r, --rts               Set ready to send (RTS)\n");
  printf("  -D, --clear-dtr         Clear data terminal ready (DTR) while executing\n");
  printf("                          commands\n");
  printf("  -R, --clear-rts         Clear ready to send (RTS) while executing commands\n");
  printf("\n Control:\n");
  printf("  -o, --once              Exit after executing command once\n");
  printf("  -w, --retest-wait=TIME  Microseconds to wait before retesting\n");
  printf("\n Help and debug:\n");
  printf("  -h, --help              Display this help text\n");
  printf("      --pinout            Show DB-25 and DE-9 pinout\n");
  printf("  -v, --verbose           Show debug output (repeat for more verbosity)\n");
  printf("  -V, --version           Show version information\n");
  printf("\n");
  printf("All triggered commands are executed before triggered exits.\n");
  printf("\n");
  exit(0);
}

void show_pinout()
{
  printf("DB-25 and DE-9 pinout\n\n");
  printf("25-pin  9-pin  Description                           Direction\n");
  printf(" 1             Protective Ground (PG)\n");
  printf(" 2      3      Transmitted Data (TxD)                DCE <- DTE\n");
  printf(" 3      2      Received Data (RxD)                   DCE -> DTE\n");
  printf(" 4      7      Request To Send (RTS)                 DCE <- DTE\n");
  printf(" 5      8      Clear To Send (CTS)                   DCE -> DTE\n");
  printf(" 6      6      Data Set Ready (DSR)                  DCE -> DTE\n");
  printf(" 7      5      Signal Ground (SG)\n");
  printf(" 8      1      Data Carrier Detect (DCD)             DCE -> DTE\n");
  printf("12             Secondary Data Carrier Detect (SDCD)  DCE -> DTE\n");
  printf("13             Secondary Clear To Send (SCTS)        DCE -> DTE\n");
  printf("14             Secondary Transmitted Data (STD)      DCE <- DTE\n");
  printf("16             Secondary Received Data (SRD)         DCE -> DTE\n");
  printf("19             Secondary Request To Send (SRTS)      DCE <- DTE\n");
  printf("20      4      Data Terminal Ready (DTR)             DCE <- DTE\n");
  printf("22      9      Ring Indicator (RI)                   DCE -> DTE\n");
}

void parse_args(int argc, char *argv[])
{
  static struct option long_options[] = {
    {"cts-exec", 1, NULL, 'c'},
    {"cts-exit", 1, NULL, 'C'},
    {"dcd-exec", 1, NULL, 'a'},
    {"dcd-exit", 1, NULL, 'A'},
    {"dsr-exec", 1, NULL, 's'},
    {"dsr-exit", 1, NULL, 'S'},
    {"ri-exec", 1, NULL, 'i'},
    {"ri-exit", 1, NULL, 'I'},
    {"dtr", 0, NULL, 'd'},
    {"rts", 0, NULL, 'r'},
    {"clear-dtr", 0, NULL, 'D'},
    {"clear-rts", 0, NULL, 'R'},
    {"once", 0, NULL, 'o'},
    {"retest-wait", 0, NULL, 'w'},
    {"help", 0, NULL, 'h'},
    {"verbose", 0, NULL, 'v'},
    {"version", 0, NULL, 'V'},
    {"pinout", 0, NULL, 9}
  };
  int c;
  int option_index;

  while ((c = getopt_long(argc, argv, "-c:Ca:As:Si:IdrDRow:hvV", long_options, &option_index)) != -1) {
    switch (c) {
    case 1:
      if (!device)
	device = optarg;
      else
	show_help();
      break;
    case 'c':
      cts_exec = optarg;
      debugprintf(2, "On CTS: %s\n", cts_exec);
      break;
    case 'C':
      cts_exit = 1;
      debugprintf(2, "Exit on CTS\n");
      break;
    case 'a':
      dcd_exec = optarg;
      debugprintf(2, "On DCD: %s\n", dcd_exec);
      break;
    case 'A':
      dcd_exit = 1;
      debugprintf(2, "Exit on DCD\n");
      break;
    case 's':
      dsr_exec = optarg;
      debugprintf(2, "On DSR: %s\n", dsr_exec);
      break;
    case 'S':
      dsr_exit = 1;
      debugprintf(2, "Exit on DSR\n");
      break;
    case 'i':
      ri_exec = optarg;
      debugprintf(2, "On RI: %s\n", ri_exec);
      break;
    case 'I':
      ri_exit = 1;
      debugprintf(2, "Exit on RI\n");
      break;
    case 'd':
      dtr = 1;
      debugprintf(2, "DTR on\n");
      break;
    case 'r':
      rts = 1;
      debugprintf(2, "RTS on\n");
      break;
    case 'D':
      cleardtr = 1;
      debugprintf(2, "Clear DTR on\n");
      break;
    case 'R':
      clearrts = 1;
      debugprintf(2, "Clear RTS on\n");
      break;
    case 'o':
      once = 1;
      debugprintf(2, "Executing only once\n");
      break;
    case 'w':
      retest_wait = atoi(optarg);
      debugprintf(2, "Retest time: %i microseconds\n", retest_wait);
      break;
    case 9:
      show_pinout();
      exit(0);
      break;
    case 'v':
      verbosity++;
      break;
    case 'V':
      show_version();
      break;
    case 'h':
    case '?':
    default:
      show_help();
      break;
    }
  }

  if (!device)
    show_help();

  if (verbosity)
    debugprintf(2, "Verbosity: %i\n", verbosity);
}

int clear_mcr(int fd)
{
  int mcs;

  if ((ioctl(fd, TIOCMGET, &mcs)) == -1) {
    debugprintf(5, "Failed to read MSR");
    return -1;
  }
  if (cleardtr) {
    debugprintf(2, "Clearing DTR\n");
    mcs &= (TIOCM_DTR ^ 0xFFFF);
  }
  if (clearrts) {
    debugprintf(2, "Clearing RTS\n");
    mcs &= (TIOCM_RTS ^ 0xFFFF);
  }
  if ((ioctl(fd, TIOCMSET, &mcs)) == -1) {
    debugprintf(5, "Failed to set MCR");
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int fd, saved_mcs, mcs, ret;
  
  parse_args(argc, argv);
  
  if ((fd = open(device, O_RDWR)) == -1) {
    perror("serialtrigger");
    exit(1);
  }
  ioctl(fd, TIOCMGET, &saved_mcs);

  do {
    /* Set DTR and RTS */
    if ((ioctl(fd, TIOCMGET, &mcs)) == -1) {
      perror("Failed to read MSR");
      usleep(1000000);
      continue;
    }
    if (dtr) {
      debugprintf(2, "Setting DTR\n");
      mcs |= TIOCM_DTR;
    }
    if (rts) {
      debugprintf(2, "Setting RTS\n");
      mcs |= TIOCM_RTS;
    }
    if ((ioctl(fd, TIOCMSET, &mcs)) == -1) {
      perror("Failed to set MCR");
      usleep(1000000);
      continue;
    }

    /* Wait for MSR change */
    mcs = 0;
    if (cts_exec)
      mcs |= TIOCM_CTS;
    if (dcd_exec)
      mcs |= TIOCM_CAR;
    if (dsr_exec)
      mcs |= TIOCM_DSR;
    if (ri_exec)
      mcs |= TIOCM_RNG;
    debugprintf(4, "Waiting for MSR change\n");
    if ((ioctl(fd, TIOCMIWAIT, &mcs)) == -1) {
      perror("Failed to wait for MSR change");
      usleep(1000000);
      continue;
    }
    debugprintf(4, "TIOCMIWAIT returned\n");
    debugprintf(4, "Retesting MSR after %i us wait\n", retest_wait);
    usleep(retest_wait);
    if ((ioctl(fd, TIOCMGET, &mcs)) == -1) {
      perror("Failed to read MSR");
      usleep(1000000);
      continue;
    }

    /* Execute commands */
    if (mcs & TIOCM_CTS && cts_exec) {
      debugprintf(1, "CTS triggered, executing %s\n", cts_exec);
      clear_mcr(fd);
      if ((ret = system(cts_exec)) == -1)
	perror(cts_exec);
      else
	debugprintf(1, "Return value: %i\n", ret);
    }
    if (mcs & TIOCM_CAR && dcd_exec) {
      debugprintf(1, "DCD triggered. Executing %s\n", dcd_exec);
      clear_mcr(fd);
      if ((ret = system(dcd_exec)) == -1)
	perror(dcd_exec);
      else
	debugprintf(1, "Return value: %i\n", ret);
    }
    if (mcs & TIOCM_DSR && dsr_exec) {
      debugprintf(1, "DSR triggered. Executing %s\n", dsr_exec);
      clear_mcr(fd);
      if ((ret = system(dsr_exec)) == -1)
	perror(dsr_exec);
      else
	debugprintf(1, "Return value: %i\n", ret);
    }
    if (mcs & TIOCM_RNG && ri_exec) {
      debugprintf(1, "RI triggered. Executing %s\n", ri_exec);
      clear_mcr(fd);
      if ((ret = system(ri_exec)) == -1)
	perror(ri_exec);
      else
	debugprintf(1, "Return value: %i\n", ret);
    }

    /* Exits */
    if ((mcs & TIOCM_CTS && cts_exit) ||
	(mcs & TIOCM_CAR && dcd_exit) ||
	(mcs & TIOCM_DSR && dsr_exit) ||
	(mcs & TIOCM_RNG && ri_exit))
      exit (0);
  } while (!once);

  ioctl(fd, TIOCMSET, &saved_mcs);

  return 0;
}
