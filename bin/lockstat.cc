#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "amd64.h"
#include "uk/lockstat.h"

static void
xwrite(int fd, char c)
{
  if (write(fd, &c, 1) != 1)
    die("lockstat: write failed");
}

static void
stats(void)
{
  static const u64 sz = sizeof(struct lockstat);
  struct lockstat ls;
  int sfd, fd;
  int r;

  fd = open("/dev/lockstat", O_RDONLY);
  if (fd < 0)
    die("lockstat: open failed");

  unlink("/lockstat.last");
  sfd = open("/lockstat.last", O_RDWR|O_CREAT);
  if (sfd < 0)
    die("lockstat: open failed");

  fprintf(1, "## name acquires contends locking locked\n");
  fprintf(sfd, "## name acquires contends locking locked\n");
  
  while (1) {
    r = read(fd, &ls, sz);
    if (r < 0)
      die("lockstat: read failed");
    if (r == 0)
      break;
    if (r != sz)
      die("lockstat: unexpected read");

    u64 acquires = 0, contends = 0,
      locking = 0, locked = 0;
    
    for (int i = 0; i < NCPU; i++) {
      acquires += ls.cpu[i].acquires;
      contends += ls.cpu[i].contends;
      locking += ls.cpu[i].locking;
      locked += ls.cpu[i].locked;
    }
    if (contends > 0) {
      fprintf(1, "%s %lu %lu %lu %lu\n", 
             ls.name, acquires, contends, locking, locked);
      fprintf(sfd, "%s %lu %lu %lu %lu\n", 
             ls.name, acquires, contends, locking, locked);
    }
  }

  close(sfd);
}

int
main(int ac, const char *av[])
{
  int fd = open("/dev/lockstat", O_RDWR);
  if (fd < 0)
    die("lockstat: open failed");
  xwrite(fd, '2');
  xwrite(fd, '3');

  int pid = fork(0);
  if (pid < 0)
    die("lockstat: fork failed");

  if (pid == 0) {
    xwrite(fd, '1');
    exec(av[1], av+1);
    die("lockstat: exec failed");
  }
  
  wait();
  xwrite(fd, '2');
  stats();
  xwrite(fd, '3');
  exit();
}
