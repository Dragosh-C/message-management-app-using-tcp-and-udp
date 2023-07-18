#ifndef __UTILS_h__
#define __UTILS_h__

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                             \
    }                                                                          \
  } while (0)

#endif // __UTILS_h__