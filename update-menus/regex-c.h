#ifndef REGEX_C_H
#define REGEX_C_H

#ifdef __cplusplus
  extern "C" {
#endif

extern void *re_alloc_pattern_c();
extern char *re_compile_pattern_c (const char *REGEX, const int REGEX_SIZE,
				   void  *PATTERN_BUFFER);

#ifdef __cplusplus
  }
#endif



#endif
