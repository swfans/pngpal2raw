#ifndef BFLIBRARY_PRIVBFLOG_H_
#define BFLIBRARY_PRIVBFLOG_H_

extern int verbose;

#define LOGSYNC(format,args...) fprintf(stdout,format "\n", ## args)
#define LOGDBG(format,args...) if (verbose) fprintf(stdout,format "\n", ## args)
#define LOGERR(format,args...) fprintf(stderr,format "\n", ## args)
#define LOGNO(format,args...)

#endif
