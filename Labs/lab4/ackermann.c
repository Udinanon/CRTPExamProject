#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


# define LOG_FATAL_ERROR (1)
# define LOG_ERROR       (2)
# define LOG_WARNING     (3)
# define LOG_DEBUG       (4)
# define LOG_INFO        (5)

typedef unsigned char error_level_t;

static error_level_t log_level = 0;

static void print_log(const char *format, error_level_t err_level, va_list args ) {
    
    if (err_level < log_level) return;
    else switch (err_level)
    {
    case LOG_FATAL_ERROR:
        printf("[FATAL ERROR] ");
        break;

    case LOG_ERROR:
        printf("[ERROR] ");
        break;

    case LOG_WARNING:
        printf("[WARNING] ");
        break;

    case LOG_DEBUG:
        printf("[DEBUG] ");
        break;
    
    case LOG_INFO:
    default:
        printf("[INFO] ");
        break;
    }
    vprintf (format, args);
}


#define __DEFINE_PRINT_LOG(name, errno) \
static inline void name(const char *msg, ...) { \
    va_list args; \
    va_start (args, msg); \
    print_log(msg, errno, args); \
    va_end (args); \
}
__DEFINE_PRINT_LOG(log_fatal, LOG_FATAL_ERROR)
__DEFINE_PRINT_LOG(log_error, LOG_ERROR)
__DEFINE_PRINT_LOG(log_warning, LOG_WARNING)
__DEFINE_PRINT_LOG(log_debug, LOG_DEBUG)
__DEFINE_PRINT_LOG(log_info, LOG_INFO)
#undef __DEFINE_PRINT_LOG


// https://en.wikipedia.org/wiki/Ackermann_function
static int ackermann(int m, int n){
  if (m == 0) return n + 1;
  else if (n == 0) return ackermann(m - 1, 1);
  else return ackermann(m - 1, ackermann(m, n - 1));
}

int main(int argc, char **argv){
  
  if( argc<2) {
      log_error("usage: %s n\n",argv[0]);
      exit(1);
  }
  int n = atoi(argv[1]);
  int count = 0, total = 0, multiplied = 0;

  while(count < n){
    count += 1;
    multiplied *= count;
    if (multiplied < 100) log_info("count: %d\n",count);
    total += ackermann(2, 2);
    total += ackermann(multiplied, n);
    int d1 = ackermann(n, 1);
    total += d1 * multiplied;
    int d2 = ackermann(n, count);
    if (count % 2 == 0) total += d2;
  }
  return total;
}
